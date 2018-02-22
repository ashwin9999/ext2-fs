#include "headers/vdi_read.h"

using namespace std;


/**
 * Open the file and return a pointer to a VDI file structure
 */

int vdiOpen(char *fn){
  int vdifile = open(fn,O_RDWR); //Open file in read/write mode. LINUX system call
  if (vdifile < 0) return 1; 
  return vdifile;
}

/**
 * Close the VDIFile 
 */

void vdiClose(int f){
  close(f);
}

/**
 * Mimic the lseek() system call
 */

off_t vdiSeek(int f, off_t offset, int whence){



}

/**
 * Mimic the read() system call (reading from an unallocated page returns zero in all bytes read from the page
 */

ssize_t vdiRead(int f, void *buf, ssize_t n){
  

}


/**
 * Mimic the write() system call (unallocated pages must be allocated before writing)
 */


ssize_t vdiWrite(int f, void *buf, ssize_t n){

}
