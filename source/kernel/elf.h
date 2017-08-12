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
 * Author: hikalium, Liva
 * 
 */

#pragma once

#include <elfhead.h>
#include <loader.h>
#include <ptr.h>
#include <array.h>

class ElfObject {
public:
  ElfObject(const void *ptr) : _head(reinterpret_cast<const uint8_t *>(ptr)), _ehdr(reinterpret_cast<const Elf64_Ehdr *>(ptr)) {
  }
  ElfObject() = delete;
  void Init();
  void Load() {
    if (_entry != nullptr) {
      _loader.Execute(_entry);
    }
  }
private:
  bool IsElf() {
    return _ehdr->e_ident[0] == ELFMAG0 && _ehdr->e_ident[1] == ELFMAG1 && _ehdr->e_ident[2] == ELFMAG2 && _ehdr->e_ident[3] == ELFMAG3;
  }
  bool IsElf64() {
    return _ehdr->e_ident[EI_CLASS] == ELFCLASS64;
  }
  bool IsOsabiSysv() {
    return _ehdr->e_ident[EI_OSABI] == ELFOSABI_SYSV;
  }
  bool IsOsabiGnu() {
    return _ehdr->e_ident[EI_OSABI] == ELFOSABI_GNU;
  }
  const uint8_t *_head;
  const Elf64_Ehdr *_ehdr;
  Loader _loader;
  FType _entry = nullptr;
};

