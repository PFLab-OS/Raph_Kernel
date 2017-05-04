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

#include "ramdisk.h"
#include <multiboot.h>

void Ramdisk::Attach() {
  Ramdisk *dev = new Ramdisk("fs.img");
  if (dev->Init() != IoReturnState::kOk) {
    delete dev;
  }
}

IoReturnState Ramdisk::InitSub() {
  _image = multiboot_ctrl->LoadFile(_fname);
  if (_image.IsNull()) {
    return IoReturnState::kErrHardware;
  }
  return IoReturnState::kOk;
}

IoReturnState Ramdisk::Read(uint8_t *buf, size_t offset, size_t size) {
  if (_image->GetLen() < offset + size) {
    return IoReturnState::kErrInvalid;
  }
  return IoReturnState::kOk;
}

IoReturnState Ramdisk::Write(uint8_t *buf, size_t offset, size_t size) {
  if (_image->GetLen() < offset + size) {
    return IoReturnState::kErrInvalid;
  }
  return IoReturnState::kOk;
}
