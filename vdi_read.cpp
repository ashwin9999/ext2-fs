#include "vdi_read.h"

using namespace std;

VDIFile *vdiOpen(char *fn){
  VDIFile file;
  file.cursor = 0;
  file.fileSize = 0;
  cout << "was at open" << endl;
  file.fileStructure.open(fn, ios::in | ios::out | ios::binary | ios::ate);
  cout << "opened the file" << endl;
  file.fileSize = (int) file.fileStructure.tellg();
  file.fileStructure.seekg(0,ios::beg);
  VDIFile *ptr = (VDIFile*) malloc(sizeof(VDIFile));
  ptr = &file;
  return ptr;
}

void vdiClose(VDIFile *f){
  f->fileStructure.close();
}

off_t vdiSeek(VDIFile *f, off_t offset, int whence){
  cout << "Before seek" << endl;
  if(whence == SEEK_SET){
    f->fileStructure.seekg(offset, ios::beg);
    cout << "Inside seek" << endl;
    f->cursor = offset;
  }
  if(whence == SEEK_CUR){
    f->fileStructure.seekg(offset, ios::cur);
    f->cursor += offset;
  }
  if(whence == SEEK_END){
    f->fileStructure.seekg(offset, ios::end);
    f->cursor = offset + f->fileSize;
  }
  return f->cursor;
}

ssize_t vdiRead(VDIFile *f, void *buf, ssize_t n){
  char tmp_buf[1000];
  f->fileStructure.read(tmp_buf, n);
  if(f->fileStructure){
    buf = tmp_buf;
    return n;
  }
  else{
    return f->fileStructure.gcount();
  }
}

ssize_t vdiWrite(VDIFile *f, void *buf, ssize_t n){
  /* f->fileStructure.write(buf, n);
  if(f->fileStructure){
    return n;
  }
  else{
    return -1;
  }
  */
  return 0;
}







  
