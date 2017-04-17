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

void FrameBuffer::Setup() {
  multiboot_ctrl->SetupFrameBuffer(&_info);
  assert(_info.bpp == 32);  // TODO : support modes other than 32bit true color
  
  auto font_file = multiboot_ctrl->LoadFile("font"); 
  _font.Load(reinterpret_cast<char *>(font_file.GetRawPtr()->GetRawPtr()));
  
  _info_row = (_info.height / _font.GetMaxh() - 1) * _font.GetMaxh();
  _cx = 0;
  _cy = 0;
}

void FrameBuffer::Write(uint8_t c) {
  if (_info.buffer == nullptr) {
    return;
  }
  switch(c) {
  case '\n':
    _cx = 0;
    _cy += _font.GetMaxh();
    break;
  default:
    int width = _font.GetWidth(c);
    if (_cx + width > _info.width) {
      _cx = 0;
      _cy += _font.GetMaxh();
      Scroll();
    }
    DrawInfo info = {
      .buf_base = _info.buffer + _cx * _cy * _info.width * 4,
      .dcolor = GetColor(),
      .bcolor = 0,
      .width = _info.width,
    };
    _font.Print(c, make_uptr(new Function<DrawInfo, bool, int, int>(DrawPoint, info)));
    _cx += width;
  }
  Scroll();
}

void FrameBuffer::PrintShell(const char *str) {
  {
    DrawInfo info = {
      .buf_base = _info.buffer + (_info.height - _font.GetMaxh()) * _info.width * 4,
      .dcolor = 0,
      .bcolor = 0x00FFFFFF,
      .width = _info.width,
    };
    _font.Print(U'>', make_uptr(new Function<DrawInfo, bool, int, int>(DrawPoint, info)));
  }
  int xoffset = _font.GetWidth(U'>');
  // TODO consider overflow 
  for (int x = 0; x < strlen(str); x++) {
    char c = ' ';
    if (*str != '\0') {
      c = *str;
      str++;
    }
    if (xoffset + _font.GetWidth(c) > _info.width) {
      break;
    }
    DrawInfo info = {
      .buf_base = _info.buffer + xoffset * (_info.height - _font.GetMaxh()) * _info.width * 4,
      .dcolor = 0,
      .bcolor = 0x00FFFFFF,
      .width = _info.width,
    };
    _font.Print(c, make_uptr(new Function<DrawInfo, bool, int, int>(DrawPoint, info)));
  }
}

void FrameBuffer::Scroll() {
  if (_cy == _info_row) {
    _cy -= _font.GetMaxh();
    for (int y = 0; y < _cy / _font.GetMaxh(); y++) {
      memcpy(_info.buffer + y * _font.GetMaxh() * _info.width * 4,
             _info.buffer + (y + 1) * _font.GetMaxh() * _info.width * 4,
             _font.GetMaxh() * _info.width * 4);
    }
    bzero(_info.buffer + _cy * _info.width * 4,
          _font.GetMaxh() * _info.width * 4);
  }
}
