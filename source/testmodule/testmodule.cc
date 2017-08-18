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
  int res;
  printf("hello world\n");
  if(argv && argv[0]){
    argv[0][0] = 'X';
    argv[0][1] = 0;
  }
  if (init() != 0) {
    return -1;
  }
  //fork
  asm volatile (
      "mov $57,%%rax;"
      "syscall;"
      "mov %%rax,%0"
      :"=m"(res)::"memory","rax","rbx","rcx","rdx","rsi","rdi","r8","r9","r10","r11","r12","r13","r14","r15");
  if (res == 0) {
    //In child Process
    asm volatile (
        "mov $59,%%rax;"
        "syscall;"
        :::"memory","rax","rbx","rcx","rdx","rsi","rdi","r8","r9","r10","r11","r12","r13","r14","r15");
  }

  asm volatile (
      "mov $61,%%rax;"
      "syscall;"
      :::"memory","rax","rbx","rcx","rdx","rsi","rdi","r8","r9","r10","r11","r12","r13","r14","r15");
  printf("Process Created pid = %d\n",res);
  printf("good bye!\n");

  for (;;) {
    asm volatile (
        "mov $329,%%rax;"
        "syscall;"
        :::"memory","rax","rbx","rcx","rdx","rsi","rdi","r8","r9","r10","r11","r12","r13","r14","r15");
  }

  return calc();
}
