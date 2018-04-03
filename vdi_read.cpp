#include "vdi_read.h"

using namespace std;

int vdiOpen(VDIFile *vdi, char *fn){
  vdi->file = open(fn, O_RDWR);
  vdi->cursor = 0;
  if(vdi->file < 0) return 1;
  return vdi->file;
}

void vdiClose(VDIFile *f){
  close(f->file);
}

off_t vdiSeek(VDIFile *f, off_t offset, int whence){
  off_t location;
  if (whence == SEEK_SET){
    location= lseek(f->file, offset, whence);
    if(location < 0){
      cout << "Error seeking the vdi header" << endl;
      return 1;
    }
    f->cursor = location;
  }
  if (whence == SEEK_CUR){
    location = lseek(f->file, offset, whence);
    if (location < 0){
      cout << "Error seeking the vdi header" << endl;
      return 1;
    }
    f->cursor += offset;
  }
  if(whence == SEEK_END){
    location = lseek(f->file, offset, whence);
    if (location < 0){
      cout << "Error seeking the vdi header" << endl;
      return 1;
    }
    f->cursor = offset + f->fileSize;
  }
  return f->cursor;
}

ssize_t vdiRead(VDIFile *f, void *buf, ssize_t n){
  //cout << "value of buf before reading: " << buf << endl;
  ssize_t numBytes = read(f->file, buf, n);
  //cout << "Cursor " << f->cursor << endl;
  if(numBytes != n) {
    cout << "Error header not read correctly" << endl;
    return 1;
  }
  return 0;
}

/*ssize_t vdiWrite(VDIFile *f, void *buf, ssize_t n){
  f->fileStructure.write(buf, n);
  if(f->fileStructure){
    return n;
  }
  else{
    return -1;
  }
  
  return 0;
  }*/

int read_vdimap(VDIFile *f, unsigned int vdimap[]){
  off_t offset = lseek(f->file, f->header.offsetBlocks, SEEK_SET);
  if (offset < 0) {
    cout << "Error! failed to seek vdimap" << endl;
    return 1;
  }
  int numMap = read(f->file, vdimap, 4*(f->header.blocksInHdd));
  if (numMap < 0){
    cout << "Error! failed to read vdimap" << endl;
    return 1;
  }
  return 0;
}

int read_MBR(VDIFile *f, BootSector &boot){
  off_t offset = lseek(f->file, f->header.offsetData, SEEK_SET);
  if (offset < 0){
    cout << "Error! failed to seek to MBR" << endl;
    return 1;
  }
  int numMBR = read(f->file, &boot, sizeof(boot));
  if (numMBR != sizeof(boot)){
    cout << "Error! failed to read MBR" << endl;
    return 1;
  } 
  return 0;
}

int read_superblock(VDIFile *f, BootSector &boot, unsigned int vdimap[], ext2_super_block &super){
  unsigned int super_block_location = translate(1024, f, boot, vdimap);
  if (lseek(f->file, super_block_location, SEEK_SET) < 0){
    cout << "Error! failed to seek to the superblock!" << endl;
    return 1;
  }
  if (read(f->file, &super, sizeof(super)) != 1024){
    cout << "Error! failed to read superblock correctly!" << endl;
    return 1;

  }
  return 0;
}

unsigned int translate(unsigned int location, VDIFile *f, BootSector boot_sector, unsigned int vdimap[]){

  unsigned int part1 = f->header.offsetData;
  unsigned int part2 = boot_sector.partitionTable[0].sector_1*512 + location;
  unsigned int offset = part2%f->header.blockSize;
  unsigned int block = part2/f->header.blockSize;
  unsigned int translation = vdimap[block];
  unsigned int value = part1+translation*f->header.blockSize + offset;

  return value;
}




  
