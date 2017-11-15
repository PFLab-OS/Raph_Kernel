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
 * Author: Liva, hikalium
 *
 * graphic driver for framebuffer
 *
 */

#include <global.h>
#include <multiboot.h>
#include <cpu.h>
#include "framebuffer.h"

FrameBuffer::DrawInfo FrameBuffer::_d_info;

void FrameBuffer::Init() {
  multiboot_ctrl->SetupFrameBuffer(&_fb_info);
  assert(_fb_info.bpp == 24 || _fb_info.bpp == 32);
  _bytes_per_pixel = _fb_info.bpp / 8;

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
  _str_buffer = new StringRingBuffer(&_fb_info, &_font_normal);
  _draw_cpuid = cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority);
}

void FrameBuffer::Write(char c) {
  Locker locker(_lock);
  //_info_buffer->Write(c);
  _str_buffer->Write(c);
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
      uint8_t *buf =
          _fb_info.buffer +
          (_cex + x + (_cey + y) * _fb_info.width) * (_fb_info.bpp / 8);
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
  _cex += width;
}

void FrameBuffer::PrintShell(const char *str) {
  Locker locker(_lock);
  if (_shell_buffer_len < strlen(str) + 1) {
    delete[] _shell_buffer;
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
  _refresh_callout->Init(make_uptr(new ClassFunction<FrameBuffer, void *>(
      this, &FrameBuffer::DoPeriodicRefresh, nullptr)));
  ScheduleRefresh();
}

void FrameBuffer::DrawScreen() {
  _needsRedraw = false;
  if (_disable_flag) return;
  //
  assert(_lock.IsLocked());
  // draw prompt
  int xoffset = _font_inverted.PrintChar(
      _fb_info.buffer, _fb_info.width, 0,
      _fb_info.height - _font_inverted.GetMaxh(), U'>');

  if (_shell_buffer) {
    // TODO consider overflow
    for (size_t x = 0; x < strlen(_shell_buffer); x++) {
      if (_shell_buffer[x] == '\0') {
        return;
      }
      size_t width =
          _font_inverted.GetWidth(static_cast<char32_t>(_shell_buffer[x]));
      if (xoffset + width > _fb_info.width) {
        break;
      }
      xoffset += _font_inverted.PrintChar(
          _fb_info.buffer, _fb_info.width, xoffset,
          _fb_info.height - _font_inverted.GetMaxh(), _shell_buffer[x]);
      ;
    }
  }
  for (int y = 0; y < _font_inverted.GetMaxh(); y++) {
    memset(_fb_info.buffer +
               (xoffset + (_fb_info.height - y - 1) * _fb_info.width) *
                   (_fb_info.bpp / 8),
           0xff, (_fb_info.width - xoffset) * (_fb_info.bpp / 8));
  }

  DrawScreenSub();
}

void FrameBuffer::DrawScreenSub() {
  uint8_t *vram = _fb_info.buffer;
  size_t xsize = _fb_info.width;
  size_t rowHeight = _font_normal.GetMaxh();
  //
  int rp = _str_buffer->GetReadPos();
  size_t x = 0;
  size_t y = 0;
  char32_t c;
  //
  static int prevRowCount = 0;
  int skipLineCount = 0;
  //
  if (prevRowCount == _str_buffer->GetScreenRows()) {
    int scrollCount = _str_buffer->GetScrollCount();
    _str_buffer->ResetScrollCount();
    //
    if (scrollCount > 0) {
      // scroll screen
      skipLineCount = _str_buffer->GetScreenRows() - scrollCount - 1;
      if (skipLineCount <= 0) {
        skipLineCount = 0;
      } else {
        // note: last row which already written in vram may have to update.
        // So skipLineCount is a number of rows to be copied.
        int offset = scrollCount * (xsize * _bytes_per_pixel * rowHeight);
        int count = skipLineCount * (xsize * _bytes_per_pixel * rowHeight);
        assert(((uint64_t)vram & 3) == 0);
        assert(((uint64_t)(vram + offset) & 3) == 0);
        assert((count & 3) == 0);

        for (int i = 0; i < count / 8; i++) {
          ((uint64_t *)vram)[i] = ((uint64_t *)vram)[i + offset / 8];
        }
      }
    } else {
      // if there was no scroll, only last row should be updated.
      skipLineCount = _str_buffer->GetRowCount() - 1;
    }
  } else {
    skipLineCount = prevRowCount;
  }
  prevRowCount = _str_buffer->GetRowCount();

  // move forward to rows should be draw.
  for (int i = 0; i < skipLineCount; i++) {
    while ((c = _str_buffer->ReadNextChar(rp)) !=
           StringRingBuffer::kEndMarker) {
      if (c == '\n') break;
    }
  }
  y = skipLineCount * rowHeight;

  // draw
  while ((c = _str_buffer->ReadNextChar(rp)) != StringRingBuffer::kEndMarker) {
    if (c == '\n') {
      for (size_t py = y; py < y + _font_normal.GetMaxh(); py++) {
        for (size_t px = x; px < _fb_info.width; px++) {
          for (size_t p = 0; p < _bytes_per_pixel; p++) {
            vram[(py * xsize + px) * _bytes_per_pixel + p] = 0;
          }
        }
      }
      x = 0;
      y += rowHeight;
    } else {
      x += _font_normal.PrintChar(vram, xsize, x, y, c);
    }
  }
  if (x < xsize && y < _fb_info.height) {
    for (size_t py = y; py < y + _font_normal.GetMaxh(); py++) {
      for (size_t px = x; px < _fb_info.width; px++) {
        for (size_t p = 0; p < _bytes_per_pixel; p++) {
          vram[(py * xsize + px) * _bytes_per_pixel + p] = 0;
        }
      }
    }
  }
}

