/*
 *
 * Copyright (c) 2015 Project Raphine
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

#ifndef __RAPH_KERNEL_VGA_H__
#define __RAPH_KERNEL_VGA_H__

#include "../global.h"
#include "../mem/physmem.h"
#include "../mem/paging.h"
#include "../raph.h"

class Vga : public Tty {
 public:
  Vga() {
    kassert(physmem_ctrl != nullptr);
    
    phys_addr vga_addr = 0xb8000;
    // TODO : virt_addrに対してphys_addrを突っ込んでる
    // TODO : サイズ適当
    physmem_ctrl->Reserve(PagingCtrl::RoundAddrOnPageBoundary(vga_addr), PagingCtrl::RoundUpAddrOnPageBoundary(0x1000));
    _vga_addr = reinterpret_cast<uint8_t *>(p2v(vga_addr));
    _cx = 0;
    _cy = 0;
  }
 private:
  virtual void Write(uint8_t c) override {
    // TODO ページ送り
    switch(c) {
    case '\n':
      _cx = 0;
      _cy++;
      break;
    default:
      _vga_addr[(_cy * _x + _cx) * 2] = c;
      _vga_addr[(_cy * _x + _cx) * 2 + 1] = 0x0f;
      _cx++;
      if (_cx == _x) {
        _cx = 0;
        _cy++;
      }
    }
  }
  uint8_t *_vga_addr;
  static const int _x = 80;
  static const int _y = 25;
  int _cx;
  int _cy;
};

#endif // __RAPH_KERNEL_VGA_H__
