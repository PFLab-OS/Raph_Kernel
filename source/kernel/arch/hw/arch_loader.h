/*
 *
 * Copyright (c) 2017 Raphine Project
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
 * Author: Liva
 * 
 */

#ifndef __RAPH_KERNEL_LOADER_H__
#error "invalid inclusion"
#endif /* __RAPH_KERNEL_LOADER_H__ */

#include <mem/physmem.h>
#include <mem/paging.h>
#include <cpu.h>
#include <mem/kstack.h>
#include <gdt.h>
#include <tty.h>

//TODO:fs/gsレジスタに関してよしなにする
  
//8*24 = 0xc0 
struct Context {
    uint64_t fs = USER_DS;
    uint64_t gs = USER_DS;
    uint64_t es = USER_DS;
    uint64_t ds = USER_DS;

    uint64_t rdi,rsi,rbp,rbx,rdx,rcx,rax;
    uint64_t r8,r9,r10,r11,r12,r13,r14,r15;
    uint64_t rflags = 0x200;
    
    uint64_t rip;
    uint64_t cs = USER_CS;

    uint64_t rsp;
    uint64_t ss = USER_DS;
} __attribute__ ((packed));

extern "C" int execute_elf_binary(FType f, uint64_t *stack_addr, uint64_t cs, uint64_t ds);
extern "C" int resume_elf_binary(Context* context,uint64_t* saved_rsp);
extern "C" int execute_kernel_elf_binary(FType f, uint64_t *stack_addr);

class Loader : public LoaderInterface {
public:
  Loader() {
  }
  Context context;
  static void CopyContext(Context* dst, const Context* src) {
    *dst = *src;
  }
  virtual void Map1GAddr(virt_addr start) override final {
    assert((start % 0x40000000) == 0);
    PhysAddr paddr;
    physmem_ctrl->Alloc(paddr, 0x40000000, 0x40000000);
    paging_ctrl->Map1GPageToVirtAddr(start, paddr, PML4E_WRITE_BIT | PML4E_USER_BIT, PDPTE_WRITE_BIT | PDPTE_GLOBAL_BIT | PDPTE_USER_BIT); // TODO check return value
  }
  virtual void MapAddr(virt_addr start, virt_addr end) override final {
    start = PagingCtrl::RoundAddrOnPageBoundary(start);
    end = PagingCtrl::RoundUpAddrOnPageBoundary(end);
    size_t psize = PagingCtrl::ConvertNumToPageSize(end - start);
    PhysAddr paddr;
    physmem_ctrl->Alloc(paddr, psize);
    paging_ctrl->MapPhysAddrToVirtAddr(start, paddr, psize, PDE_WRITE_BIT | PDE_USER_BIT, PTE_WRITE_BIT | PTE_GLOBAL_BIT | PTE_USER_BIT); // TODO check return value
  }
  //x64の場合スタックを作る
  //TODO:Execute()のためにstackのアドレスを返しているが,これをやめる
  //設計を上手くどうにかする
  virtual uint64_t* MakeExecuteEnvironment(FType entry) {
    // TODO : カーネルスタックで実行しない
    // TODO : 現在のカーネルスタックが破棄されるので、なんとかする
    //uint64_t *stack_addr = reinterpret_cast<uint64_t *>(KernelStackCtrl::GetCtrl().AllocThreadStack(cpu_ctrl->GetCpuId()));
    //TODO:なんとかする
    uint64_t *stack_addr = reinterpret_cast<uint64_t *>(0xfffbc000);
    //MapAddr(reinterpret_cast<virt_addr>(0xfff9b000),reinterpret_cast<virt_addr>(0xfffbc000));
    MapAddr(0xfff9b000,0xfffbc000);
    gtty->Printf("stack addr: %llx\n", stack_addr);
    //gtty->Printf("entry point: %llx\n", f);

    int argc = 1;
    const char *str[1] = {"123"};
    size_t arg_buf_size = 0;
    for (int i = 0; i < argc; i++) {
      arg_buf_size += strlen(str[i]) + 1;
    }
    stack_addr -= (arg_buf_size + 7) / 8;
    char *arg_ptr = reinterpret_cast<char *>(stack_addr);
    // null auxiliary vector entry
    stack_addr--;
    *stack_addr = 0;

    stack_addr--;
    *stack_addr = 0;

    // no environment pointers
  
    stack_addr--;
    *stack_addr = 0;

    // argument pointers
    stack_addr -= argc;
    for (int i = 0; i < argc; i++) {
      strcpy(arg_ptr, str[i]);
      stack_addr[i] = reinterpret_cast<uint64_t>(arg_ptr);
      arg_ptr += strlen(str[i]) + 1;
    }
  
    stack_addr--;
    *stack_addr = argc;
  
    gtty->Flush();

    context.rip = reinterpret_cast<uint64_t>(entry);
    context.rsp = context.rbp = reinterpret_cast<uint64_t>(stack_addr);

    return stack_addr;
  }
  virtual void Execute(FType f) override final {
    uint64_t* stack = MakeExecuteEnvironment(f);
  
    int64_t rval = ExecuteSub(f, stack);

    gtty->Printf("return value: %d\n", rval); 
    //gtty->Printf("%s Returned.\n", str);
  }
  virtual void Resume() {
    resume_elf_binary(&context,&saved_rsp);
  } 
  virtual void ExitResume() {
    asm("movq %0,%%rsp;"
        "ret"
        ::"m"(saved_rsp));
  }
  virtual void SetContext(Context* _context) {
    context = *_context;
  }
protected:
  virtual int64_t ExecuteSub(FType f, uint64_t *stack_addr) {
    return execute_elf_binary(f, stack_addr, USER_CS, USER_DS);
  }
private:
  uint64_t saved_rsp;
};

class Ring0Loader : public Loader {
private:
  virtual int64_t ExecuteSub(FType f, uint64_t *stack_addr) override {
    return execute_kernel_elf_binary(f, stack_addr);
  }
};
