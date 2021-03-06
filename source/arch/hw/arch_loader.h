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
 * Author: Liva,mumumu
 * 
 */

#ifndef __RAPH_KERNEL_LOADER_H__
#error "invalid inclusion"
#endif /* __RAPH_KERNEL_LOADER_H__ */

#include <mem/physmem.h>
#include <mem/virtmem.h>
#include <ptr.h>
#include <cpu.h>
#include <mem/kstack.h>
#include <gdt.h>
#include <tty.h>
#include <idt.h>

//TODO: using fs/gs registers

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

static_assert(sizeof(Context) == sizeof(uint64_t) * 24,"Invalid size of x86_64 Context struct.");

class ContextWrapper { 
public:
  Context* GetContext() {
    return &(_context);
  }

  ContextWrapper(size_t base,size_t stack) {
    SaveContext(base,stack);
  }

  ContextWrapper() {
  }

  ContextWrapper(Regs* rs) {
    SaveContext(rs);
  }

  ContextWrapper(Context* c) {
    _context = *c;
  }

  void SaveContext(Regs* rs) { 
    _context.rip = rs->rip;
    _context.rflags = rs->rflags;

    _context.rdi = rs->rdi;
    _context.rsi = rs->rsi;

    _context.rsp = rs->rsp;
    _context.rbp = rs->rbp;
    
    _context.rax = rs->rax;
    _context.rbx = rs->rbx;
    _context.rcx = rs->rcx;
    _context.rdx = rs->rdx;

    _context.r8 = rs->r8;
    _context.r9 = rs->r9;
    _context.r10 = rs->r10;
    _context.r11 = rs->r11;
    _context.r12 = rs->r12;
    _context.r13 = rs->r13;
    _context.r14 = rs->r14;

  }

  void SaveContext(size_t base,size_t stack) {
    _context.rsp = stack;
    asm("movq %7,%%rax;"
        "subq $112,%%rax;"
        "movq 0(%%rax),%%rcx;"
        "movq %%rcx,%0;"
        "movq 8(%%rax),%%rcx;"
        "movq %%rcx,%1;"
        "movq 16(%%rax),%%rcx;"
        "movq %%rcx,%2;"
        "movq 24(%%rax),%%rcx;"
        "movq %%rcx,%3;"
        "movq 32(%%rax),%%rcx;"
        "movq %%rcx,%4;"
        "movq 40(%%rax),%%rcx;"
        "movq %%rcx,%5;"
        "movq 48(%%rax),%%rcx;"
        "movq %%rcx,%6;"
      : "=m"(_context.rip),"=m"(_context.rflags),"=m"(_context.rdi),"=m"(_context.rsi),
        "=m"(_context.rdx),"=m"(_context.r10),"=m"(_context.r8)
      : "m"(base)
      : "%rax","%rcx");
    asm("movq %7,%%rax;"
        "subq $112,%%rax;"
        "movq 56(%%rax),%%rcx;"
        "movq %%rcx,%0;"
        "movq 64(%%rax),%%rcx;"
        "movq %%rcx,%1;"
        "movq 72(%%rax),%%rcx;"
        "movq %%rcx,%2;"
        "movq 80(%%rax),%%rcx;"
        "movq %%rcx,%3;"
        "movq 88(%%rax),%%rcx;"
        "movq %%rcx,%4;"
        "movq 96(%%rax),%%rcx;"
        "movq %%rcx,%5;"
        "movq 104(%%rax),%%rcx;"
        "movq %%rcx,%6;"
        "swapgs;"
      : "=m"(_context.r9),"=m"(_context.rbx),"=m"(_context.rbp),"=m"(_context.r12),
        "=m"(_context.r13),"=m"(_context.r14),"=m"(_context.r15)
      : "m"(base)
      : "%rax","%rcx");
  }

  void ShowRegContext() {
    gtty->Printf("rip : %llx\n",_context.rip);
    gtty->Printf("rax : %llx\n",_context.rax);
    gtty->Printf("rbx : %llx\n",_context.rbx);
    gtty->Printf("rcx : %llx\n",_context.rcx);
    gtty->Printf("rdx : %llx\n",_context.rdx);
    gtty->Printf("rsi : %llx\n",_context.rsi);
    gtty->Printf("rdi : %llx\n",_context.rdi);
    gtty->Printf("rsp : %llx\n",_context.rsp);
    gtty->Printf("rbp : %llx\n",_context.rbp);
    gtty->Printf("r8 : %llx\n", _context.r8);
    gtty->Printf("r9 : %llx\n", _context.r9);
    gtty->Printf("r10 : %llx\n",_context.r10);
    gtty->Printf("r11 : %llx\n",_context.r11);
    gtty->Printf("r12 : %llx\n",_context.r12);
    gtty->Printf("r13 : %llx\n",_context.r13);
    gtty->Printf("r14 : %llx\n",_context.r14);
    gtty->Printf("r15 : %llx\n",_context.r15);
    gtty->Printf("rflags : %llx\n",_context.rflags);

    gtty->Flush();
  }

private:
  Context _context;
};

extern "C" int execute_elf_binary(FType f, uint64_t *stack_addr, uint64_t cs, uint64_t ds);
extern "C" int resume_elf_binary(Context* context,uint64_t* _saved_rsp);
extern "C" int execute_kernel_elf_binary(FType f, uint64_t *stack_addr);

class Loader : public LoaderInterface {
public:
  //TODO:redesigning
  Loader(sptr<MemCtrl> mem_ctrl) : _mem_ctrl(mem_ctrl) {
  }
  //Using this constructor is not recommended.
  Loader() : _mem_ctrl(make_sptr(system_memory_space)) {
  }
  Context context;
  static void CopyContext(Context* dst, const Context* src) {
    *dst = *src;
  }
  virtual void Map1GAddr(virt_addr start) override final {
    assert((start % 0x40000000) == 0);
    PhysAddr paddr;
    physmem_ctrl->Alloc(paddr, 0x40000000, 0x40000000);
    _mem_ctrl->Map1GPageToVirtAddr(start, paddr, PML4E_WRITE_BIT | PML4E_USER_BIT, PDPTE_WRITE_BIT | PDPTE_GLOBAL_BIT | PDPTE_USER_BIT); // TODO check return value
  }
  virtual void MapAddr(virt_addr start, virt_addr end) override final {
    start = PagingCtrl::RoundAddrOnPageBoundary(start);
    end = PagingCtrl::RoundUpAddrOnPageBoundary(end);
    size_t psize = PagingCtrl::ConvertNumToPageSize(end - start);
    PhysAddr paddr;
    physmem_ctrl->Alloc(paddr, psize);
    _mem_ctrl->MapPhysAddrToVirtAddr(start, paddr, psize, PDE_WRITE_BIT | PDE_USER_BIT, PTE_WRITE_BIT | PTE_GLOBAL_BIT | PTE_USER_BIT); // TODO check return value
  }

  //Making stack
  //TODO: redesigning
  virtual uint64_t* MakeExecuteEnvironment(FType entry) {
    uint64_t *stack_addr = reinterpret_cast<uint64_t *>(_stack_entry_address);
    MapAddr(_stack_entry_address - _stack_size,_stack_entry_address);
    gtty->Printf("stack addr: %llx\n", stack_addr);

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
  }
  virtual void Resume() {
    resume_elf_binary(&context,&_saved_rsp);
  }
  virtual void ExitResume() {
    asm("movq %0,%%rsp;"
        "ret"
        ::"m"(_saved_rsp));
  }
  virtual void SetContext(Context* _context) {
    context = *_context;
  }
protected:
  virtual int64_t ExecuteSub(FType f, uint64_t *stack_addr) {
    return execute_elf_binary(f, stack_addr, USER_CS, USER_DS);
  }
private:
  //TODO: Check this stack entry address is valid?
  static const uint64_t _stack_entry_address = 0xfffbc000;
  static const uint64_t _stack_size = 0x21000;
  sptr<MemCtrl> _mem_ctrl;
  uint64_t _saved_rsp;
};

class Ring0Loader : public Loader {
private:
  virtual int64_t ExecuteSub(FType f, uint64_t *stack_addr) override {
    return execute_kernel_elf_binary(f, stack_addr);
  }
};
