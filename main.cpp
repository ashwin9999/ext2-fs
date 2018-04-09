#include "vdi_read.h"
#include <iostream>
#include <string>
#include "vdifile.h"
#include <fstream>
#include <stack>
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
  //display_vdihead(file);
  
   int mapChar;
   unsigned int vdimap[file->header.blocksInHdd];
   mapChar = read_vdimap(file, vdimap);
   //display_vdimap(vdimap);

   int numMBR;
   BootSector boot_sector;
   numMBR = read_MBR(file, boot_sector);
   if(numMBR == 1) return 1;
   //display_MBR(boot_sector);

   ext2_super_block super_block;
   int numSuperBlock;
   numSuperBlock = read_superblock(file, boot_sector, vdimap, super_block);
   if(numSuperBlock == 1) return 1;
   //display_superblock(super_block);

   unsigned int group_count = (super_block.s_blocks_count- super_block.s_first_data_block) / super_block.s_blocks_per_group;   
   unsigned int remainder = (super_block.s_blocks_count - super_block.s_first_data_block) % super_block.s_blocks_per_group;
   if (remainder > 0) group_count++;
   unsigned int block_size = 1024 << super_block.s_log_block_size;
   ext2_group_descriptor group_descriptor[group_count];
   if (read_group_descriptor(file, boot_sector, vdimap, block_size, group_descriptor, group_count) == 1) return 1;
   //display_group_descriptor(group_descriptor, group_count);
   
   /***  All good until here **/


   ext2_inode inode = read_inode(file, boot_sector, vdimap, 2, block_size, super_block, group_descriptor);
 
   //cout << "inode-size "  << inode.size << endl;
   //display_inode(inode);


   /*
    * Fetching directories from the inode
    */

   //display_file_system();
   
   string path = "Path: /";
   unsigned char *buf = (unsigned char*) malloc(block_size);
   ext2_dir_entry_2 current;
   bool found = false;

   unsigned int root_size = inode.size/block_size;
   if(inode.size%block_size > 0) root_size++;

   for(int i =0; i < root_size; i++){
     int difference;
     difference = read_block(inode, i, block_size, file, boot_sector, vdimap, buf);
     if (difference == -1) {
       cout << "Error in displaying the file system! - 1" << endl;
       return 1;
     }
     if (get_dir_entry(current, buf, difference, ".", false)){
       found = true;
       break;
     }
   }

   if (!found){
     cout << "Error in displaying the file system! - 2" << endl;
     return 1;
   }

   cout << path << endl;

   stack<int> *dirname_length = new stack<int>;
   bool cont = true;
   string answer;

   while(cont){
     getline(cin, answer);
     if (answer == "ls"){
       cout << endl;
       ext2_inode new_inode = read_inode(file, boot_sector, vdimap, current.inode, block_size, super_block, group_descriptor);

       unsigned int file_size = inode.size/block_size;
       if (inode.size % block_size > 0) file_size++;

       for (int i = 0; i < file_size; i++){
	 int diff;
	 diff = read_block(new_inode, i, block_size, file, boot_sector, vdimap, buf);

	 if (diff == -1) {
	   cout << "Error displaying the file!" << endl;
	   return 1;
	 }
	 get_dir_entry(current, buf, diff, "", true);
       }
     }
     close(file->file);
     free(buf);
   }
  return 0;
}
