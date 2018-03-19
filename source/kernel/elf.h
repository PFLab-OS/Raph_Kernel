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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * Author: hikalium, Liva
 *
 */

#pragma once

#include <elfhead.h>
#include <loader.h>
#include <ptr.h>
#include <array.h>
#include <bin.h>

class ElfObject : public ExecutableObject {
 public:
  ElfObject(Loader &loader, const void *ptr)
      : _head(reinterpret_cast<const uint8_t *>(ptr)),
        _ehdr(reinterpret_cast<const Elf64_Ehdr *>(ptr)),
        _membuffer(nullptr),
        _loader(loader) {}
  ElfObject() = delete;
  virtual ~ElfObject() { free(_membuffer); }
  virtual ErrorState Init() override __attribute__((warn_unused_result));
  void Execute() {
    if (_entry != nullptr) {
      _loader.Execute(_entry);
    }
  }
  void Resume() { _loader.Resume(); }

  void ReturnToKernelJob() { _loader.ExitResume(); }
  // TODO: redesigning
  void SetContext(Context *context) { _loader.SetContext(context); }
  Context *GetContext() { return &(_loader.context); }

 protected:
  bool IsElf() {
    return _ehdr->e_ident[0] == ELFMAG0 && _ehdr->e_ident[1] == ELFMAG1 &&
           _ehdr->e_ident[2] == ELFMAG2 && _ehdr->e_ident[3] == ELFMAG3;
  }
  bool IsElf64() { return _ehdr->e_ident[EI_CLASS] == ELFCLASS64; }
  bool IsOsabiSysv() { return _ehdr->e_ident[EI_OSABI] == ELFOSABI_SYSV; }
  bool IsOsabiGnu() { return _ehdr->e_ident[EI_OSABI] == ELFOSABI_GNU; }
  virtual ErrorState LoadMemory(bool page_mapping);
  const uint8_t *_head;
  const Elf64_Ehdr *_ehdr;
  uint8_t *_membuffer;
  Loader &_loader;
  FType _entry = nullptr;
  static const bool kDebugOutput = false;
};

class RaphineRing0AppObject : public ElfObject {
 public:
  RaphineRing0AppObject(Loader &loader, const void *ptr)
      : ElfObject(loader, ptr) {}
  RaphineRing0AppObject() = delete;
  virtual ErrorState Init() override __attribute__((warn_unused_result));

 private:
  virtual ErrorState LoadMemory(bool page_mapping) override;
  struct Info {
    uint64_t version;
    void (*putc)(char c);
  };
  static void putc(char c);
  static Info _info;
  static const int kInfoSize = 4096;
  static const uint64_t kMajorVersion = 1;
  static const uint64_t kMinorVersion = 0;
  static const uint64_t kVersion = (kMajorVersion << 32) + kMinorVersion;
};
