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

#ifndef __RAPH_LIB_ELF_H__
#define __RAPH_LIB_ELF_H__

#include <boot/multiboot2.h>
#include <elfhead.h>

class ElfLoader {
public:
  static void Load(const void *ptr);
private:
  static bool IsElf(const Elf64_Ehdr *ehdr) {
    return ehdr->e_ident[0] == ELFMAG0 && ehdr->e_ident[1] == ELFMAG1 && ehdr->e_ident[2] == ELFMAG2 && ehdr->e_ident[3] == ELFMAG3;
  }
  static bool IsElf64(const Elf64_Ehdr *ehdr) {
    return ehdr->e_ident[EI_CLASS] == ELFCLASS64;
  }
  static bool IsOsabiSysv(const Elf64_Ehdr *ehdr) {
    return ehdr->e_ident[EI_OSABI] == ELFOSABI_SYSV;
  }
  static bool IsOsabiGnu(const Elf64_Ehdr *ehdr) {
    return ehdr->e_ident[EI_OSABI] == ELFOSABI_GNU;
  }
};

#endif // __RAPH_LIB_ELF_H__
