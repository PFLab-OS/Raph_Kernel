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
 * graphic driver for framebuffer
 * 
 */

#pragma once

#include <global.h>
#include <raph.h>
#include <tty.h>
#include <font.h>
#include <stdint.h>
#include <spinlock.h>

struct FrameBufferInfo {
  uint8_t *buffer;
  uint32_t width;
  uint32_t height;
  uint32_t bpp;
};

class FrameBuffer : public Tty {
public:
  FrameBuffer() {
    _fb_info.buffer = nullptr;
    ResetColor();
  }
  void Setup();
  virtual int GetRow() override {
    return 0;
  }
  virtual int GetColumn() override {
    return 0;
  }
private:
  virtual void Write(uint8_t c) override;
  virtual void PrintShell(const char *str) override;
  struct DrawInfo {
    uint8_t *buf_base;
    uint32_t color;
    uint32_t width;
    uint32_t bpp;
  };
  static void DrawPoint(int x, int y) {
    _d_info.buf_base[(y * _d_info.width + x) * (_d_info.bpp / 8)] = _d_info.color;
    _d_info.buf_base[(y * _d_info.width + x) * (_d_info.bpp / 8) + 1] = _d_info.color >> 8;
    _d_info.buf_base[(y * _d_info.width + x) * (_d_info.bpp / 8) + 2] = _d_info.color >> 16;
  }
  uint32_t GetColor() {
    return 0x00FFFFFF;
  }
  void Scroll();
  SpinLock _lock;
  FrameBufferInfo _fb_info;
  Font _font;
  int _column;
  int _info_row; // row of information display area
  int _cx, _cy; // current position of the cursor

  // frameinfo to path DrawPoint()
  static DrawInfo _d_info;
};
