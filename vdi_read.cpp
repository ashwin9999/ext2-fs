#include "vdi_read.h"
using namespace std;


/**
 * Open the file and return a pointer to a VDI file structure
 */

VDIFile *vdiOpen(char *fn){
  VDIFile *file;
  file->cursor = open(fn, O_RDWR); //Open file in read/write mode. LINUX system call
  if (file->cursor < 0){
    cout << "Cannot open the file" << endl;
  }
  cout << "olpen call done"<<endl;
  //file->fileStructure.open(fn, ios::in | ios::out | ios::binary);
  //file->fileStructure.open(fn);
  file->cursor = 0;
  //if ((file->fileStructure.rdstate() & std::ifstream::failbit ) != 0 )
    //std::cerr << "Error opening 'test.txt'\n";
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
  //off_t result = lseek(f->cursor, offset, whence);
  //if (result < 0){
  //  return -1;
  //}
  off_t result;

  if(whence == SEEK_SET){
    result = offset;
  }
  else if(whence == SEEK_CUR){
    result = f->cursor + offset;
  }
  else if(whence == SEEK_END){
    // TODO:Implement later if needed
  }
  cout << "Result in seek: " << result << endl;
  //f->fileStructure.seekg((int)result);
  f->cursor = result;
  f->fileStructure.clear();
  cout<< "cleared"<<endl;
  f->fileStructure.seekg(result);
  cout << "set vars"<<endl;
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
  //char *buffer = new char[n];
  char buffer[100];
  cout << "n " << n << endl;
  //f->fileStructure.seekg(f->cursor);
  if(!f->fileStructure.is_open()){
    cout << "Error" << endl;
  }
  cout<<"about to read"<<endl;
  f->fileStructure.read(buffer, n);
  if(!f->fileStructure){
    cout << "Error" << endl;
  }
  cout << "Read " << n << endl;
  buf = buffer;
  return f->fileStructure.gcount();
}


/**
 * Mimic the write() system call (unallocated pages must be allocated before writing)
 */


ssize_t vdiWrite(VDIFile *f, void *buf, ssize_t n){

}
