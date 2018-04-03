#ifndef GROUPDESCRIPTOR
#define GROUPDESCRIPTOR

struct __attribute__((packed)) ext2_group_descriptor{
  unsigned int block_bitmap;
  unsigned int inode_bitmap;
  unsigned int inode_table;
  unsigned short free_blocks_count;
  unsigned short free_inodes_count;
  unsigned short used_dirs_count;
  unsigned short pad;
  unsigned int reserved[3];
};

#endif
