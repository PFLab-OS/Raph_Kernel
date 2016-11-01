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
 * Author: Liva, hikalium
 * 
 */

#ifndef __RAPH_LIB__CPU_H__
#define __RAPH_LIB__CPU_H__

#include <global.h>

class CpuId {
public:
  static const int kCpuIdNotFound = -1;
  static const int kCpuIdBootProcessor = 0;
  CpuId() {
    Init(kCpuIdNotFound);
  }
  explicit CpuId(int newid) {
    Init(newid);
    CheckIfValid();
  }
  int GetRawId() {
    CheckIfValid();
    return _rawid;
  }
  uint8_t GetApicId();
  bool IsValid();
private:
  void CheckIfValid();
  int _rawid;
  void Init(int newid) {
    _rawid = newid;
  }
};


#endif // __RAPH_LIB__CPU_H__
