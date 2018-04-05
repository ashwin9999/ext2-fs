#ifndef VDI_READ
#define VDI_READ
#include <cstdint>
#include <string>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "vdifile.h"
#include "mbr.h"
#include "superblock.h"
#include "group_descriptor.h"
#include "inode.h"
#include "dir.h"

using namespace std;

int vdiOpen(VDIFile *vdi, char *fn);
void vdiClose(VDIFile *f);
off_t vdiSeek(VDIFile *f, off_t offset, int whence);
ssize_t vdiRead(VDIFile *f, void *buf, ssize_t n);
ssize_t vdiWrite(VDIFile *f, void *buf, ssize_t n);
int read_vdimap(VDIFile *f, unsigned int vdimap[]);
int read_MBR(VDIFile *f, BootSector &boot);
int read_superblock(VDIFile *f, BootSector &boot, unsigned int vdimap[], ext2_super_block &super);
unsigned int translate(unsigned int location, VDIFile *f, BootSector boot_sector, unsigned int vdimap[]);
int read_group_descriptor(VDIFile *f, BootSector boot_sector, unsigned int vdimap[], unsigned int block_size,ext2_group_descriptor group_descriptor[], unsigned int block_group_count);
unsigned char* read_bitmap(unsigned int block_size, unsigned int block_id, VDIFile *f, BootSector boot_sector, unsigned int vdimap[]);
ext2_inode read_inode(VDIFile *f, BootSector boot_sector, unsigned int vdimap[], unsigned int inode_count, unsigned int block_size, ext2_super_block super_block, ext2_group_descriptor group_descriptor[]);
bool get_dir_entry(ext2_dir_entry_2 &found, unsigned char *data_block, unsigned int size_diff, string fname, bool display);



#endif
