/*
 *
 * Copyright (c) 2016 Raphine Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Author: hikalium
 * 
 */

#include <tty.h>
#include <mem/physmem.h>
#include <string.h>
#include <stdlib.h>
#include <x86.h>
#include <syscall.h>
#include <gdt.h>
#include <process.h>
#include <multiboot.h> //for exec
#include <global.h>

extern "C" int64_t syscall_handler();
extern size_t syscall_handler_stack; 
extern size_t syscall_handler_caller_stack; 

extern "C" int64_t syscall_handler_sub(SystemCallCtrl::Args *args, int index) {
  return SystemCallCtrl::Handler(args, index);
}

//TODO:なんとかする
#define SaveContext(c) \
    c.rsp = syscall_handler_caller_stack;\
    asm("movq %7,%%rax;"\
        "subq $112,%%rax;"\
        "movq 0(%%rax),%%rcx;"\
        "movq %%rcx,%0;"\
        "movq 8(%%rax),%%rcx;"\
        "movq %%rcx,%1;"\
        "movq 16(%%rax),%%rcx;"\
        "movq %%rcx,%2;" \
        "movq 24(%%rax),%%rcx;"\
        "movq %%rcx,%3;" \
        "movq 32(%%rax),%%rcx;"\
        "movq %%rcx,%4;"\
        "movq 40(%%rax),%%rcx;"\
        "movq %%rcx,%5;"\
        "movq 48(%%rax),%%rcx;"\
        "movq %%rcx,%6;"\
      : "=m"(c.rip),"=m"(c.rflags),"=m"(c.rdi),"=m"(c.rsi),\
        "=m"(c.rdx),"=m"(c.r10),"=m"(c.r8)\
      : "m"(syscall_handler_stack)\
      : "%rax","%rcx");\
    asm("movq %7,%%rax;"\
        "subq $112,%%rax;"\
        "movq 56(%%rax),%%rcx;"\
        "movq %%rcx,%0;"\
        "movq 64(%%rax),%%rcx;"\
        "movq %%rcx,%1;"\
        "movq 72(%%rax),%%rcx;"\
        "movq %%rcx,%2;"\
        "movq 80(%%rax),%%rcx;"\
        "movq %%rcx,%3;"\
        "movq 88(%%rax),%%rcx;"\
        "movq %%rcx,%4;"\
        "movq 96(%%rax),%%rcx;"\
        "movq %%rcx,%5;"\
        "movq 104(%%rax),%%rcx;"\
        "movq %%rcx,%6;"\
        "swapgs;"\
      : "=m"(c.r9),"=m"(c.rbx),"=m"(c.rbp),"=m"(c.r12),\
        "=m"(c.r13),"=m"(c.r14),"=m"(c.r15)\
      : "m"(syscall_handler_stack)\
      : "%rax","%rcx");


void SystemCallCtrl::Init() {
  // IA32_EFER.SCE = 1
  const uint64_t bit_efer_SCE = 0x01;
  uint64_t efer = rdmsr(kIA32EFER);
  efer |= bit_efer_SCE;
  wrmsr(kIA32EFER, efer);
  // set vLSTAR
  wrmsr(kIA32STAR, (static_cast<uint64_t>(KERNEL_CS) << 32) | ((static_cast<uint64_t>(USER_DS) - 8) << 48));
  wrmsr(kIA32LSTAR, reinterpret_cast<uint64_t>(syscall_handler));
  wrmsr(kIA32KernelGsBase, KERNEL_DS);
  syscall_handler_stack = KernelStackCtrl::GetCtrl().AllocThreadStack(cpu_ctrl->GetCpuId());
}

int64_t SystemCallCtrl::Handler(Args *args, int index) {
  switch(index) {
  case 2:
    {
      // // open
      // gtty->Printf("<%s>", args->arg1);
      // return 3;
      break;
    }
  case 16:
    {
      // ioctl
      if (args->arg1 == 1) {
        // stdout
        switch (args->arg2) {
        case TCGETS:
          {
            return 0;
          }
        case TIOCGWINSZ:
          {
            winsize *ws = reinterpret_cast<winsize *>(args->arg3);
            ws->ws_row = gtty->GetRow();
            ws->ws_col = gtty->GetColumn();
            ws->ws_xpixel = 0;
            ws->ws_ypixel = 0;
            return 0;
          }
        default:
          {
            gtty->DisablePrint();
            gtty->ErrPrintf("%x\n", args->arg2);
            kernel_panic("Sysctrl", "unknown argument(ioctrl)");
          }
        };
      } else {
        kernel_panic("Sysctrl", "unknown fd(ioctrl)");
      }
      break;
    }
  case 20:
    // writev
    {
      if (args->arg1 == 1) {
        // stdout
        iovec *iv_array = reinterpret_cast<iovec *>(args->arg2);
        int rval = 0;
        for (int i = 0; i < args->arg3; i++) {
          for (int j = 0; j < iv_array[i].iov_len; j++) {
            gtty->Printf("%c", reinterpret_cast<char *>(iv_array[i].iov_base)[j]);
            rval++;
          }
        }
        gtty->Flush();
        return rval;
      } else {
        gtty->DisablePrint();
        gtty->ErrPrintf("<%llx>", args->arg1);
        kernel_panic("Sysctrl", "unknown fd(writev)");
      }
      break;
    }
  case 57:
    //fork
    {
      Context c;

      Process* p = process_ctrl->GetCurrentExecProcess();
      SaveContext(c);

      Process* forkedp = process_ctrl->ForkProcess(p);
      c.rax = forkedp->GetPid();
      p->elfobj->SetContext(&c);

      if (p->GetStatus() == ProcessStatus::HALTED) {
        kernel_panic("syscall","fork");
      }else {
        process_ctrl->SetStatus(p,ProcessStatus::HALTED);
      }
      Process::ReturnToKernelJob(p);

      while(true) {asm volatile("hlt;");}
    }
  case 59:
    //execve TODO:本物のexecve
    {
      Process* p = process_ctrl->GetCurrentExecProcess();

      process_ctrl->ExecProcess(p,multiboot_ctrl->LoadFile("forked.elf")->GetRawPtr());

      p->elfobj->ReturnToKernelJob();

      while(true) {asm volatile("hlt;");}
    }
  case 61:
    //本物のwait? TODO:要確認
    //DBG
    {
      Context c;
      Process* p = process_ctrl->GetCurrentExecProcess();

      pid_t pid = process_ctrl->WaitProcess(p);
      if (pid != INVALID_PID) return pid;

      SaveContext(c);
      p->elfobj->SetContext(&c);
      assert(process_ctrl->MakeProcessSleep(p,p));
      Process::ReturnToKernelJob(p);

    }
  case 63:
    // uname
    {
#define __NEW_UTS_LEN 64
      struct new_utsname {
        char sysname[__NEW_UTS_LEN + 1];
        char nodename[__NEW_UTS_LEN + 1];
        char release[__NEW_UTS_LEN + 1];
        char version[__NEW_UTS_LEN + 1];
        char machine[__NEW_UTS_LEN + 1];
        char domainname[__NEW_UTS_LEN + 1];
      };
      new_utsname tmp;
      strcpy(tmp.sysname, "");
      strcpy(tmp.nodename, "");
      strcpy(tmp.release, "");
      strcpy(tmp.version, "");
      strcpy(tmp.machine, "");
      strcpy(tmp.domainname, "");
      memcpy(reinterpret_cast<void *>(args->arg1), &tmp, sizeof(new_utsname));
      return 0;
    }
  case 158:
    {
      gtty->Printf("sys_arch_prctl code=%llx addr=%llx\n", args->arg1, args->arg2);
      switch(args->arg1){
      case kArchSetGs:
        wrmsr(kIA32GsBase, args->arg2);
        break;
      case kArchSetFs:
        wrmsr(kIA32FsBase, args->arg2);
        break;
      case kArchGetFs:
        *reinterpret_cast<uint64_t *>(args->arg2) = rdmsr(kIA32FsBase);
        break;
      case kArchGetGs:
        *reinterpret_cast<uint64_t *>(args->arg2) = rdmsr(kIA32GsBase);
        break;
      default:
        gtty->DisablePrint();
        gtty->ErrPrintf("Invalid args\n", index);
        kassert(false);
      }
      return 0;
    }
  case 231:
    {
      // exit group
      gtty->Printf("user program called exit\n");
      gtty->Flush();

      process_ctrl->ExitProcess(process_ctrl->GetCurrentExecProcess());

      while(true) {asm volatile("hlt;");}
    }
  case 329:
    {
      //TODO;x86依存コード多し
      //gtty->Printf("user program called context switch\n");
      //gtty->Flush();
      Context c;
      Process* p = process_ctrl->GetCurrentExecProcess();
      //uint64_t stack_addr = syscall_handler_stack;

      SaveContext(c);
      c.rax = 1; //return value

      p->elfobj->SetContext(&c);

      Process::ReturnToKernelJob(p);

      while(true) {asm volatile("hlt;");}
    }
  }
  gtty->DisablePrint();
  gtty->ErrPrintf("syscall(%d) (return address %llx)\n", index, args->raddr);
  kernel_panic("Syscall", "unimplemented systemcall");
}

