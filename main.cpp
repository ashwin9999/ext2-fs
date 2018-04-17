#include "vdi_read.h"
#include <iostream>
#include <string>
#include "vdifile.h"
#include <fstream>
#include <stack>
#include <sstream>
#include <vector>
#include "mbr.h"
#include "vdiInfo.h"
#include "vdi_write.h"

using namespace std;

int main(int argc, char *argv[]){
  int vdifile;
  VDIFile *file = new VDIFile;
  vdifile = vdiOpen(file, argv[1]);
  off_t offset;
  ssize_t numChar;
  short int magic;

  /**
   * Reads the vdiheader
   */
  offset = vdiSeek(file, 0, SEEK_SET);
  numChar = vdiRead(file, &(file->header), sizeof(file->header));

  /**
   * Reads the vdimap
   */  
   int mapChar;
   unsigned int vdimap[file->header.blocksInHdd];
   mapChar = read_vdimap(file, vdimap);

   /**
    * Reads the MBR
    */
   int numMBR;
   BootSector boot_sector;
   numMBR = read_MBR(file, boot_sector);
   if(numMBR == 1) return 1;

   /**
    * Reads the superblock
    */
   ext2_super_block super_block;
   int numSuperBlock;
   numSuperBlock = read_superblock(file, boot_sector, vdimap, super_block);
   if(numSuperBlock == 1) return 1;

   /**
    * Reads the group descriptor
    */
   unsigned int group_count = (super_block.s_blocks_count- super_block.s_first_data_block) / super_block.s_blocks_per_group;   
   unsigned int remainder = (super_block.s_blocks_count - super_block.s_first_data_block) % super_block.s_blocks_per_group;
   if (remainder > 0) group_count++;
   unsigned int block_size = 1024 << super_block.s_log_block_size;
   ext2_group_descriptor group_descriptor[group_count];
   if (read_group_descriptor(file, boot_sector, vdimap, block_size, group_descriptor, group_count) == 1) return 1;
 
   /**
    * Reads the inode
    */
   ext2_inode inode = read_inode(file, boot_sector, vdimap, 2, block_size, super_block, group_descriptor);

   /**
    * Fetching directories from the inode
    */
   string path = "Path: /";
   unsigned char *buf = (unsigned char*) malloc(block_size);
   ext2_dir_entry_2 current;
   bool found = false;

   unsigned int root_size = inode.size/block_size;
   if(inode.size % block_size > 0) root_size++;

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
   cout << "Enter ls to view the list of files" << endl;
   cout << "==================================" << endl;

   stack<int> *dirname_length = new stack<int>;
   bool cont = true;
   string answer;

   while(cont){
     getline(cin, answer);
     if (answer == "view"){
       display_vdihead(file);
       display_vdimap(vdimap);
       display_MBR(boot_sector);
       display_superblock(super_block);
       display_inode(inode);
     }
     if (answer == "help"){
       cout << endl;
       cout << "To change a directory -- cd [dirname]" << endl;
       cout << "To move one directory up -- cd .." << endl;
       cout << "To display files within the current directory -- ls" <<endl;
       cout << "To view the individual parts of a vdi file -- view" << endl;
       cout << endl;
     }
     if (answer == "cd .."){
       cout << endl;
       if (current.inode != 2){
	 ext2_inode inode = read_inode(file, boot_sector, vdimap, current.inode, block_size, super_block, group_descriptor);
	 char fname[256];
	 memcpy(fname, current.name, current.name_len);
	 fname[current.name_len] = '\0';
	 if ((string) fname != "..") dirname_length->push((int)current.name_len);
	 unsigned int f_size = inode.size/block_size;
	 if (inode.size%block_size > 0) f_size++;
	 found = false;
	 for (int i =0; i < f_size; i++){
	   int difference;
	   difference = read_block(inode, i, block_size, file, boot_sector, vdimap, buf);
	   if (difference == -1){
	     cout << "Error in displaying file system! -cd .." << endl;
	     return 1;
	   }
	   if (get_dir_entry(current, buf, difference, "..", false)){
	     found = true;
	     break;
	   }
	 }
	   if (!found) cout << "The directory couldnt be located cd .." << endl;
	   else{
	     path = path.substr(0,path.length()-dirname_length->top()-1);
	     dirname_length->pop();
	   }
	 } else cout << "You are at the root!" << endl;
	 
       cout << path << endl;
     }
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
     if  (answer.compare(0,3,"cd ") == 0) {
       cout << endl;
       ext2_inode new_inode_2 = read_inode(file, boot_sector, vdimap, current.inode, block_size, super_block, group_descriptor);
       string dir = answer.substr(3, answer.length()-1);
       char fname[256];
       memcpy(fname, current.name, current.name_len);
       if ((string)fname != "..") dirname_length->push((int) current.name_len);
       ext2_dir_entry_2 new_dir;
       unsigned int f_size = new_inode_2.size/block_size;
       if (new_inode_2.size%block_size > 0) f_size++;
       found = false;
       for (int i = 0; i < f_size; i++){
	 int difference;
	 difference = read_block(new_inode_2, i, block_size, file, boot_sector, vdimap, buf);
	 if (difference == -1) {
	   cout << "Error displaying the file system! - 3" << endl;
	   return 1;
	 }
	 if (get_dir_entry(new_dir, buf, difference, dir, false)){
	   found = true;
	   break;
	 }
       }
       if (!found) cout << "The directory cannot be located!" << endl;
       else {
	 if ((int) new_dir.file_type == 2) {
	   current = new_dir;
	   path += dir + "/";
	 }
	 else cout << "The name you entered is not a directory file!" << endl;
       }
     }
     if (answer.compare(0,4,"read") == 0){
       cout << endl;
       stringstream ss(answer);
       vector<string> elements; 
       string item;
       while(getline(ss, item, ' ')) elements.push_back(item);
       vector<string> split1 = elements;
       if (split1.size() < 3) {
	 cout << "Error, there are fewer arguments than required!" << endl;
	 continue;
       }
       string ext2path = split1[1];
       string hostpath = split1[2];

       stringstream ss2(ext2path);
       vector<string> elements2;
       string item2;
       while(getline(ss2, item2, '/')) elements2.push_back(item2);
       vector<string> split2 = elements2;

       ext2_inode current_inode = read_inode(file, boot_sector, vdimap, current.inode, block_size, super_block, group_descriptor);
       ext2_dir_entry_2 curr_dir;

       if(split2.size() < 2){
	 cout << "The path for ext2 is not in a correct format!" << endl;
	 continue;
       }
       found = false;
       for (int j= 1; j < split2.size(); j++) {
	 unsigned int fsize = current_inode.size/block_size;
	 if (current_inode.size % block_size > 0) fsize++;
	 for (int i = 0; i < fsize; i++){
	   int difference;
	   difference = read_block(current_inode, i, block_size, file, boot_sector, vdimap,  buf);
	   if (difference == -1) {
	     cout << "Error in parsing the path for directory!" << endl;
	     return 1;
	   }
	   if (get_dir_entry(curr_dir, buf, difference, split2[j], false)){
	     found = true;
	     break;
	   } 
	 }
	 
	 if (!found) {
	   cout << "Cannot find the directory entry in ext2path" << endl;
	   return 1;
	 }
	 else{
	   current_inode = read_inode(file, boot_sector, vdimap, curr_dir.inode, block_size, super_block, group_descriptor);
	 }
       }
       if (hostpath.empty()){
	 cout << "Error, please enter a host path" << endl;
	 continue;
       }

       int fd = open(hostpath.c_str(), O_CREAT | O_WRONLY | O_APPEND, S_IRWXU);
       if (fd < 0) {
	 cout << "The file cannot be created in the host path" << endl;
	 continue;
       }
       if ((int) curr_dir.file_type != 1) {
	 cout << "The file in the ext2path cannot be read" << endl;
	 continue;
       }
       unsigned int file_size = current_inode.size/block_size;
       if(current_inode.size % block_size > 0) file_size++;

       for (int i = 0; i < file_size; i++){
	 int difference = read_block(current_inode, i, block_size,file,boot_sector, vdimap, buf);
	 if (difference == -1) {
	   cout << "Error in parsing the path of the directory! " << endl;
	   return 1;
	 }
	 if(i+1 == file_size) write(fd, buf, difference);
	 else write(fd, buf, block_size);
       }
       cout << "File " + ext2path + " has been read correctly to " + hostpath + "." << endl;
       close(fd);
     }
     cout << "==================================" << endl;
     cout << path << endl;
     //close(file->file);
     //free(buf);
   }
  return 0;
}
