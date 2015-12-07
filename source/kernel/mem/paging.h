/*
 *
 * Copyright (c) 2015 Raphine Project
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

#ifndef __RAPH_KERNEL_MEM_PAGING_H__
#define __RAPH_KERNEL_MEM_PAGING_H__

#define PML4E_PRESENT_BIT          (1<<0)
#define PML4E_WRITE_BIT            (1<<1)
#define PML4E_USER_BIT             (1<<2)
#define PML4E_WRITETHROUGH_BIT     (1<<3)
#define PML4E_CACHEDISABLE_BIT     (1<<4)
#define PML4E_ACCESSED_BIT         (1<<5)

#define PDPTE_PRESENT_BIT          (1<<0)
#define PDPTE_WRITE_BIT            (1<<1)
#define PDPTE_USER_BIT             (1<<2)
#define PDPTE_WRITETHROUGH_BIT     (1<<3)
#define PDPTE_CACHEDISABLE_BIT     (1<<4)
#define PDPTE_ACCESSED_BIT         (1<<5)
#define PDPTE_DIRTY_BIT            (1<<6)  // 1GPAGE_BIT==1の時のみ
#define PDPTE_1GPAGE_BIT           (1<<7)
#define PDPTE_GLOBAL_BIT           (1<<8)  // 1GPAGE_BIT==1の時のみ

#define PDE_PRESENT_BIT            (1<<0)
#define PDE_WRITE_BIT              (1<<1)
#define PDE_USER_BIT               (1<<2)
#define PDE_WRITETHROUGH_BIT       (1<<3)
#define PDE_CACHEDISABLE_BIT       (1<<4)
#define PDE_ACCESSED_BIT           (1<<5)
#define PDE_DIRTY_BIT              (1<<6)  // valid when 2MPAGE_BIT==1
#define PDE_2MPAGE_BIT             (1<<7)
#define PDE_GLOBAL_BIT             (1<<8)  // valid when 2MPAGE_BIT==1

#define PTE_PRESENT_BIT            (1<<0)
#define PTE_WRITE_BIT              (1<<1)
#define PTE_USER_BIT               (1<<2)
#define PTE_WRITETHROUGH_BIT       (1<<3)
#define PTE_CACHEDISABLE_BIT       (1<<4)
#define PTE_ACCESSED_BIT           (1<<5)
#define PTE_DIRTY_BIT              (1<<6)
#define PTE_GLOBAL_BIT             (1<<8)

#ifndef ASM_FILE

#include <assert.h>
#include "../spinlock.h"
#include "physmem.h"
#include "virtmem.h"

typedef uint64_t entry_type;

class PagingCtrl {
 public:
  PagingCtrl();
  bool IsVirtAddrMapped(virt_addr vaddr);
  // 物理ページを仮想メモリにマッピングする
  // 既にマッピングされていた場合はfalseを返す
  // 物理メモリの確保はPhysmemCtrlで事前に行っておくこと
  // flagには、present_bit以外の全ての立てたいbitを指定すること
  //
  //   pst_flag: page structure table entryに立てるフラグの指定
  //             pstが既に設定済みの場合、フラグの上書き等はしない
  //   page_flag: page entryに立てるフラグの指定
  //
  // write_bitを立てるのを忘れないように
  bool MapPhysAddrToVirtAddr(virt_addr vaddr, PhysAddr &paddr, phys_addr pst_flag, phys_addr page_flag);
  // マッピングした仮想メモリを解放する
  void UnmapVirtAddr(virt_addr addr);
  // kHeapEndAddr以降から空き領域を探す
  virt_addr SearchUnmappedArea(size_t size);
  // 数字をページサイズに広げる
  static int ConvertNumToPageSize(int size) {
    return ((size + kPageSize - 1) / kPageSize) * kPageSize;
  }
  static virt_addr RoundUpAddrOnPageBoundary(virt_addr addr) {
    return RoundAddrOnPageBoundary(addr + kPageSize - 1);
  }
  static virt_addr RoundAddrOnPageBoundary(virt_addr addr) {
    return (addr / kPageSize) * kPageSize;
  }
  static int GetPML4TIndex(virt_addr addr) {
    return (addr >> 39) & ((1 << 9) - 1);
  }
  static int GetPDPTIndex(virt_addr addr) {
    return (addr >> 30) & ((1 << 9) - 1);
  }
  static int GetPDIndex(virt_addr addr) {
    return (addr >> 21) & ((1 << 9) - 1);
  }
  static int GetPTIndex(virt_addr addr) {
    return (addr >> 12) & ((1 << 9) - 1);
  }
  static int GetPageOffset(virt_addr addr) {
    return addr & ((1 << 12) - 1);
  }
  static const int kPageSize = 0x1000;
  #ifdef __UNIT_TEST__
  entry_type *_pml4t;

  // page structure tablesのindex情報を元に仮想アドレスを算出する
  static virt_addr CalcVirtAddrFromStructureTableOffset(int pml4_index, int pdpt_index, int pd_index, int pt_index, int offset) {
    assert(IsPageStructureTableIndex(pml4_index));
    assert(IsPageStructureTableIndex(pdpt_index));
    assert(IsPageStructureTableIndex(pd_index));
    assert(IsPageStructureTableIndex(pt_index));
    entry_type *pdpt = reinterpret_cast<entry_type *>(RoundAddrOnPageBoundary(_pml4t[pml4_index]));
    entry_type *pd = reinterpret_cast<entry_type *>(RoundAddrOnPageBoundary(pdpt[pdpt_index]));
    entry_type *pt = reinterpret_cast<entry_type *>(RoundAddrOnPageBoundary(pd[pd_index]));
    entry_type *page = reinterpret_cast<entry_type *>(RoundAddrOnPageBoundary(pt[pt_index]));
    return reinterpret_cast<virt_addr>(page) + offset;
  }
  #else
private:
  // page structure tablesのindex情報を元に仮想アドレスを算出する
  static virt_addr CalcVirtAddrFromStructureTableOffset(int pml4_index, int pdpt_index, int pd_index, int pt_index, int offset) {
    assert(IsPageStructureTableIndex(pml4_index));
    assert(IsPageStructureTableIndex(pdpt_index));
    assert(IsPageStructureTableIndex(pd_index));
    assert(IsPageStructureTableIndex(pt_index));
    return (static_cast<virt_addr>(pml4_index) << 39) | (static_cast<virt_addr>(pdpt_index) << 30) | (static_cast<virt_addr>(pd_index) << 21) << (static_cast<virt_addr>(pt_index) << 12) | offset;
  }
  #endif
  static bool IsPageStructureTableIndex(int index) {
    return index >= 0 && index < 512;
  }
  static bool IsPageOffset(int offset) {
    return offset >= 0 && offset < 4096;
  }
};

#endif // ! ASM_FILE

#endif // __RAPH_KERNEL_MEM_PAGING_H__
