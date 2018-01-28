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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * Author: Liva
 *
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <io.h>
#include <dev/device.h>

class Storage : public Device {
 public:
  Storage() {}
  virtual ~Storage() {}
  /*
   * Since these functions may call Task::Wait() internally, threads calling
   * these functions must be executed on TaskWithStack.
   */
  IoReturnState Init() __attribute__((warn_unused_result));
  template <class T>
  IoReturnState Read(T &buf, size_t offset) __attribute__((warn_unused_result));
  template <class T>
  IoReturnState Write(T &buf, size_t offset)
      __attribute__((warn_unused_result));
  virtual IoReturnState Read(uint8_t *buf, size_t offset, size_t size)
      __attribute__((warn_unused_result)) = 0;
  virtual IoReturnState Write(uint8_t *buf, size_t offset, size_t size)
      __attribute__((warn_unused_result)) = 0;

 protected:
  virtual IoReturnState InitSub() = 0;
  virtual size_t GetBlockSize() = 0;
  virtual const char *GetName() = 0;

 private:
};

template <class T>
IoReturnState Storage::Read(T &buf, size_t offset) {
  return Read(reinterpret_cast<uint8_t *>(&buf), offset, sizeof(T));
}

template <class T>
IoReturnState Storage::Write(T &buf, size_t offset) {
  return Write(reinterpret_cast<uint8_t *>(&buf), offset, sizeof(T));
}

class StorageCtrl {
 public:
  StorageCtrl(const StorageCtrl &) = delete;
  StorageCtrl &operator=(const StorageCtrl &) = delete;
  StorageCtrl(StorageCtrl &&) = delete;
  StorageCtrl &operator=(StorageCtrl &&) = delete;

  static StorageCtrl &GetCtrl() {
    static StorageCtrl instance;
    return instance;
  }
  IoReturnState Register(Storage *dev, const char *str)
      __attribute__((warn_unused_result));
  IoReturnState GetDevice(const char *str, Storage *&dev)
      __attribute__((warn_unused_result));

 private:
  StorageCtrl() {}
  ~StorageCtrl() {}
  static const int kElementNum = 10;
  static const int kStringLenMax = 9;
  struct Container {
    Storage *dev;
    char str[kStringLenMax + 1];
  };
  Container _container[kElementNum];
};
