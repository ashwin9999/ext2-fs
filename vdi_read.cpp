#include "headers/vdi_read.h"

using namespace std;


/**
 * Open the file and return a pointer to a VDI file structure
 */

VDIFile *vdiOpen(char *fn){

}

/**
 * Close the VDIFile 
 */

void vdiClose(VDIFile *f){


}

/**
 * Mimic the lseek() system call
 */

off_t vdiSeek(VDIFile *f, off_t offset, int whence){



}

/**
 * Mimic the read() system call (reading from an unallocated page returns zero in all bytes read from the page
 */

ssize_t vdiRead(VDIFile *f, void *buf, ssize_t n){


}


/**
 * Mimic the write() system call (unallocated pages must be allocated before writing)
 */


ssize_t vdiWrite(VDIFile *f, void *buf, ssize_t n){

}
