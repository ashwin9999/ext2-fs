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
  return 0;
}
