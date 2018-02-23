#include "vdi_read.h"
#include <iostream>
#include <string>
#include "vdifile.h"
#include <fstream>

using namespace std;

int main(int argc, char *argv[]){

  fstream f;
  f.open(argv[1], ios::in | ios::out | ios::binary | ios::ate); // ate puts the cursor at the end of the file to start with
  int size = (int) f.tellg(); // Store the max size of file
  cout << "Size: " << size << endl;

  f.seekg(0, ios::beg); // Moves the cursor for read - Move back to beginning

  char buffer[1000];

  cout << "Get pos: " << f.tellg() << endl;
  f.read(buffer, 1000); // Read the file
  if(!f){
    cout << "Error" << endl;
  }
  cout<<"success?"<<endl;

  cout<<buffer<<endl;

  cout << "Get pos: " << f.tellg() << endl;
  f.seekg(1);
  cout << "Get pos: " << f.tellg() << endl;
  f.seekg(254, ios::beg);
  cout << "Get pos: " << f.tellg() << endl;

  // Attempt to get the magic test value
  char magic[2];
  f.read(magic, 2);
  short int mag = (short int)(magic[0] + magic[1]);
  //for(int i = 0; i < 5; i++){
    //short int mag = (short int)magic[i];
    //cout << hex << mag << endl;
  //}
  cout << "should say magic value: " << hex << mag << endl;

  f.seekg(0);
  char * memblock;
  memblock = new char[size];
  f.read(memblock, size);
  f.close();

  cout << "Value at 254: " << memblock[253] << endl;
  cout << "Value at 1: " << memblock[1] << endl;
  for(int i = ; i < 260; i++){
    cout << memblock[i];
  }
  cout << endl;













  /*short int magic;
  cout << "Opening: " << endl;
  VDIFile *file = vdiOpen(argv[1]);

  //cout<<file->fileStructure.is_open()<<endl;
  //file->fileStructure.seekg(0, ios::beg);
  //cout << "seeked to 0"<<endl;
  //char test[100];
  //cout<<file->fileStructure.tellg()<<endl;
  //return 1;
  //file->fileStructure.read(test, 2);
  //cout << test <<endl;

  cout << "Opened the file : " << argv[1] << endl;
  off_t result = vdiSeek(file, 254, SEEK_SET);
  cout << "Seeked to " << result << endl;
  ssize_t res = vdiRead(file, &magic, 2);
  cout << "res " << res << endl;
  cout << "magic " << hex << magic << endl;
  vdiClose(file);
  return 0;*/
  return 0;
}
