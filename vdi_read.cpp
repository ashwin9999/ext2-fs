#include "vdi_read.h"

using namespace std;

int vdiOpen(VDIFile *vdi, char *fn){
  vdi->file = open(fn, O_RDWR);
  vdi->cursor = 0;
  if(vdi->file < 0) return 1;
  return vdi->file;
}

void vdiClose(VDIFile *f){
  close(f->file);
}

off_t vdiSeek(VDIFile *f, off_t offset, int whence){
  off_t location;
  if (whence == SEEK_SET){
    location= lseek(f->file, offset, whence);
    if(location < 0){
      cout << "Error seeking the vdi header" << endl;
      return 1;
    }
    f->cursor = location;
  }
  if (whence == SEEK_CUR){
    location = lseek(f->file, offset, whence);
    if (location < 0){
      cout << "Error seeking the vdi header" << endl;
      return 1;
    }
    f->cursor += offset;
  }
  if(whence == SEEK_END){
    location = lseek(f->file, offset, whence);
    if (location < 0){
      cout << "Error seeking the vdi header" << endl;
      return 1;
    }
    f->cursor = offset + f->fileSize;
  }
  return f->cursor;
}

ssize_t vdiRead(VDIFile *f, void *buf, ssize_t n){
  //cout << "value of buf before reading: " << buf << endl;
  ssize_t numBytes = read(f->file, buf, n);
  //cout << "Cursor " << f->cursor << endl;
  if(numBytes != n) {
    cout << "Error header not read correctly" << endl;
    return 1;
  }
  return 0;
}

/*ssize_t vdiWrite(VDIFile *f, void *buf, ssize_t n){
  f->fileStructure.write(buf, n);
  if(f->fileStructure){
    return n;
  }
  else{
    return -1;
  }
  
  return 0;
  }*/

int read_vdimap(VDIFile *f, unsigned int vdimap[]){
  off_t offset = lseek(f->file, f->header.offsetBlocks, SEEK_SET);
  if (offset < 0) {
    cout << "Error! failed to seek vdimap" << endl;
    return 1;
  }
  int numMap = read(f->file, vdimap, 4*(f->header.blocksInHdd));
  if (numMap < 0){
    cout << "Error! failed to read vdimap" << endl;
    return 1;
  }
  return 0;
}

int read_MBR(VDIFile *f, BootSector &boot){
  off_t offset = lseek(f->file, f->header.offsetData, SEEK_SET);
  if (offset < 0){
    cout << "Error! failed to seek to MBR" << endl;
    return 1;
  }
  int numMBR = read(f->file, &boot, sizeof(boot));
  if (numMBR != sizeof(boot)){
    cout << "Error! failed to read MBR" << endl;
    return 1;
  } 
  return 0;
}

int read_superblock(VDIFile *f, BootSector &boot, unsigned int vdimap[], ext2_super_block &super){
  unsigned int super_block_location = translate(1024, f, boot, vdimap);
  if (lseek(f->file, super_block_location, SEEK_SET) < 0){
    cout << "Error! failed to seek to the superblock!" << endl;
    return 1;
  }
  if (read(f->file, &super, sizeof(super)) != 1024){
    cout << "Error! failed to read superblock correctly!" << endl;
    return 1;

  }
  return 0;
}

unsigned int translate(unsigned int location, VDIFile *f, BootSector boot_sector, unsigned int vdimap[]){

  unsigned int part1 = f->header.offsetData;
  unsigned int part2 = boot_sector.partitionTable[0].sector_1*512 + location;
  unsigned int offset = part2%f->header.blockSize;
  unsigned int block = part2/f->header.blockSize;
  unsigned int translation = vdimap[block];
  unsigned int value = part1+translation*f->header.blockSize + offset;
  return value;
}

int read_group_descriptor(VDIFile *f, BootSector boot_sector, unsigned int vdimap[], unsigned int block_size, ext2_group_descriptor group_descriptor[], unsigned int block_group_count){

  unsigned int start = 0;
  if (block_size == 1024) start = 2*block_size;
  else start = 1*block_size;

  unsigned int location = translate(start, f, boot_sector, vdimap);
  if (lseek(f->file, location, SEEK_SET) < 0) {
    cout << "Error! Cannot seek to group descriptor table!" << endl;
    return 1;
  }
  if (read(f->file, group_descriptor, sizeof(ext2_group_descriptor)*block_group_count) != sizeof(ext2_group_descriptor)*block_group_count) {
    cout << "Error! Cannot read group descriptor!" << endl;
    return 1;
  }

  return 0;
  
}

unsigned char* read_bitmap(unsigned int block_size, unsigned int block_id, VDIFile *f, BootSector boot_sector, unsigned int vdimap[]) {

  unsigned char *bitmap;
  bitmap = (unsigned char *)malloc(block_size);
  lseek(f->file, translate(block_id*block_size, f, boot_sector, vdimap), SEEK_SET);
  read(f->file, bitmap, block_size);
  return bitmap;
}

ext2_inode read_inode(VDIFile *f, BootSector boot_sector, unsigned int vdimap[], unsigned int inode_count, unsigned int block_size, ext2_super_block super_block, ext2_group_descriptor group_descriptor[]){

  ext2_inode inode;
  inode_count--;

  unsigned int group_count = inode_count/super_block.s_inodes_per_group;
  unsigned int offset1 = inode_count % super_block.s_inodes_per_group;
  unsigned int inodes_per_block = block_size/sizeof(ext2_inode);
  unsigned int block_num = group_descriptor[group_count].inode_table + (offset1/inodes_per_block);
  unsigned int offset2 = inode_count % inodes_per_block;

  lseek(f->file, translate((block_num*block_size) + offset2*128, f, boot_sector, vdimap), SEEK_SET);
  
  read(f->file, &inode, 128);
  return inode;
}

bool get_dir_entry(ext2_dir_entry_2 &found, unsigned char *data_block, unsigned int size_diff, string fname, bool display) {

  ext2_dir_entry_2 *entry = (ext2_dir_entry_2 *) malloc (sizeof(ext2_dir_entry_2));
  memcpy(entry, data_block, sizeof(*entry));
  unsigned int entry_number = 0;
  unsigned int size = 0;

  while (size < size_diff) {
    char f_name[256];
    memcpy(f_name, entry->name, entry->name_len);
    f_name[entry->name_len] = '\0';
    if (entry->inode != 0) {
      if (display) cout << f_name << endl;
      else if ((string) f_name == fname) {
	found = *entry;
	free(entry);
	return true;
      }
    }
    else {
      size += sizeof(*entry);
      memcpy(entry, data_block + size, sizeof(*entry));
      continue;
    }
    size += entry->rec_len;
    memcpy(entry, data_block + size, sizeof(*entry));
  }
  free(entry);
  return false;
}

int read_block(ext2_inode inode, unsigned int block_num, unsigned int block_size, VDIFile * f, BootSector boot_sector, unsigned int vdimap[], unsigned char *buf){

  if (block_num*block_size >= inode.size) return -1;
  int direct, index_1, index_2, index_3;

  compute_index(block_num, block_size, direct, index_1, index_2, index_3);
  unsigned int direct_block_num = 0;
  unsigned int block_num_1 = 0;
  unsigned int block_num_2 = 0;
  unsigned int block_num_3 = inode.i_block[14];
  bool hole = false;

  if (index_3 != -1){
    if (block_num_3 == 0) hole = true;
    else {
      if (index_1 != -1 && index_2 != -1 && direct == -1){
	if (lseek(f->file, translate(block_num_3*block_size, f, boot_sector, vdimap), SEEK_SET) < 0) return -1;
	if (read(f->file, buf, block_size) < 0) return -1;
	if (index_3 >= (block_size/4)) return -1;
	block_num_2 = *(((unsigned int *) buf) + index_3);
      }
      else return -1;
    }
  }

  if (!hole && index_2 != -1){
    if (index_1 != -1 && direct == -1) {
      if (index_3 == -1) block_num_2 = inode.i_block[13];

      if (block_num_2 == 0) hole = true;

      else{
	if (lseek(f->file, translate(block_num_2*block_size, f, boot_sector, vdimap), SEEK_SET) < 0) return -1;
	if (read(f->file, buf, block_size) < 0) return -1;
	if (index_2 >= (block_size/4)) return -1;
	block_num_1 = *(((unsigned int *) buf) + index_2);
      }
    }
    else return -1;
    
  }

  if (!hole && index_1 != -1) {
    if (direct == -1) {
      if (index_2 == -1) block_num_1 = inode.i_block[12];

      if (block_num_1 == 0) hole = true;

      else{
	if (lseek(f->file, translate(block_num_1*block_size, f, boot_sector, vdimap), SEEK_SET) < 0) return -1;

	if (read(f->file, buf, block_size) < 0) return -1;

	if (index_1 >= (block_size/4))return -1;

	direct_block_num = *(((unsigned int *) buf) + index_1);
      }
      
    } else return -1;

  }

  if (!hole && direct != -1) {
    if (direct < 12) direct_block_num = inode.i_block[direct];
    else return -1;
  }

  if (hole) memset(buf, 0, block_size);
  else{
    if (lseek(f->file, translate(direct_block_num*block_size, f, boot_sector, vdimap), SEEK_SET) < 0) return -1;

    if (read(f->file, buf, block_size) < 0) return -1;
   
  }

  unsigned int difference = inode.size - (block_num*block_size);
  if (difference >= block_size) return block_size;
  return difference;
}

void compute_index(unsigned int block_num, unsigned int block_size, int &direct_number, int &indirect_index, int &index_2, int& index_3){

  if (block_num <= 11){
    direct_number = block_num;
    indirect_index = -1;
    index_2 = -1;
    index_3 = -1;
    return;
  }
  unsigned int blocks_rem = block_num - 12;

  if (blocks_rem < block_size/4) {
    direct_number = -1;
    index_2 = -1;
    index_3 = -1;
    indirect_index = blocks_rem;
    return;
  }
  blocks_rem -= block_size/4;

  if (blocks_rem < (block_size/4)*(block_size/4)){
    direct_number = -1;
    index_3 = -1;
    index_2 = blocks_rem/(block_size/4);
    indirect_index = blocks_rem - (index_2)*(block_size/4);
    return;
  }
  blocks_rem -= (block_size/4)*(block_size/4);

  if (blocks_rem < (block_size/4)*(block_size/4)*(block_size/4)){
    direct_number = -1;
    index_3 = blocks_rem/((block_size/4)*(block_size/4));
    index_2 = (blocks_rem - (index_3)*(block_size/4)*(block_size/4))/(block_size/4);
    indirect_index = (blocks_rem - (index_3)*(block_size/4)*(block_size/4)) - (index_2)*(block_size/4);
    return;
  }
}

  
