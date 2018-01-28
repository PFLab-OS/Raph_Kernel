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

  _back_buffer =
      new uint8_t[_fb_info.width * _fb_info.height * _bytes_per_pixel];
  memset(_back_buffer, 0, _fb_info.width * _fb_info.height * _bytes_per_pixel);

  // check alignments
  assert((reinterpret_cast<size_t>(_back_buffer) % sizeof(uint64_t)) == 0);
  assert((reinterpret_cast<size_t>(_fb_info.buffer) % sizeof(uint64_t)) == 0);
  assert(((_fb_info.width * _fb_info.height * _bytes_per_pixel) %
          sizeof(uint64_t)) == 0);

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

void FrameBuffer::StringRingBuffer::Write(char32_t c) {
  assert(((_write_pos + 1) & _index_mask) != _read_pos);
  assert(((_write_pos + 2) & _index_mask) != _read_pos);
  if (c == '\n') {
    _char_buffer[_write_pos] = c;
    _write_pos = ((_write_pos + 1) & _index_mask);
    _last_row_width_used = 0;
    _row_count++;
    if (_row_count > _screen_rows) {
      // scroll
      char32_t ch;
      do {
        ch = _char_buffer[_read_pos];
        _read_pos = ((_read_pos + 1) & _index_mask);
      } while (ch != '\n');
      _row_count--;
      _scroll_count++;
    }
  } else {
    uint32_t fontIndex = _font->GetIndex(c);
    size_t fontWidth = _font->GetWidth(fontIndex);
    if (fontWidth + _last_row_width_used > _screen->width) {
      // wrap line
      Write('\n');
    }
    _char_buffer[_write_pos] = c;
    _write_pos = ((_write_pos + 1) & _index_mask);
    _last_row_width_used += fontWidth;
  }
}

int FrameBuffer::CachedFont::PrintChar(uint8_t *vram, int vramXSize, int px,
                                       int py, char32_t c) {
  if (!_is_initialized || _bytesPerPixel == 0) return 0;
  uint8_t *font;
  int fontWidth;

  if (0 <= c && c < _cachedCount && _fontCache && _widthChache) {
    // use cached font
    fontWidth = _widthChache[c];
    font = getFontCache(c);
  } else {
    // not in cache
    GetPixels(c, _bytesPerPixel, GetMaxw(), _tmpFont, fontWidth, _fColor,
              _bColor);
    font = _tmpFont;
  }

  for (int y = 0; y < GetMaxh(); y++) {
    memcpy(&vram[((y + py) * vramXSize + px) * _bytesPerPixel],
           &font[(y * GetMaxw() + 0) * _bytesPerPixel],
           _bytesPerPixel * fontWidth);
  }
  return fontWidth;
}

void FrameBuffer::CachedFont::UpdateCache() {
  delete _tmpFont;
  delete _widthChache;
  delete _fontCache;
  //
  _tmpFont = new uint8_t[GetMaxh() * GetMaxw() * _bytesPerPixel];
  _fontCache =
      new uint8_t[GetMaxh() * GetMaxw() * _bytesPerPixel * _cachedCount];
  _widthChache = new int[_cachedCount];
  for (uint32_t k = 0; k < _cachedCount; k++) {
    uint8_t *font = getFontCache(k);
    GetPixels(k, _bytesPerPixel, GetMaxw(), font, _widthChache[k], _fColor,
              _bColor);
  }
}

void FrameBuffer::Write(char c) {
  Locker locker(_lock);
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

void FrameBuffer::Refresh() {
  if (_timeup_draw) {
    if (ThreadCtrl::IsInitialized() &&
        ThreadCtrl::GetCtrl(_draw_cpuid).GetState() !=
            ThreadCtrl::QueueState::kNotStarted) {
      DisableTimeupDraw();
    } else {
      if (timer->ReadTime() >= _last_time_refresh + 33 * 1000) {
        DrawScreen();
        _last_time_refresh = timer->ReadTime();
      }
    }
  } else {
    _needs_redraw = true;
  }
}

void FrameBuffer::DoPeriodicRefresh(void *) {
  Locker locker(_lock);
  if (_needs_redraw) {
    DrawScreen();
  }
  ScheduleRefresh();
}

void FrameBuffer::DisableTimeupDraw() {
  _timeup_draw = false;
  _needs_redraw = true;

  _refresh_thread = ThreadCtrl::GetCtrl(_draw_cpuid)
                        .AllocNewThread(Thread::StackState::kShared);
  _refresh_thread->CreateOperator().SetFunc(
      make_uptr(new ClassFunction<FrameBuffer, void *>(
          this, &FrameBuffer::DoPeriodicRefresh, nullptr)));
  ScheduleRefresh();
}

void FrameBuffer::DrawScreen() {
  _needs_redraw = false;
  if (_disable_flag) return;
  //
  assert(_lock.IsLocked());
  // draw prompt
  int xoffset = _font_inverted.PrintChar(
      _back_buffer, _fb_info.width, 0,
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
          _back_buffer, _fb_info.width, xoffset,
          _fb_info.height - _font_inverted.GetMaxh(), _shell_buffer[x]);
      ;
    }
  }
  for (int y = 0; y < _font_inverted.GetMaxh(); y++) {
    memset(
        _back_buffer + (xoffset + (_fb_info.height - y - 1) * _fb_info.width) *
                           (_fb_info.bpp / 8),
        0xff, (_fb_info.width - xoffset) * (_fb_info.bpp / 8));
  }

  DrawScreenSub();
}

void FrameBuffer::DrawScreenSub() {
  size_t xsize = _fb_info.width;
  size_t row_height = _font_normal.GetMaxh();
  //
  int rp = _str_buffer->GetReadPos();
  size_t x = 0;
  size_t y = 0;
  char32_t c;
  //
  static int prev_row_count = 0;
  int skip_line_count = 0;
  //
  if (prev_row_count == _str_buffer->GetScreenRows()) {
    int scroll_count = _str_buffer->GetScrollCount();
    _str_buffer->ResetScrollCount();
    //
    if (scroll_count > 0) {
      // scroll screen
      skip_line_count = _str_buffer->GetScreenRows() - scroll_count - 1;
      if (skip_line_count <= 0) {
        skip_line_count = 0;
      } else {
        // note: last row which already written in vram may have to update.
        // So skip_line_count is a number of rows to be copied.
        int offset = scroll_count * (xsize * _bytes_per_pixel * row_height);
        int count = skip_line_count * (xsize * _bytes_per_pixel * row_height);
        assert((count & 3) == 0);

        for (int i = 0; i < count / 8; i++) {
          reinterpret_cast<uint64_t *>(_back_buffer)[i] =
              reinterpret_cast<uint64_t *>(_back_buffer)[i + offset / 8];
        }
      }
    } else {
      // if there was no scroll, only last row should be updated.
      skip_line_count = _str_buffer->GetRowCount() - 1;
    }
  } else {
    skip_line_count = prev_row_count;
  }
  prev_row_count = _str_buffer->GetRowCount();

  // move forward to rows should be draw.
  for (int i = 0; i < skip_line_count; i++) {
    while ((c = _str_buffer->ReadNextChar(rp)) !=
           StringRingBuffer::kEndMarker) {
      if (c == '\n') break;
    }
  }
  y = skip_line_count * row_height;

  // draw
  while ((c = _str_buffer->ReadNextChar(rp)) != StringRingBuffer::kEndMarker) {
    if (c == '\n') {
      for (size_t py = y; py < y + _font_normal.GetMaxh(); py++) {
        for (size_t px = x; px < _fb_info.width; px++) {
          for (size_t p = 0; p < _bytes_per_pixel; p++) {
            _back_buffer[(py * xsize + px) * _bytes_per_pixel + p] = 0;
          }
        }
      }
      x = 0;
      y += row_height;
    } else {
      x += _font_normal.PrintChar(_back_buffer, xsize, x, y, c);
    }
  }
  if (x < xsize && y < _fb_info.height) {
    for (size_t py = y; py < y + _font_normal.GetMaxh(); py++) {
      for (size_t px = x; px < _fb_info.width; px++) {
        for (size_t p = 0; p < _bytes_per_pixel; p++) {
          _back_buffer[(py * xsize + px) * _bytes_per_pixel + p] = 0;
        }
      }
    }
  }

  // copy back buffer to front buffer
  for (size_t offset = 0;
       offset <
       (_fb_info.width * _fb_info.height * _bytes_per_pixel) / sizeof(uint64_t);
       offset++) {
    reinterpret_cast<uint64_t *>(_fb_info.buffer)[offset] =
        reinterpret_cast<uint64_t *>(_back_buffer)[offset];
  }
}
