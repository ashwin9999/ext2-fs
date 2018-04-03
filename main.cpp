#include "vdi_read.h"
#include <iostream>
#include <string>
#include "vdifile.h"
#include <fstream>
#include "mbr.h"
#include "vdiInfo.h"

using namespace std;

int main(int argc, char *argv[]){
  int vdifile;
  VDIFile *file = new VDIFile;
  vdifile = vdiOpen(file, argv[1]);
  off_t offset;
  ssize_t numChar;
  short int magic;
  offset = vdiSeek(file, 0, SEEK_SET);
  numChar = vdiRead(file, &(file->header), sizeof(file->header));
  display_vdihead(file);
  
   int mapChar;
   unsigned int vdimap[file->header.blocksInHdd];
   mapChar = read_vdimap(file, vdimap);
   display_vdimap(vdimap);

   int numMBR;
   BootSector boot_sector;
   numMBR = read_MBR(file, boot_sector);
   if(numMBR == 1) return 1;
   display_MBR(boot_sector);

   ext2_super_block super_block;
   int numSuperBlock;
   numSuperBlock = read_superblock(file, boot_sector, vdimap, super_block);
   if(numSuperBlock == 1) return 1;
   display_superblock(super_block);

   unsigned int group_count = (super_block.s_blocks_count- super_block.s_first_data_block) / super_block.s_blocks_per_group;
   unsigned int remainder = (super_block.s_blocks_count - super_block.s_first_data_block) % super_block.s_blocks_per_group;
   if (remainder > 0) group_count++;

   unsigned int block_size = 1024 << super_block.s_log_block_size;
   ext2_group_descriptor group_descriptor[group_count];
   if (read_group_descriptor(file, boot_sector, vdimap, block_size, group_descriptor, group_count) == 1) return 1;


   
  return 0;
}
