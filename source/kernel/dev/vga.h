/*
 *
 * Copyright (c) 2015 Raphine Project
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

#include <global.h>
#include <mem/physmem.h>
#include <mem/paging.h>
#include <raph.h>
#include <tty.h>

class Vga : public Tty {
 public:
  Vga() {
    kassert(physmem_ctrl != nullptr);
    
    phys_addr vga_addr = 0xb8000;
    ResetColor();
    // TODO : virt_addrに対してphys_addrを突っ込んでる
    // TODO : サイズ適当
    physmem_ctrl->Reserve(PagingCtrl::RoundAddrOnPageBoundary(vga_addr), PagingCtrl::RoundUpAddrOnPageBoundary(0x1000));
    _vga_addr = reinterpret_cast<uint8_t *>(p2v(vga_addr));
  }
  void SetColor(Color c) override {
    _color = c;
  }
  void ResetColor() override {
    _color = Color::kWhite;
  }
 private:
  virtual void _Init() override {
    // turn off bilinking
    inb(0x3DA);
    outb(0x3C0, 0x10 + 0x20);
    outb(0x3C0, inb(0x3C1) & ~0x08);
    
    PrintShell("");
  }
  virtual void Write(uint8_t c) override {
    switch(c) {
    case '\n':
      for (int x = _cx; x < _x; x++) {
        _vga_addr[(_cy * _x + x) * 2] = ' ';
        _vga_addr[(_cy * _x + x) * 2 + 1] = static_cast<uint8_t>(_color);
      }
      _cx = 0;
      _cy++;
      break;
    default:
      _vga_addr[(_cy * _x + _cx) * 2] = c;
      _vga_addr[(_cy * _x + _cx) * 2 + 1] = static_cast<uint8_t>(_color);
      _cx++;
      if (_cx == _x) {
        _cx = 0;
        _cy++;
      }
    }
    Scroll();
  }
  virtual void PrintShell(const char *str) override {
    _vga_addr[((_y - 1) * _x) * 2] = '>';
    _vga_addr[((_y - 1) * _x) * 2 + 1] = 0xF0;
    size_t len = strlen(str);
    if (len >= _x - 1) {
      str += len - (_x - 1);
    }
    for (int x = 1; x < _x; x++) {
      char c = ' ';
      if (*str != '\0') {
        c = *str;
        str++;
      }
      _vga_addr[((_y - 1) * _x + x) * 2] = c;
      _vga_addr[((_y - 1) * _x + x) * 2 + 1] = 0xF0;
    }
  }
  void Scroll() {
    if (_cy == _y - 1) {
      _cy--;
      for(int y = 1; y < _y - 1; y++) {
        for(int x = 0; x < _x; x++) {
          _vga_addr[((y - 1) * _x + x) * 2] = _vga_addr[(y * _x + x) * 2];
          _vga_addr[((y - 1) * _x + x) * 2 + 1] = _vga_addr[(y * _x + x) * 2 + 1];
        }
      }
      for (int x = 0; x < _x; x++) {
        _vga_addr[(_cy * _x + x) * 2] = ' ';
        _vga_addr[(_cy * _x + x) * 2 + 1] = static_cast<uint8_t>(_color);
      }
    }
  }
  uint8_t *_vga_addr;
  Color _color;
  static const int _x = 80;
  static const int _y = 25;
};

#endif // __RAPH_KERNEL_VGA_H__
