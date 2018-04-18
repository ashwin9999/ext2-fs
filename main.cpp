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
     if (answer.compare(0, 5, "write") == 0){
       cout << endl;
       vector<string> split1;
       stringstream ss(answer);
       string item;
       while(getline(ss, item, ' ')) split1.push_back(item);
       if (split1.size() < 3){
	 cout << "Error! Not enough arguments." << endl;
	 continue;
       }
       string hostpath = split1[1];
       string ext2path = split1[2];

       vector<string> split2;
       stringstream ss2(ext2path);
       string item2;
       while(getline(ss2, item2, '/')) split2.push_back(item);
       if (split2.empty() || ext2path.compare(0, 1,"/") != 0) {
	 cout << "The ext2 path is not in a correct format" << endl;
	 continue;
       }
       if (split2[split2.size() - 1].empty()){
	 cout << "Please provide a file name for your file in the ext2 directory" << endl;
	 continue;
       }
       int fd = open(hostpath.c_str(), O_RDONLY);
       cout << "File name: " << hostpath.c_str() << endl;
       if (fd < 0) {
	 cout << "Cannot open the file in the host path!" << endl;
	 continue;
       }
       ext2_inode current_inode = read_inode(file, boot_sector, vdimap, current.inode, block_size, super_block, group_descriptor);
       ext2_dir_entry_2 current_dir;
       found = false;
       for(int i =0; i < root_size; i++) {
	 int difference;
	 difference = read_block(inode, i, block_size, file, boot_sector, vdimap, buf);
	 if (difference == -1) {
	   cout << "Error in displaying the file system - 4" << endl;
	   return 1;
	 }
	 if (get_dir_entry(current_dir, buf, difference, ".", false)) {
	   found = true;
	   break;
	 }
       }
       if (!found) {
	 cout << "Error in setting the current dir to the root dir" << endl;
	 return -1;
       }
       
       found = false;
       for (int j = 1; j < split2.size() - 1; j++){
	 unsigned int size = current_inode.size/block_size;
	 if(current_inode.size % block_size > 0) size++;
	 for (int k = 0; k < size; k++) {
	   int difference;
	   difference = read_block(current_inode, k, block_size, file, boot_sector, vdimap, buf);
	   if (difference == -1) {
	     cout << "Error in parsing the dir path" << endl;
	     return 1;
	   }
	   cout << "Split value: " << split2[j] << endl;
	   cout << "Difference: " << difference  << endl;
	   cout << "Current dir inode: " << current_dir.inode << endl;
	   if (get_dir_entry(current_dir, buf, difference, split2[j], false)) {
	     found = true;
	     break;
	   }
	 }

	 // cout << "difference: " << difference << endl;
	 
	 /* This is where it is breaking*/

	 
	 if (!found) {
	   cout << "Cannot locate the dir entry in ext2path" << endl;
	   return 1;
	 } else current_inode = read_inode(file, boot_sector, vdimap, current_dir.inode, block_size, super_block, group_descriptor);
       }

	 string name = split2[split2.size() - 1];
	 unsigned int new_dir_rec_len = 8 + name.length() + 1;
	 if (new_dir_rec_len % 4 > 0) new_dir_rec_len += 4 - (new_dir_rec_len % 4);

	 unsigned int dir_size = current_inode.size/block_size;
	 if (current_inode.size % block_size > 0) dir_size++;

	 unsigned char* last_dir_block = (unsigned char*)malloc(block_size);
	 int difference;
	 difference = read_block(current_inode, dir_size-1, block_size, file, boot_sector, vdimap, last_dir_block);
	 if (difference == -1){
	   cout << "Error in parsing the directory path" << endl;
	   return 1;
	 }
	 unsigned int blocks_for_dir_entry;
	 if (block_size - difference >= new_dir_rec_len) blocks_for_dir_entry = 0;
	 else {
	   int new_direct, old_direct, new_indirect, old_indirect, new_double, old_double, new_triple, old_triple;

	   compute_index(dir_size, block_size, new_direct, new_indirect, new_double, new_triple);
	   compute_index(dir_size -1, block_size, old_direct, old_indirect, old_double, old_triple);
	   if (new_triple != -1 && old_triple == -1) blocks_for_dir_entry = 4;
	   else if (new_triple != old_triple) blocks_for_dir_entry = 3;
	   else if (new_double != -1 && old_double == -1) blocks_for_dir_entry = 3;
	   else if (new_double != old_double) blocks_for_dir_entry = 2;
	   else if (new_indirect != -1 && old_indirect == -1) blocks_for_dir_entry = 2;
	   else if (new_indirect != old_indirect) blocks_for_dir_entry = 1;
	   else blocks_for_dir_entry = 1;						
	 }

	 unsigned int f_size = lseek(fd, 0, SEEK_END) + 1;
	 unsigned int f_block_num = f_size/block_size;
	 if (f_size % block_size > 0) f_block_num++;
	 unsigned int free_block_num = super_block.s_free_blocks_count - super_block.s_r_blocks_count;

	 int direct, indirect, double_, triple;
	 compute_index(f_block_num-1, block_size, direct, indirect, double_, triple);
	 unsigned int blocks_for_addresses;
	 if (direct != -1 && indirect == -1 && double_ == -1 && triple == -1) blocks_for_addresses = 0;
	 else if (direct == -1 && indirect != -1 && double_ == -1 && triple == -1) blocks_for_addresses = 1;
	 else if (direct == -1 && indirect != -1 && double_ != -1 && triple == -1) blocks_for_addresses = 1 + double_ + 1 + 1;
	 else if (direct == -1 && indirect != -1 && double_ != -1 && triple != -1) blocks_for_addresses = 1 + triple + 1 + triple*(block_size/4) + double_ + 1 + 1 + (block_size/4) + 1;
    
       if (f_block_num + blocks_for_addresses + blocks_for_dir_entry > free_block_num) {
	 cout << "Ext2 file system cannot fit the host file" << endl;
	 return 1;
       }

       if (super_block.s_free_inodes_count <= 0) {
	 cout << "Error, there isn't enough room to write a new inode" << endl;
	 return 1;
       }

       unsigned int inode_address = 0;
       int i = 0;
       for (i = 0; i < group_count; i++){
	 unsigned char *bitmap = read_bitmap(block_size, group_descriptor[i].inode_bitmap, file, boot_sector, vdimap);
	 unsigned int inode_offset = i*super_block.s_inodes_per_group;

	 if (is_inode_free(bitmap, block_size, inode_offset, inode_address)) {
	   if (write_bitmap(file, boot_sector, vdimap, bitmap, block_size, group_descriptor[i].inode_bitmap) == 1) {
	     cout << "Error in writing the inode bitmap " << endl;
	     return 1;
	   }
	   group_descriptor[i].free_inodes_count--;
	   break;
	 }
       }

	 vector <unsigned int> addresses;
	 for (i; i < f_block_num + blocks_for_addresses + blocks_for_dir_entry; i++) {
	   unsigned int address;
	   for (int j = 0; j < group_count; j++){
	     unsigned char *bitmap = read_bitmap(block_size, group_descriptor[j].block_bitmap, file, boot_sector, vdimap);
	     unsigned int block_offset = j*super_block.s_blocks_per_group;

	     if (is_block_free(bitmap, block_size, block_offset, address)) {
	       addresses.push_back(address);
	       if (write_bitmap(file, boot_sector, vdimap, bitmap, block_size, group_descriptor[j].block_bitmap) == 1) {
		 cout << "Error in writing the block bitmap" << endl;
		 return 1;
	       }
	       group_descriptor[j].free_blocks_count--;
	       break;
	     }   
	   }
	 }

	 ext2_inode new_inode = read_inode(file, boot_sector, vdimap, 12, block_size, super_block, group_descriptor);
	 new_inode.mode = 0x8000 | 0x0100 | 0x0080 | 0x0040;
	 new_inode.uid = 1000;
	 new_inode.size = f_size;
	 new_inode.gid = 1000;
	 new_inode.links_count = 1;
	 new_inode.blocks = f_size/512;
	 if (f_size % 512 > 0) new_inode.blocks++;
	 if (direct != -1 && indirect == -1 && double_ == -1 && triple == -1) {
	   for (i = 0; i <= 14; i++){
	     if (i < addresses.size()) {
	       new_inode.i_block[i] = addresses.back();
	       addresses.pop_back();
	     } else new_inode.i_block[i] = 0;
	   }
	 }
	 else if (direct == -1 && indirect != -1 && double_ == -1 && triple == -1) {
	   for (i =0; i <= 14; i++) {
	     if (i <= 12) {
	       new_inode.i_block[i] = addresses.back();
	       addresses.pop_back();
	     } else new_inode.i_block[i] = 0;
	   }
	 }
	 else if (direct == -1 && indirect != -1 && double_ != -1 && triple == -1) {
	   for (i = 0; i <= 13; i++) {
	     new_inode.i_block[i] = addresses.back();
	     addresses.pop_back();
	   }
	   new_inode.i_block[14] = 0;
	 }
	 else if (direct == -1 && indirect != -1 && double_ != -1 && triple != -1) {
	   for (i = 0; i <= 14; i++){
	     new_inode.i_block[i] = addresses.back();
	     addresses.pop_back();
	   }
	 }

	 for (i = 0; i < f_block_num; i++) {
	   if (lseek(fd, i*block_size, SEEK_SET) < 0) {
	     cout << "Couldn't seek the host file" << endl;
	     return 1;
	   }
	   if (read(fd, buf, block_size) < 0) {
	     cout << "Failed to read a block of data from host file" << endl;
	     return 1;
	   }
	   int difference = write_block(file, boot_sector, vdimap, new_inode, addresses, i, block_size, buf);
	   if(difference == -1) {
	     cout << "Error in writing a block to the ext2 filesystem" << endl;
	     return 1;
	   }
	 }
	 if (write_inode(file, boot_sector, vdimap, new_inode, inode_address, block_size, super_block, group_descriptor) == 1){
	   cout << "Error in writing inode" << endl;
	   return 1;
	 }

	 ext2_dir_entry_2 new_dir_entry;
	 new_dir_entry.inode = inode_address;
	 new_dir_entry.name_len = name.length() + 1;
	 new_dir_entry.rec_len = new_dir_rec_len;
	 new_dir_entry.file_type = 1;
	 memcpy(new_dir_entry.name, name.c_str(), name.size());
	 new_dir_entry.name[name.length()] = '\0';
	 current_inode.size += new_dir_entry.rec_len;
	 vector<unsigned int> empty;
	 if (block_size - difference >= new_dir_entry.rec_len) {
	   memcpy(last_dir_block + difference, &new_dir_entry, new_dir_entry.rec_len);
	   if (write_block(file, boot_sector, vdimap, current_inode, empty, dir_size - 1, block_size, last_dir_block) == -1){
	     cout << "Error in writing the data lock" << endl;
	     return 1;
	   }
	 }else{
	   unsigned char *new_block = (unsigned char*) malloc(block_size);
	   memset(new_block, 0, block_size);
	   memcpy(new_block, &new_dir_entry, new_dir_entry.rec_len);

	   int new_direct_num, old_direct_num;
	   int new_indirect_index, old_indirect_index;
	   int new_double_index, old_double_index;
	   int new_triple_index, old_triple_index;

	   compute_index(dir_size, block_size, new_direct_num, new_indirect_index, new_double_index, new_triple_index);
	   compute_index(dir_size-1, block_size, old_direct_num, old_indirect_index, old_double_index, old_triple_index);
	   if (new_triple_index != -1 && old_triple_index == -1) {
	     current_inode.i_block[14] = addresses.back();
	     addresses.pop_back();
	   } else if (new_double_index != -1 && old_double_index == -1 && new_triple_index == -1) {
	     current_inode.i_block[13] = addresses.back();
	     addresses.pop_back();
	   } else if (new_indirect_index != -1 && old_indirect_index == -1 && new_double_index == -1) {
	     current_inode.i_block[12] = addresses.back();
	     addresses.pop_back();
	   } else if (new_direct_num != old_direct_num) {
	     current_inode.i_block[new_direct_num] = addresses.back();
	     addresses.pop_back();
	   }

	   if (write_block(file, boot_sector, vdimap, current_inode, addresses, dir_size, block_size, new_block) == -1) {
	     cout << "Error writing the data block with the new dir entry" << endl;
	     return 1;
	   }
	   free(new_block);
	 }
	 free(last_dir_block);

	 if (!addresses.empty()){
	   cout << "All of the allocated addresses were not used" << endl;
	   return 1;
	 }

	 if (write_inode(file, boot_sector, vdimap, current_inode, current_dir.inode, block_size, super_block, group_descriptor) == 1) {
	   cout << "Error in writing the dir inode" << endl;
	   return 1;
	 }
	 if (write_group_descriptor(file, boot_sector, vdimap, block_size, group_descriptor, group_count) == 1) {
	   cout << "Error in writing the group descriptor" << endl;
	   return 1;
	 }

	 super_block.s_free_blocks_count -= (f_block_num + blocks_for_addresses+ blocks_for_dir_entry);
	 super_block.s_free_inodes_count--;

	 if (write_superblock(file, boot_sector, vdimap, super_block) == 1){
	   cout << "Error writing the super block" << endl;
	   return 1;
	 }

	 cout << "File " + hostpath + " successfully written into the ext2 file system at " + ext2path + "." << endl;
       }
     cout << "==================================" << endl;
     cout << path << endl;
   }
     //close(file->file);
     //free(buf);
     
  return 0;
}
