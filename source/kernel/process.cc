
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
 * Author: mumumu
 * 
 */

//TODO:プロセスが死んだ後処理
//TODO:ELFのロードを別のクラスで行う


#include <elfhead.h>
#include <process.h>
#include <task.h>
#include <tty.h>
#include <multiboot.h>
#include <syscall.h>
#include <gdt.h>
#include <stdlib.h>
#include <mem/paging.h>
#include <mem/physmem.h>

void Process::Init() {
  //これもコンストラクタでやらせたい
  task = make_sptr(new TaskWithStack(cpu_ctrl->GetCpuId()));
  task->Init();

  //pml4t = reinterpret_cast<PageTable*>(virtmem_ctrl->Alloc(sizeof(PageTable)));
  //kassert(pml4t);
  
}

void Process::ReturnToKernelJob(Process* p) {
  gtty->Printf("called ReturnToKernelJob()\n");
  gtty->Flush();

  asm("movq %0,%%rsp;"
      "ret"
      ::"m"(p->saved_rsp));

}

void Process::Exit(Process* p) {
  process_ctrl->HaltProcess(p);
}

void ProcessCtrl::Init() {
  {

    Locker locker(table_lock);
    process_table.Init();
    CreateFirstProcess();
  }

  auto ltask_ = make_sptr(new Task);
  ltask_->SetFunc(make_uptr(new Function<sptr<Task>>([](sptr<Task> ltask) {

    //TODO:デバッグ用簡易実装
    Process* p = process_ctrl->GetNextExecProcess();
    if(p != nullptr && p->status == ProcessStatus::RUNNABLE) {
      //処理したいプロセスが存在すれば、それを追加
      task_ctrl->Register(cpu_ctrl->GetCpuId(),p->GetKernelJob());
      task_ctrl->Register(cpu_ctrl->GetCpuId(),ltask);
    } 

    //誰がディスパッチ用のタスク（これ）をカーネルジョブに追加する？
    //プリエンプティブ・ノンプリエンプティブで異なるため要検討
    if(p != nullptr && p->status ==ProcessStatus::EMBRYO){
      task_ctrl->Register(cpu_ctrl->GetCpuId(),ltask);
    }

  },ltask_)));

  task_ctrl->Register(cpu_ctrl->GetCpuId(),ltask_);

}

//ここでスケジューリングをする
Process* ProcessCtrl::GetNextExecProcess() {

  {
    Locker locker(table_lock);
    current_process = process_table.GetNextProcess();
  }
  return current_process;

}

//実行可能になったプロセスを返す関数
Process* ProcessCtrl::CreateProcess() { 
  return nullptr;
  Process* process = process_table.AllocProcess();
  if(process == nullptr) {
    kernel_panic("ProcessCtrl","Could not alloc process space.");
  }
  process->Init();

  process->SetKernelJobFunc(make_uptr(new Function2<sptr<TaskWithStack>,Process*>([](sptr<TaskWithStack> task,Process* p) {
    //TODO
    {
      Locker locker(process_ctrl->table_lock);
      p->status = ProcessStatus::RUNNABLE;
    }
    
    while(true) {
      task->Wait();

      gtty->Printf("Context Switch\n");
      gtty->Flush();
      Process::Resume(p);

    }
  },process->GetKernelJob(),process)));

  return process;
}


//すべての親となるプロセスを生成する
Process* ProcessCtrl::CreateFirstProcess() {
  Process* process = process_table.AllocProcess();
  if(process == nullptr) {
    kernel_panic("ProcessCtrl","Could not alloc first process space.");
  }
  process->Init();

  process->SetKernelJobFunc(make_uptr(new Function2<sptr<TaskWithStack>,Process*>([](sptr<TaskWithStack> task,Process* p) {
    //TODO: cr3を変更する機能

    //初期化用の処理
    //copy and paste from elf.cc
    const void* ptr = multiboot_ctrl->LoadFile("test.elf")->GetRawPtr();
    const uint8_t *head = reinterpret_cast<const uint8_t *>(ptr);
    const Elf64_Ehdr *ehdr = reinterpret_cast<const Elf64_Ehdr *>(ptr);

    // IA32_EFER.SCE = 1
    SystemCallCtrl::Init();
  //  if(!IsElf(ehdr) || !IsElf64(ehdr) || (!IsOsabiSysv(ehdr) && !IsOsabiGnu(ehdr))){
  //   gtty->Printf("Not supported module type.\n");
  //   return;
  // }
    gtty->Printf("ABI: %d\n", ehdr->e_ident[EI_OSABI]);
    gtty->Printf("Entry point is 0x%08x \n", ehdr->e_entry);

    const Elf64_Shdr *shstr = &reinterpret_cast<const Elf64_Shdr *>(head + ehdr->e_shoff)[ehdr->e_shstrndx];
    
    Elf64_Xword total_memsize = 0;

    gtty->Printf("Sections:\n");
    if (ehdr->e_shstrndx != SHN_UNDEF) {
      const char *strtab = reinterpret_cast<const char *>(head + shstr->sh_offset);
      for(int i = 0; i < ehdr->e_shnum; i++){
        const Elf64_Shdr *shdr = (const Elf64_Shdr *)(head + ehdr->e_shoff + ehdr->e_shentsize * i);
        const char *sname = strtab + shdr->sh_name;
        //gtty->Printf(" [%2d] %s size: %x offset: %x\n", i, sname, shdr->sh_size, shdr->sh_offset);
        if (total_memsize < shdr->sh_addr + shdr->sh_offset) {
          total_memsize = shdr->sh_addr + shdr->sh_offset;
        }
      }
    }

    uint8_t *membuffer;
    bool page_mapping = false;
    if (ehdr->e_type == ET_DYN) {
      membuffer = reinterpret_cast<uint8_t *>(malloc(total_memsize));
    } else if (ehdr->e_type == ET_EXEC) {
      membuffer = reinterpret_cast<uint8_t *>(0);
      page_mapping = true;
    } else {
      kassert(false);
    }
    gtty->Printf("membuffer: 0x%llx (%llx)\n", membuffer, total_memsize);
    // PT_LOADとなっているセグメントをメモリ上にロード
    for(int i = 0; i < ehdr->e_phnum; i++){
      const Elf64_Phdr *phdr = (const Elf64_Phdr *)(head + ehdr->e_phoff + ehdr->e_phentsize * i);

      if (page_mapping && phdr->p_memsz != 0) {
        virt_addr start = ptr2virtaddr(membuffer) + phdr->p_vaddr;
        virt_addr end = start + phdr->p_memsz;
        start = PagingCtrl::RoundAddrOnPageBoundary(start);
        end = PagingCtrl::RoundUpAddrOnPageBoundary(end);
        size_t psize = PagingCtrl::ConvertNumToPageSize(end - start);
        PhysAddr paddr;
        physmem_ctrl->Alloc(paddr, psize);
        paging_ctrl->MapPhysAddrToVirtAddr(start, paddr, psize, PDE_WRITE_BIT | PDE_USER_BIT, PTE_WRITE_BIT | PTE_GLOBAL_BIT | PTE_USER_BIT); // TODO check return value
      }
      
      switch(phdr->p_type){
        case PT_LOAD:
          gtty->Printf("phdr[%d]: Load to +0x%llx\n", i, phdr->p_vaddr);
          memcpy(membuffer + phdr->p_vaddr, &head[phdr->p_offset], phdr->p_filesz);
          break;
      }
    }
    // セクション .bss を0クリア
    for(int i = 0; i < ehdr->e_shnum; i++){
      const Elf64_Shdr *shdr = (const Elf64_Shdr *)(head + ehdr->e_shoff + ehdr->e_shentsize * i);
      if (shdr->sh_type == SHT_NOBITS) {
        if ((shdr->sh_flags & SHF_ALLOC) != 0) {
          memset(membuffer + shdr->sh_addr, 0, shdr->sh_size);
        }
      }
    }

    // TODO: キレイにする
    uint8_t* stack_addr_tmp = (reinterpret_cast<uint8_t *>(malloc(PagingCtrl::kPageSize*10)) );
    PhysAddr psaddr;
    physmem_ctrl->Alloc(psaddr,PagingCtrl::kPageSize*10);
    paging_ctrl->MapPhysAddrToVirtAddr(PagingCtrl::RoundAddrOnPageBoundary(ptr2virtaddr(stack_addr_tmp)), psaddr, PagingCtrl::kPageSize*11, PDE_WRITE_BIT | PDE_USER_BIT, PTE_WRITE_BIT | PTE_GLOBAL_BIT | PTE_USER_BIT); // TODO check return value

    uint64_t* stack_addr = reinterpret_cast<uint64_t*>(stack_addr_tmp + PagingCtrl::kPageSize*10);


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

    //trapframe eip 
    p->context.rip = reinterpret_cast<uint64_t>(membuffer + ehdr->e_entry);
    p->context.rsp = reinterpret_cast<uint64_t>(stack_addr);




    gtty->Printf("First Process Created rip = %llx\n",p->context.rip);
    gtty->Flush();


    while(true) {
      //Q:こんなクラス設計でいいですか？
      //TODO:ProcessCtrlでStatusいじる
      {
        Locker locker(process_ctrl->table_lock);
        p->status = ProcessStatus::RUNNABLE;
      }

      task->Wait();

      {
        Locker locker(process_ctrl->table_lock);
        p->status = ProcessStatus::RUNNING;
      }
      Process::Resume(p);

    }

  },process->GetKernelJob(),process)));

  task_ctrl->Register(cpu_ctrl->GetCpuId(),process->GetKernelJob());

  return process;
}

void Process::Resume(Process* _p) {
  Process* p = _p;

  //現在のコンテキストを退避
  asm volatile (
      "subq $0x40,%%rsp;"
      "pushfq;"
      "pushq %%rbx;"
      "pushq %%r12;"
      "pushq %%r13;"
      "pushq %%r14;"
      "pushq %%r15;"
      "pushq %%rbp;"
      "jmp _returner;"
      "_saver:"
      "movq %%rsp,%0;"

      "pushq %5;"
      "pushq %6;"
      "pushq %7;"
      "pushq %8;"
      : "=m"(p->saved_rsp)
      : "r"(p->context.ds),"r"(p->context.es),"r"(p->context.fs),"r"(p->context.gs),
        "r"(p->context.ss),"r"(p->context.rsp),"r"(p->context.cs),"r"(p->context.rip)
      : "memory"
      );

  //プロセスのコンテキストをスタックに積む
  asm volatile (
      "pushq %0;"
      "pushq %1;"
      "pushq %2;"
      "pushq %3;"
      "pushq %4;"
      "pushq %5;"
      "pushq %6;"
      "pushq %7;"
      "pushq %8;"
      "pushq %9;"
      "pushq %10;"
      "pushq %11;"
      "pushq %12;"
      "pushq %13;"
      :
      : "r"(p->context.rax),"r"(p->context.rbx),"r"(p->context.rcx),"r"(p->context.rdx),"r"(p->context.r8),
        "r"(p->context.r9),"r"(p->context.r10),"r"(p->context.r11),"r"(p->context.r12),"r"(p->context.r13),
        "r"(p->context.r14),"r"(p->context.r15),"r"(p->context.rdi),"r"(p->context.rsi)
      : "memory"
      );

  //コンテキストをポップして飛ぶ
  asm volatile (
      "movq %1,%%rax;"
      "movq %%rax,%%ds;"
      "movq %2,%%rax;"
      "movq %%rax,%%es;"
      "movq %3,%%rax;"
      "movq %%rax,%%fs;"
      "movq %4,%%rax;"
      "movq %%rax,%%gs;"

      "pushq %0;"
      "popfq;"
      "popq %%rsi;"
      "popq %%rdi;"
      "popq %%r15;"
      "popq %%r14;"
      "popq %%r13;"
      "popq %%r12;"
      "popq %%r11;"
      "popq %%r10;"
      "popq %%r9;"
      "popq %%r8;"
      "popq %%rdx;"
      "popq %%rcx;"
      "popq %%rbx;"
      "popq %%rax;"

      "lretq"
      :
      : "r"(p->context.rflags), "r"(p->context.ds),"r"(p->context.es),"r"(p->context.fs),"r"(p->context.gs)
      : "%rax"
      );

  //復帰処理
  asm volatile (
      "_returner:"
      "call _saver;"
      //セグメントデスクリプタの復旧
      "movq %0,%%rax;"
      "movq %%rax,%%ds;"
      "movq %1,%%rax;"
      "movq %%rax,%%es;"
      "movq %2,%%rax;"
      "movq %%rax,%%fs;"
      "movq %3,%%rax;"
      "movq %%rax,%%gs;"

      //元のコンテキスト復旧
      //バイナリ見る限り、GCCがやっているけど。。。
      "popq %%rbp;"
      "popq %%r15;"
      "popq %%r14;"
      "popq %%r13;"
      "popq %%r12;"
      "popq %%rbx;"
      "popfq;"
      "add $0x40,%%rsp;"
      :
      : "i"(KERNEL_DS),"i"(KERNEL_DS),"i"(KERNEL_DS),"i"(KERNEL_DS)
      : "%rax"
    );
}



void ProcessCtrl::ProcessTable::Init() {
  for(int i= 0;i < max_process;i++) {
    processes[i].status = ProcessStatus::UNUSED;
  }
}

Process* ProcessCtrl::ProcessTable::AllocProcess() { 
  for(int i= 0;i < max_process;i++) {
    if(processes[i].status == ProcessStatus::UNUSED) {
      processes[i].status = ProcessStatus::EMBRYO;
      processes[i].pid = next_pid++;
      return &(processes[i]);
    }
  }
  return nullptr;
}

//TODO:デバッグ用簡易実装
//どんな実装にするのか考える
Process* ProcessCtrl::ProcessTable::GetNextProcess() {
  while(true) { 
    for(int i = current_process;i < max_process;i++) {
      if(processes[i].status != ProcessStatus::UNUSED) {
        current_process = i;
        return &(processes[i]);
      }
    }
  }
  return nullptr;
}

Process* ProcessCtrl::GetCurrentProcess() {
  return current_process;
}

//TODO:デバッグ用簡易実装
//どうやって参照を消すか？
void ProcessCtrl::HaltProcess(Process* p) {
  {
    Locker locker(table_lock);
    p->status = ProcessStatus::ZOMBIE;
  }
  p->GetKernelJob()->Wait();

}
