#include "vdiInfo.h"
using namespace std;

void display_vdihead(VDIFile *f){
  cout << "Header information" << endl;
  cout << "======================" << endl;
  cout<<"Disk Image: "<< f->header.diskImage<<endl;
  cout<<"Image Signature: "<<hex<< f->header.imageSignature <<endl;
  cout<<"Version: "<< f->header.version <<endl;
  cout<<"Header Size: "<< f->header.headerSize<<endl;
  cout<<"Image Type: "<< f->header.imageType<<endl;
  cout<<"Image Flags: "<<dec<< f->header.imageFlags<<endl;
  cout<<"Image Descriptor: "<< f->header.imageDescription<<endl;
  cout<<"Offset Blocks: "<< f->header.offsetBlocks<<endl;
  cout<<"Offset Data: "<< f->header.offsetData<<endl;
  cout<<"Cylinders: "<< f->header.cylinders<<endl;
  cout<<"Heads: "<< f->header.heads<<endl;
  cout<<"Sector: "<< f->header.sectors<<endl;
  cout<<"Sector Size: "<< f->header.sectorSize<<endl;
  cout<<"Disk Size: "<< f->header.diskSize<<endl;
  cout<<"Block Size: "<< f->header.blockSize<<endl;
  cout<<"Extra block data: "<< f->header.blockExtraData<<endl;
  cout<<"Blocks in HDD: "<< f->header.blocksInHdd<<endl;
  cout<<"Allocated blocks: "<< f->header.blocksAllocated<<endl;
  cout << "===============================" << endl;
}

void display_vdimap(unsigned int vdimap[]){
  cout << "VDI MAP" << endl;
  cout << "============" << endl;
  for(int i=0; i < 128; i++){
    cout << vdimap[i] << " ";
    if (i == 127) cout << endl;
  }
  cout << "============" << endl;
}

void display_MBR(BootSector boot){
  cout << "MBR" << endl;
  cout << "======" << endl;
  cout << hex << "bs_magic: 0x" << boot.magic << endl;
  cout << dec << "partition table 0 Sector 1: " << boot.partitionTable[0].sector_1 << endl;
  cout << "======================" <<endl;
}

void display_superblock(ext2_super_block super){
  cout<<"Superblock Information"<<endl;
  cout << "========================" << endl;
  cout<<"Inodes count: "<<super.s_inodes_count<<endl;
  cout<<"Blocks count: "<<super.s_blocks_count<<endl;
  cout<<"Reserved blocks count: "<<super.s_r_blocks_count<<endl;
  cout<<"Free blocks count: "<<super.s_free_blocks_count<<endl;
  cout<<"Free inodes count: "<<super.s_free_inodes_count<<endl;
  cout<<"First Data Block: "<<super.s_first_data_block<<endl;
  cout<<"Block size: "<<super.s_log_block_size<<endl;
  cout<<"Fragment size: "<<super.s_log_frag_size<<endl;
  cout<<"Blocks per group: "<<super.s_blocks_per_group<<endl;
  cout<<"Number of fragments per group: "<<super.s_frags_per_group<<endl;
  cout<<"Number of inodes per group: "<<super.s_inodes_per_group<<endl;
  cout<<"Mount time: "<<super.s_mtime<<endl;
  cout<<"Write time: "<<super.s_wtime<<endl;
  cout<<"Mount count: "<<super.s_mnt_count<<endl;
  cout<<"Maximal mount count: "<<super.s_max_mnt_count<<endl;
  cout<<"Magic Number: 0x"<< hex << super.s_magic<<endl;
  cout<<"File system state: "<< dec <<super.s_state<<endl;
  cout<<"Behaviour when detecting errors: "<<super.s_errors<<endl;
  cout<<"Minor revision level: "<<super.s_minor_rev_level<<endl;
  cout<<"Time of last check: "<<super.s_lastcheck<<endl;
  cout<<"Maximum time between checks: "<<super.s_checkinterval<<endl;
  cout<<"OS: "<<super.s_creator_os<<endl;
  cout<<"Revision level: "<<super.s_rev_level<<endl;
  cout<<"Default uid for reserved blocks: "<<super.s_def_resuid<<endl;
  cout<<"Default gid for reserved blocks: "<<super.s_def_resgid<<endl;
  cout << "===============================" << endl;
}

void display_group_descriptor(ext2_group_descriptor group[], unsigned int count){
  cout << "Group descriptor table" <<endl;
  cout << "======================" << endl;
  for (int i=0; i <count; i++){
    cout << "Block bitmap: " << group[i].block_bitmap << endl;
    cout << "Inode bitmap: " << group[i].inode_bitmap << endl;
    cout << "Inode table: " << group[i].inode_table << endl;
  }
  cout << "======================" << endl;
  cout << endl;
}

void display_inode(ext2_inode inode){

  cout << "Inode information" << endl;
  cout << "====================" << endl;
  cout << "i_size: " << dec << inode.size << endl;
  cout << "i_mode: " << oct << inode.mode << endl;
  cout << "uid: " << dec << inode.uid << endl;
  cout << "====================" << endl;
}
