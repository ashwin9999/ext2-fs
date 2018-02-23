#include "vdi_read.h"
#include <iostream>
#include <string>
#include "vdifile.h"

using namespace std;

int main(int argc, char *argv[]){
  short int magic;
  cout << "Was working here" << endl;
  VDIFile *file = vdiOpen(argv[1]);
  cout << "Opened" << endl;
  if (file->cursor < 0) return -1; 
  cout << "Opened the file : " << argv[1] << endl;
  off_t result = vdiSeek(file, 254, SEEK_SET);
  cout << "result " << result << endl;
  ssize_t res = vdiRead(file, &magic, 2);
  cout << "res " << res << endl;
  cout << "magic " << hex << magic << endl;
  vdiClose(file);
  return 0;
}

