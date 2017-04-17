#include <stdio.h>

int x = 123;
int buf[10] = {0};

int init() {
  for(int i = 0; i < 10; i++) {
    if (buf[i] != 0) {
      return -1;
    }
    buf[i] = i;
  }
  return 0;
}

int calc() {
  int y = x;
  for(int i = 0; i < 10; i++) {
    y += buf[i];
  }
  return y;
}

int main(int argc, char *argv[])
{
  printf("hello world\n");
  if(argv && argv[0]){
    argv[0][0] = 'X';
    argv[0][1] = 0;
  }
  if (init() != 0) {
    return -1;
  }
  return calc();
}
