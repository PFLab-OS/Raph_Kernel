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
#include <cpu.h>
#include "framebuffer.h"

void FrameBuffer::Setup() {
  multiboot_ctrl->SetupFrameBuffer(&_fb_info);
  assert(_fb_info.bpp == 24 || _fb_info.bpp == 32);

  // Load font for normal area
  {
    auto font_file = multiboot_ctrl->LoadFile("font");
    _font_normal.Load(font_file, font_file->GetLen());
    _font_normal.SetBitsPerPixel(_fb_info.bpp);
    _font_normal.SetForegroundColor(0xff, 0xff, 0xff);
    _font_normal.SetBackgroundColor(0x00, 0x00, 0x00);
    _font_normal.UpdateCache();
  }

  // Load font for prompt area
  {
    auto font_file = multiboot_ctrl->LoadFile("font");
    _font_inverted.Load(font_file, font_file->GetLen());
    _font_inverted.SetBitsPerPixel(_fb_info.bpp);
    _font_inverted.SetForegroundColor(0x00, 0x00, 0x00);
    _font_inverted.SetBackgroundColor(0xff, 0xff, 0xff);
    _font_inverted.UpdateCache();
  }

  _font_buffer = new bool[_font_normal.GetMaxh() * _font_normal.GetMaxw()];
  _info_row = (_fb_info.height / _font_normal.GetMaxh() - 1) * _font_normal.GetMaxh();

  _info_buffer = new StringBuffer(&_fb_info, &_font_normal);
  
  _draw_cpuid = cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority);
}

void FrameBuffer::Write(char c) {
  Locker locker(_lock);
  _info_buffer->Write(c);
  Refresh();
}

void FrameBuffer::WriteErr(char c) {
  const int maxh = _font_normal.GetMaxh();
  if (c == '\n') {
    _cex = 0;
    _cey += maxh;
    return;
  }
    
  int width;
  _font_normal.GetData(c, width, _font_buffer);
  if (_cex + width >= _fb_info.width) {
    _cex = 0;
    _cey += maxh;
  }
  if (_cey + maxh >= _fb_info.height) {
    return;
  }
  for (int y = 0; y < maxh; y++) {
    for (int x = 0; x < width; x++) {
      uint8_t *buf = _fb_info.buffer + (_cex + x + (_cey + y) * _fb_info.width) * (_fb_info.bpp / 8);
      if (_font_buffer[y * width + x]) {
        buf[0] = 0xff;
        buf[1] = 0xff;
        buf[2] = 0xff;
      } else {
        buf[0] = 0;
        buf[1] = 0;
        buf[2] = 0xff;
      }
    }
  }
  _cex+=width;
}

void FrameBuffer::PrintShell(const char *str) {
  Locker locker(_lock);
  if (_shell_buffer_len < strlen(str) + 1) {
    delete [] _shell_buffer;
    _shell_buffer = new char[(strlen(str) + 1) * 2];
  }
  strcpy(_shell_buffer, str);
  Refresh();
}

void FrameBuffer::DoPeriodicRefresh(void *) {
  Locker locker(_lock);
  if (_needsRedraw) {
    DrawScreen();
  }
  ScheduleRefresh();
}

void FrameBuffer::DisableTimeupDraw() {
  _timeup_draw = false;
  _needsRedraw = true;
    
  _refresh_callout = make_sptr(new Callout);
  _refresh_callout->Init(make_uptr(new ClassFunction<FrameBuffer, void *>(this, &FrameBuffer::DoPeriodicRefresh, nullptr)));
  ScheduleRefresh();
}

void FrameBuffer::DrawScreen() {
  _needsRedraw = false;
  if (_disable_flag) return;
  //
  assert(_lock.IsLocked());
  // draw prompt 
  int xoffset = _font_inverted.PrintChar(_fb_info.buffer, _fb_info.width,
                                         0, _fb_info.height - _font_inverted.GetMaxh(), U'>');

  if (_shell_buffer) {
    // TODO consider overflow 
    for (int x = 0; x < strlen(_shell_buffer); x++) {
      if (_shell_buffer[x] == '\0') {
        return;
      }
      int width = _font_inverted.GetWidth(static_cast<char32_t>(_shell_buffer[x]));
      if (xoffset + width > _fb_info.width) {
        break;
      }
      xoffset += _font_inverted.PrintChar(_fb_info.buffer, _fb_info.width,
                                          xoffset, _fb_info.height - _font_inverted.GetMaxh(), _shell_buffer[x]); ;
    }
  }
  for (int y = 0; y < _font_inverted.GetMaxh(); y++) {
    memset(_fb_info.buffer + (xoffset + (_fb_info.height - y - 1) * _fb_info.width) * (_fb_info.bpp / 8), 0xff, (_fb_info.width - xoffset) * (_fb_info.bpp / 8));
  }

  DrawScreenSub();
}

void FrameBuffer::DrawScreenSub() {
  int row = 0;
  // count sum 
  do {
    StringBuffer *buf = _info_buffer;
    while(true) {
      row += buf->_row * _font_normal.GetMaxh();
      if (buf->_next == nullptr) {
        if (buf->_buffer[buf->_length - 1] == StringBuffer::kNewLineSignature) {
          row--;
        }
        break;
      }
      buf = buf->_next;
    }
  } while(0);

  while(row - _info_buffer->_row * _font_normal.GetMaxh() >= _info_row) {
    StringBuffer *buf = _info_buffer;
    _info_buffer = buf->_next;
    buf->_next = nullptr;
    row -= buf->_row * _font_normal.GetMaxh();
    delete buf;
  }
   
  StringBuffer *buf = _info_buffer;
  int index = 0;
  while(row >= _info_row) {
    for (; index < buf->_length; index++) {
      if (buf->_buffer[index] == StringBuffer::kNewLineSignature) {
        row -= _font_normal.GetMaxh();
        index++;
        break;
      }
    }
    if (index == buf->_length) {
      buf = buf->_next;
      index = 0;
    }
    assert(buf != nullptr);
  }
  int y = 0;
  int x = 0;
  while(buf != nullptr) {
    for(; index < buf->_length; index++) {
      uint32_t i = buf->_buffer[index];
      char32_t c = buf->_cBuffer[index];
      if (i == StringBuffer::kNewLineSignature) {
        int ny = y + _font_normal.GetMaxh();
        for (; y < ny; y++) {
          bzero(_fb_info.buffer + (x + y * _fb_info.width) * (_fb_info.bpp / 8),
                (_fb_info.width - x) * (_fb_info.bpp / 8));
        }
        x = 0;
      } else {
        x += _font_normal.PrintChar(_fb_info.buffer, _fb_info.width, x, y, c);
      }
    }
      
    buf = buf->_next;
    index = 0;
  }
  if (y < _info_row) {
    int ny = y + _font_normal.GetMaxh();
    for (; y < ny; y++) {
      bzero(_fb_info.buffer + (x + y * _fb_info.width) * (_fb_info.bpp / 8), (_fb_info.width - x) * (_fb_info.bpp / 8));
    }
  } else if (y == _info_row) {
    assert(x == 0);
  } else {
    assert(false);
  }
}

