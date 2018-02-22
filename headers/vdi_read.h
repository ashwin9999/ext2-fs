#ifndef VDI_READ
#define VDI_READ
#include <cstdint>
#include <string>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

class vdi_read{
  VDIFile *vdiOpen(char *fn);
  void vdiClose(VDIFile *f);
  off_t vdiSeek(VDIFile *f, off_t offset, int whence);
  ssize_t vdiRead(VDIFile *f, void *buf, ssize_t n);
  ssize_t vdiWrite(VDIFile *f, void *buf, ssize_t n);
}

#endif
