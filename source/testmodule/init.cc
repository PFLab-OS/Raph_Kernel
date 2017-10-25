#include <stdio.h>

int main() {
  printf("executed init.\n");

  while(1) {
    asm volatile (
      "mov $61,%%rax;"
      "syscall;"
      :::"memory","rax","rbx","rcx","rdx","rsi","rdi","r8","r9","r10","r11","r12","r13","r14","r15");
  }

  return 0;
}
