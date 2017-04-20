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

#include <global.h>
#include <multiboot.h>
#include "framebuffer.h"

FrameBuffer::DrawInfo FrameBuffer::_d_info;

void FrameBuffer::Setup() {
  multiboot_ctrl->SetupFrameBuffer(&_fb_info);
  assert(_fb_info.bpp == 24 || _fb_info.bpp == 32);
  
  auto font_file = multiboot_ctrl->LoadFile("font");
  _font.Load(font_file, font_file->GetLen());
  
  _info_row = (_fb_info.height / _font.GetMaxh() - 1) * _font.GetMaxh();
  _cx = 0;
  _cy = 0;

  bzero(_fb_info.buffer, _fb_info.width * _fb_info.height * (_fb_info.bpp / 8));
}

void FrameBuffer::Write(uint8_t c) {
  if (_fb_info.buffer == nullptr) {
    return;
  }
  Locker locker(_lock);
  switch(c) {
  case '\n':
    _cx = 0;
    _cy += _font.GetMaxh();
    break;
  default:
    int width = _font.GetWidth(c);
    if (_cx + width > _fb_info.width) {
      _cx = 0;
      _cy += _font.GetMaxh();
      Scroll();
    }
    {
      DrawInfo info = {
        .buf_base = _fb_info.buffer + (_cx + _cy * _fb_info.width) * (_fb_info.bpp / 8),
        .dcolor = GetColor(),
        .bcolor = 0,
        .width = _fb_info.width,
        .bpp = _fb_info.bpp,
      };
      _d_info = info;
      _font.Print(c, DrawPoint);
    }
    _cx += width;
    break;
  }
  Scroll();
}

void FrameBuffer::PrintShell(const char *str) {
  Locker locker(_lock);
  memset(_fb_info.buffer + (_fb_info.height - _font.GetMaxh()) * _fb_info.width * (_fb_info.bpp / 8), 0xff, _fb_info.width * _font.GetMaxh() * (_fb_info.bpp / 8));
  {
    DrawInfo info = {
      .buf_base = _fb_info.buffer + (_fb_info.height - _font.GetMaxh()) * _fb_info.width * (_fb_info.bpp / 8),
      .dcolor = 0,
      .bcolor = 0x00FFFFFF,
      .width = _fb_info.width,
      .bpp = _fb_info.bpp,
    };
    _d_info = info;
    _font.Print(U'>', DrawPoint);
  }
  int xoffset = _font.GetWidth(U'>');
  // TODO consider overflow 
  for (int x = 0; x < strlen(str); x++) {
    char c = ' ';
    if (*str != '\0') {
      c = *str;
      str++;
    }
    if (xoffset + _font.GetWidth(c) > _fb_info.width) {
      break;
    }
    {
      DrawInfo info = {
        .buf_base = _fb_info.buffer + (xoffset + (_fb_info.height - _font.GetMaxh()) * _fb_info.width) * (_fb_info.bpp / 8),
        .dcolor = 0,
        .bcolor = 0x00FFFFFF,
        .width = _fb_info.width,
        .bpp = _fb_info.bpp,
      };
      _d_info = info;
      _font.Print(c, DrawPoint);
    }
    xoffset += _font.GetWidth(c);
  }
}

void FrameBuffer::Scroll() {
  if (_cy >= _info_row) {
    _cy -= _font.GetMaxh();
    for (int y = 0; y < _cy / _font.GetMaxh(); y++) {
      memcpy(_fb_info.buffer + y * _font.GetMaxh() * _fb_info.width * (_fb_info.bpp / 8),
             _fb_info.buffer + (y + 1) * _font.GetMaxh() * _fb_info.width * (_fb_info.bpp / 8),
             _font.GetMaxh() * _fb_info.width * (_fb_info.bpp / 8));
    }
    bzero(_fb_info.buffer + _cy * _fb_info.width * (_fb_info.bpp / 8),
          _font.GetMaxh() * _fb_info.width * (_fb_info.bpp / 8));
  }
}
