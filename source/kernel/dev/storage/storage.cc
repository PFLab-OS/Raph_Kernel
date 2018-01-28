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

#include "storage.h"
#include <string.h>

IoReturnState Storage::Init() {
  IoReturnState rs = InitSub();
  if (rs != IoReturnState::kOk) {
    return rs;
  }
  return StorageCtrl::GetCtrl().Register(this, GetName());
}

IoReturnState StorageCtrl::Register(Storage *dev, const char *str) {
  for (int i = 0; i < kElementNum; i++) {
    if (_container[i].dev == nullptr) {
      _container[i].dev = dev;
      size_t len = strlen(str);
      if (len > kStringLenMax - 1) {
        len = kStringLenMax - 1;
      }
      memcpy(_container[i].str, str, len);
      _container[i].str[len] = '0' + i;
      _container[i].str[len + 1] = '\0';
      return IoReturnState::kOk;
    }
  }
  return IoReturnState::kErrNoSwResource;
}

IoReturnState StorageCtrl::GetDevice(const char *str, Storage *&dev) {
  for (int i = 0; i < kElementNum; i++) {
    if (_container[i].dev == nullptr) {
      continue;
    }
    if (strcmp(str, _container[i].str) == 0) {
      dev = _container[i].dev;
      return IoReturnState::kOk;
    }
  }
  return IoReturnState::kErrNotFound;
}
