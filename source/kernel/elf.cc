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
#include <elf.h>
#include <mem/physmem.h>
#include <string.h>
#include <stdlib.h>

void readElf(const void *p);
void readElfTest(struct multiboot_tag_module *module)
{
  gtty->CprintfRaw("read elf module from %x to %x\n", module->mod_start, module->mod_end);
  uint8_t *p_begin = reinterpret_cast<uint8_t *>(p2v(module->mod_start));
  //
  readElf(p_begin);
}

#define IS_ELF(ehdr)    (ehdr->e_ident[0] == ELFMAG0 && ehdr->e_ident[1] == ELFMAG1 && ehdr->e_ident[2] == ELFMAG2 && ehdr->e_ident[3] == ELFMAG3)
#define IS_ELF64(ehdr)  (ehdr->e_ident[EI_CLASS] == ELFCLASS64)
#define IS_OSABI_SYSV(ehdr)   (ehdr->e_ident[EI_OSABI] == ELFOSABI_SYSV)
#define IS_OSABI_GNU(ehdr)   (ehdr->e_ident[EI_OSABI] == ELFOSABI_GNU)

void readElf(const void *p)
{
  const uint8_t *head = reinterpret_cast<const uint8_t *>(p);
  const Elf64_Ehdr *ehdr = reinterpret_cast<const Elf64_Ehdr *>(p);
  const Elf64_Shdr *shstr;
  
  if(!IS_ELF(ehdr) || !IS_ELF64(ehdr) || (!IS_OSABI_SYSV(ehdr) && !IS_OSABI_GNU(ehdr))){
    gtty->CprintfRaw("Not supported module type.\n");
    return;
  }
  gtty->CprintfRaw("ABI: %d\n", ehdr->e_ident[EI_OSABI]);
  gtty->CprintfRaw("Entry point is 0x%08x \n", ehdr->e_entry);

  shstr = (Elf64_Shdr *)(head + ehdr->e_shoff + ehdr->e_shentsize * ehdr->e_shstrndx);
  const Elf64_Shdr *shdr = (const Elf64_Shdr *)ehdr->e_shoff;

  
  gtty->CprintfRaw("%c\n", head[0x1080]);

  using FType = int (*)(int, char*[]);
  FType f;

  

  gtty->CprintfRaw("Sections:\n");
  for(int i = 0; i < ehdr->e_shnum; i++){
    const Elf64_Shdr *shdr = (const Elf64_Shdr *)(head + ehdr->e_shoff + ehdr->e_shentsize * i);
    const char *sname = (const char *)(head + shstr->sh_offset + shdr->sh_name); 
    gtty->CprintfRaw(" [%2d] %s\n", i, sname);
    if(strcmp(sname, ".text") == 0){
      f = reinterpret_cast<FType>(malloc(shdr->sh_size));
      memcpy(reinterpret_cast<void *>(f), &head[shdr->sh_offset], shdr->sh_size);
      gtty->CprintfRaw("\n");
    }
  }

  char *argv[1] = {"123"};
  gtty->CprintfRaw("%s\n", argv[0]);
  f(1, argv); 
  gtty->CprintfRaw("%s Returned.\n", argv[0]);

}
