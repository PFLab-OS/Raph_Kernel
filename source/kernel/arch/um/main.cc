#include <stdio.h>
#include <elf.h>
#include <tty.h>
#include <stdlib.h>
#include <stdint.h>


extern "C" void function() __attribute__((weak));
void function() {
}

Tty *gtty;

int main() {
  printf("=== user mode Raph_Kernel(experimental)\t===\n");
  
  FILE *fp;
  const char *fname = "kernel";
  size_t file_size;

  fp = fopen( fname, "rb" );
  if( fp == NULL ){
    printf( "%sファイルが開けません\n", fname );
    return -1;
  }

  fseek( fp, 0, SEEK_END );
  file_size = ftell( fp );
  printf( "%sのファイルサイズ:%zu バイト\n", fname, file_size);

  fseek(fp, 0, SEEK_SET);

  gtty = new Tty();

  uint8_t *file = new uint8_t[file_size];

  size_t read_size = 0;
  while(read_size != file_size) {
    read_size += fread(file + read_size, 1, file_size - read_size, fp);
  };

  ElfObject elf(file);

  elf.Init();
  
  return 0;
}

extern "C" void _kassert(const char *file, int line, const char *func) {
  if (gtty != nullptr) {
    gtty->ErrPrintf("assertion failed at %s l.%d (%s)\n",
                     file, line, func);
  }
  exit(-1);
}

extern "C" void _kernel_panic(const char *class_name, const char *err_str) {
  if (gtty != nullptr) {
    gtty->ErrPrintf("\n!!!! Kernel Panic !!!!\n");
    gtty->ErrPrintf("[%s] error: %s\n",class_name, err_str);
    gtty->ErrPrintf("\n"); 
  }
  exit(-1);
}
