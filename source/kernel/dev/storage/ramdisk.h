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

#pragma once

#include "storage.h"
#include <ptr.h>
#include <array.h>

class Ramdisk : public Storage {
public:
  Ramdisk() = delete;
  Ramdisk(const char *str) : _fname(str) {
  }
  virtual ~Ramdisk() {
  }
  static void Attach();
private:
  virtual IoReturnState InitSub() override;
  virtual IoReturnState Read(uint8_t *buf, size_t offset, size_t size) override;
  virtual IoReturnState Write(uint8_t *buf, size_t offset, size_t size) override;
  virtual size_t GetBlockSize() {
    return 1;
  }
  virtual const char *GetName() {
    return "ram";
  }
  uptr<Array<uint8_t>> _image;
  const char *_fname;
};
