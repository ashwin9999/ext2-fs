#include "vdi_write.h"
using namespace std;

int write_superblock(VDIFile *f, BootSector boot, unsigned int vdimap[], ext2_super_block &super_block){
  unsigned int superblock_loc = translate(1024, f, boot, vdimap);
  if (lseek(f->file, superblock_loc, SEEK_SET) < 0) {
    cout << "Error! Failed to seek to the superblock" << endl;
    return 1;
  }
  if (write(f->file, &super_block, sizeof(super_block)) != 1024) {
    cout << "Error! superblock was not written correctly!" << endl;
    return 1;
  }
  return 0;
}

int write_group_descriptor(VDIFile *f, BootSector boot, unsigned int vdimap[], unsigned int block_size, ext2_group_descriptor group_desc[], unsigned int numberBlockGroups){
  unsigned int group_start = 0;
  if (block_size == 1024) group_start = block_size*2;
  else group_start = block_size;

  unsigned int group_loc = translate(group_start, f, boot, vdimap);
  if (lseek(f->file, group_loc, SEEK_SET) < 0) {
    cout << "Error! Failed to seek to the group desc" << endl;
    return 1;
  }
  size_t size = sizeof(ext2_group_descriptor)*numberBlockGroups;
  if (write(f->file, group_desc, size) != size){
    cout << "Error! Failed to write group desc" << endl;
    return 1;
  }
  return 0;
}

int write_bitmap(VDIFile *f, BootSector boot, unsigned int vdimap[], unsigned char *bitmap, unsigned int block_size, unsigned int blockID){

  if (lseek(f->file, translate(blockID*block_size, f, boot, vdimap), SEEK_SET) < 0) {
    cout << "Error! Failed to seek to bitmap" << endl;
    return 1;
  }
  if (write(f->file, bitmap, block_size) != block_size) {
    cout << "Error! Failed to write bitmap" << endl;
    return 1;
  }
  return 0;
}

int write_block(VDIFile *f, BootSector boot, unsigned int vdimap[], ext2_inode inode, std::vector<unsigned int> &addresses, unsigned int block_num, unsigned int block_size, unsigned char *data){

  if (block_num*block_size >= inode.size) return -1;
  unsigned char* buffer = (unsigned char*)malloc(block_size);
  int direct_index;
  int single_index;
  int double_index;
  int triple_index;
	
  compute_index(block_num, block_size, direct_index, single_index, double_index, triple_index);
  unsigned int triple_blocknum = inode.i_block[14];
  unsigned int double_blocknum = 0;
  if (triple_index != -1) {
    if (double_index != -1 && single_index != -1 && direct_index == -1) {
      if (lseek(f->file, translate(triple_blocknum*block_size, f, boot, vdimap), SEEK_SET) < 0) return -1;
      if (read(f->file, buffer, block_size) < 0) return -1;
      if (triple_index >= (block_size / 4)) return -1;
      double_blocknum = *(((unsigned int *) buffer) + triple_index);
    } else return -1;
  }
  unsigned int single_blocknum = 0;
  if (double_index != -1) {
    if (single_index != -1 && direct_index == -1) {
      if (triple_index == -1) {
	double_blocknum = inode.i_block[13];
      }
      if (double_blocknum == 0) {
	double_blocknum = addresses.back();
	addresses.pop_back();
	*(((unsigned int *) buffer) + triple_index) = double_blocknum;
	if (lseek(f->file, translate(triple_blocknum*block_size, f, boot, vdimap), SEEK_SET) < 0) return -1;
	if (write(f->file, buffer, block_size) != block_size) return -1;
      }
      if (lseek(f->file, translate(double_blocknum*block_size, f, boot, vdimap), SEEK_SET) < 0) return -1;
      if (read(f->file, buffer, block_size) < 0) return -1;
      if (double_index >= (block_size / 4)) return -1;
      single_blocknum = *(((unsigned int *) buffer) + double_index);
    } else return -1;
  }

	unsigned int direct_blocknum = 0;
	if (single_index != -1) {
		if (direct_index == -1) {
			if (double_index == -1) single_blocknum = inode.i_block[12];
			if (single_blocknum == 0) {
				single_blocknum = addresses.back();
				addresses.pop_back();
				*(((unsigned int *) buffer) + double_index) = single_blocknum;
				if (lseek(f->file, translate(double_blocknum*block_size, f, boot, vdimap), SEEK_SET) < 0) return -1;
				if (write(f->file, buffer, block_size) != block_size) return -1;
			}
			if (lseek(f->file, translate(single_blocknum*block_size, f, boot, vdimap), SEEK_SET) < 0) return -1;
			if (read(f->file, buffer, block_size) < 0) return -1;
			if (single_index >= (block_size / 4)) return -1;
			direct_blocknum = *(((unsigned int *) buffer) + single_index);
		} else return -1;
	}

	if (direct_index != -1) {
		if (direct_index < 12) direct_blocknum = inode.i_block[direct_index];
		else return -1;
	}

	if (direct_blocknum == 0) {
		direct_blocknum = addresses.back();
		addresses.pop_back();
		*(((unsigned int *) buffer) + single_index) = direct_blocknum;
		if (lseek(f->file, translate(single_blocknum*block_size, f, boot, vdimap), SEEK_SET) < 0) return -1;
		if (write(f->file, buffer, block_size) != block_size) return -1;
	}
		
	if (lseek(f->file, translate(direct_blocknum*block_size,f, boot, vdimap), SEEK_SET) < 0) return -1;
	if (write(f->file, data, block_size) != block_size) return -1;
	unsigned int diff = inode.size - (block_num*block_size);
	if (diff >= block_size) return block_size;
	return diff;

}

int write_inode(VDIFile *f, BootSector boot, unsigned int vdimap[], ext2_inode inode, unsigned int inode_num, unsigned int block_size, ext2_super_block super_block, ext2_group_descriptor group_desc[]){

  inode_num--;
  unsigned int group_num = inode_num/super_block.s_inodes_per_group;
  unsigned int offset1 = inode_num % super_block.s_inodes_per_group;
  unsigned int inodes_per_block = block_size/sizeof(ext2_inode);
  unsigned int block_num = group_desc[group_num].inode_table + (offset1/inodes_per_block);
  unsigned int offset2 = inode_num % inodes_per_block;

  if (lseek(f->file, translate(block_num*block_size + offset2*sizeof(ext2_inode), f, boot, vdimap), SEEK_SET) < 0) return 1;
  if (write(f->file, &inode, sizeof(ext2_inode)) != sizeof(ext2_inode)) return 1;

  return 0;
}

bool is_block_free(unsigned char *bitmap, unsigned int block_size, unsigned int block_offset, unsigned int &address){
  for (int i = 0; i < block_size; i++){
    unsigned char current_byte = *(bitmap + i);

    for (int j = 0; j < 8; j++){
      if (!((current_byte >> j) & 0x1)) {
	address = block_offset + i*8 + j;
	if (block_size == 1024) address++;
	*(bitmap + i) = ((1 << j) | current_byte);
	return true;
      }
    }
  }
  return false;
}

bool is_inode_free(unsigned char *bitmap, unsigned int block_size, unsigned int inode_block_offset, unsigned int &address){

  for (int i =0; i < block_size; i++){
    unsigned char current_byte = *(bitmap+i);
    for (int j = 0; j < block_size; j++){
      if (!((current_byte >> j) & 0x1)) {
	address = (inode_block_offset + j + i*8) + 1;
	*(bitmap + i) = ((1 << j) | current_byte);
	return true;
      }
    }
  }
  return false;
}
