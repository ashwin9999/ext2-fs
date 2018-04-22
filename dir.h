#ifndef DIR
#define DIR

struct __attribute__ ((packed)) ext2_dir_entry_2{
  unsigned int inode;
  unsigned short rec_len;
  unsigned char name_len;
  unsigned char file_type;
  unsigned char name[225];

	};
#endif
