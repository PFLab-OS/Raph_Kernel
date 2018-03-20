#include <stdio.h>
#include <unistd.h>

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

pid_t myfork() {
  pid_t pid;
  asm volatile (
      "mov $57,%%rax;"
      "syscall;"
      "mov %%rax,%0;"
      :"=m"(pid)::"memory","rax","rbx","rcx","rdx","rsi","rdi","r8","r9","r10","r11","r12","r13","r14","r15");
  return pid;
}

void myexit() {
  asm volatile (
      "mov $231,%%rax;"
      "syscall;"
      :::"memory","rax","rbx","rcx","rdx","rsi","rdi","r8","r9","r10","r11","r12","r13","r14","r15");
}

void contextswitch() {
  asm volatile (
      "mov $329,%%rax;"
      "syscall;"
      :::"memory","rax","rbx","rcx","rdx","rsi","rdi","r8","r9","r10","r11","r12","r13","r14","r15");
}

void myexec() {
    asm volatile (
        "mov $59,%%rax;"
        "syscall;"
        :::"memory","rax","rbx","rcx","rdx","rsi","rdi","r8","r9","r10","r11","r12","r13","r14","r15");
}

void mywait() {
  asm volatile (
      "mov $61,%%rax;"
      "syscall;"
      :::"memory","rax","rbx","rcx","rdx","rsi","rdi","r8","r9","r10","r11","r12","r13","r14","r15");
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

  printf("This is test module.(test: fork/exec/wait/contextswitch)\n");

  pid_t pid = myfork();

  contextswitch();

  printf("pid = %d\n",pid);


  if (pid == 0) {
    //In child Process
    printf("  forked!\n");
    myexec();
  }

  mywait();

  printf("good bye!\n");

  return calc();
}
