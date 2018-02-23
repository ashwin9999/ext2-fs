#ifndef VDIFILE
#define VDIFILE
#include <fstream>
#include "vdiheader.h"

using namespace std;

struct VDIFile{
  fstream fileStructure;
  VDIHeader header;
  int cursor;
};

#endif
