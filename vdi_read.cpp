#include "vdi_read.h"
using namespace std;


/**
 * Open the file and return a pointer to a VDI file structure
 */

VDIFile *vdiOpen(char *fn){
  VDIFile *file;
  /*file->cursor = open(fn, O_RDWR); //Open file in read/write mode. LINUX system call
  if (file->cursor < 0){
    cout << "Cannot open the file" << endl;
    }*/
  file->fileStructure.open(fn, ios::in | ios::out | ios::binary);
  //file->cursor = 0;
  if ((file->fileStructure.rdstate() & std::ifstream::failbit ) != 0 )
    std::cerr << "Error opening 'test.txt'\n";
  return file;
}

/**
 * Close the VDIFile 
 */

void vdiClose(VDIFile *f){
  close(f->cursor);
}

/**
 * Mimic the lseek() system call
 */

off_t vdiSeek(VDIFile *f, off_t offset, int whence){
  off_t result = lseek(f->cursor, offset, whence);
  if (result < 0){
    return -1;
  }
  f->cursor = result;
  return result;
}

/**
 * Mimic the read() system call (reading from an unallocated page returns zero in all bytes read from the page
 */

ssize_t vdiRead(VDIFile *f, void *buf, ssize_t n){
  /*cout << "f cursor " << f->cursor << endl;
  ssize_t result = read(f->cursor, &buf, n);
  cout << errno;
  cout << "result after read " << result << endl;
  if (result != n){
    return -1;
    }*/
  char *buffer = new char[n];
  cout << "n " << n << endl;
  f->fileStructure.read(buffer,int(n));
  cout << "n " << n << endl;
  buf = buffer;
  return 1;
}


/**
 * Mimic the write() system call (unallocated pages must be allocated before writing)
 */


ssize_t vdiWrite(VDIFile *f, void *buf, ssize_t n){
  
}


