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

extern "C" int execute_elf_binary(FType f, uint64_t *stack_addr);

class Loader : public LoaderInterface {
public:
  virtual void MapAddr(virt_addr start, virt_addr end) override final {
    start = PagingCtrl::RoundAddrOnPageBoundary(start);
    end = PagingCtrl::RoundUpAddrOnPageBoundary(end);
    size_t psize = PagingCtrl::ConvertNumToPageSize(end - start);
    PhysAddr paddr;
    physmem_ctrl->Alloc(paddr, psize);
    paging_ctrl->MapPhysAddrToVirtAddr(start, paddr, psize, PDE_WRITE_BIT | PDE_USER_BIT, PTE_WRITE_BIT | PTE_GLOBAL_BIT | PTE_USER_BIT); // TODO check return value
  }
  virtual void Execute(FType f) override final {
    // TODO : カーネルスタックで実行しない
    // TODO : 現在のカーネルスタックが破棄されるので、なんとかする
    uint64_t *stack_addr = reinterpret_cast<uint64_t *>(KernelStackCtrl::GetCtrl().AllocThreadStack(cpu_ctrl->GetCpuId()));
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
  
    int64_t rval = execute_elf_binary(f, stack_addr);

    gtty->Printf("return value: %d\n", rval); 
    gtty->Printf("%s Returned.\n", str);
  }
};
