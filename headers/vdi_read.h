#ifndef VDI_READ
#define VDI_READ
#include <cstdint>
#include <string>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

class vdi_read{
  int vdiOpen(char *fn);
  void vdiClose(int f);
  off_t vdiSeek(int f, off_t offset, int whence);
  ssize_t vdiRead(int f, void *buf, ssize_t n);
  ssize_t vdiWrite(int f, void *buf, ssize_t n);
}

#endif
