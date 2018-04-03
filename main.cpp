#include "vdi_read.h"
#include <iostream>
#include <string>
#include "vdifile.h"
#include <fstream>
#include "mbr.h"

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

  // Magic number is just a checkpoint for testing. We can comment this out now, since we know it works, so we don't mess with the cursor positionings. 

  /* offset = vdiSeek(file, 510+file->header.offsetData, SEEK_SET);
   numChar = vdiRead(file, &magic,2);

   cout << "magic: " << hex << magic << endl;*/

   int mapChar;
   unsigned int vdimap[file->header.blocksInHdd];
   mapChar = read_vdimap(file, vdimap);

   int numMBR;
   BootSector boot_sector;
   numMBR = read_MBR(file, boot_sector);
   if(numMBR == 1) return 1;

   ext2_super_block super_block;
   int numSuperBlock;
   numSuperBlock = read_superblock(file, boot_sector, vdimap, super_block);
   if(numSuperBlock == 1) return 1;

   cout << "s_magic: " << hex << super_block.s_magic << endl;


   
  return 0;
}
