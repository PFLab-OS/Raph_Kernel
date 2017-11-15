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

#pragma once

#include <global.h>
#include <raph.h>
#include <tty.h>
#include <task.h>
#include <font.h>
#include <stdint.h>
#include <spinlock.h>
#include <ptr.h>

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
  virtual void Init();
  virtual int GetRow() override { return 0; }
  virtual int GetColumn() override { return 0; }

 private:
  class StringRingBuffer {
   public:
    const static char32_t kEndMarker = 0xffffffff;
    StringRingBuffer() = delete;
    StringRingBuffer(const FrameBufferInfo *info, const Font *font)
        : _screen(info), _font(font) {
      _buffer_size_in_log = 12;
      _index_mask = (1 << _buffer_size_in_log) - 1;
      _char_buffer = new char32_t[1 << _buffer_size_in_log];
      _screen_rows = _screen->height / _font->GetMaxh() - 1;
    }
    ~StringRingBuffer() {
      if (_char_buffer) {
        delete _char_buffer;
      }
    }
    int GetReadPos() { return _read_pos; }
    char32_t ReadNextChar(int &readPos) {
      if (readPos == _write_pos) {
        return kEndMarker;
      }
      int prevPos = readPos;
      readPos = ((readPos + 1) & _index_mask);
      return _char_buffer[prevPos];
    }
    void Write(char32_t c) {
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
    int GetScrollCount() { return _scroll_count; }
    void ResetScrollCount() { _scroll_count = 0; }
    int GetScreenRows() { return _screen_rows; }
    int GetRowCount() { return _row_count; }

   private:
    const FrameBufferInfo *_screen;
    int _screen_rows;
    const Font *_font;
    //
    int _buffer_size_in_log;
    int _index_mask;
    char32_t *_char_buffer = nullptr;
    //
    int _write_pos = 0;
    int _read_pos = 0;
    //
    int _row_count = 1;
    int _last_row_width_used = 0;
    int _scroll_count = 0;
  } * _str_buffer;

  class CachedFont : public Font {
   public:
    CachedFont() {}
    ~CachedFont() {
      delete _widthChache;
      delete _fontCache;
    }
    void SetBitsPerPixel(int bitsPerPixel) {
      _bytesPerPixel = bitsPerPixel / 8;
      UpdateCache();
    }

    int PrintChar(uint8_t *vram, int vramXSize, int px, int py, char32_t c) {
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
    void SetForegroundColor(uint8_t r, uint8_t g, uint8_t b) {
      _fColor[0] = b;
      _fColor[1] = g;
      _fColor[2] = r;
      _fColor[3] = 0x00;
    }
    void SetBackgroundColor(uint8_t r, uint8_t g, uint8_t b) {
      _bColor[0] = b;
      _bColor[1] = g;
      _bColor[2] = r;
      _bColor[3] = 0x00;
    }
    void UpdateCache() {
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

   private:
    int _bytesPerPixel = 0;
    uint8_t _fColor[4];
    uint8_t _bColor[4];
    const uint32_t _cachedCount = 0x100;
    uint8_t *_fontCache = nullptr;
    int *_widthChache = nullptr;
    uint8_t *_tmpFont;
    uint8_t *getFontCache(char32_t c) {
      if (c < 0 || _cachedCount <= c) return nullptr;
      return &_fontCache[GetMaxh() * GetMaxw() * _bytesPerPixel * c];
    }
  };

  virtual void Write(char c) override;
  virtual void WriteErr(char c) override;
  virtual void PrintShell(const char *str) override;
  virtual void FlushSub() override {
    Locker locker(_lock);
    DrawScreen();
    if (_timeup_draw) {
      _last_time_refresh = timer->ReadMainCnt();
    } else {
      _needsRedraw = false;
    }
  }
  struct DrawInfo {
    uint8_t *buf_base;
    uint32_t bcolor;
    uint32_t pcolor;
    uint32_t width;
    uint32_t bpp;
  };
  static void DrawPoint(bool f, int x, int y) {
    uint32_t c = f ? _d_info.pcolor : _d_info.bcolor;
    _d_info.buf_base[(y * _d_info.width + x) * (_d_info.bpp / 8)] = c;
    _d_info.buf_base[(y * _d_info.width + x) * (_d_info.bpp / 8) + 1] = c >> 8;
    _d_info.buf_base[(y * _d_info.width + x) * (_d_info.bpp / 8) + 2] = c >> 16;
  }
  void Refresh() {
    if (_timeup_draw) {
      if (task_ctrl->GetState(_draw_cpuid) !=
          TaskCtrl::TaskQueueState::kNotStarted) {
        DisableTimeupDraw();
      } else {
        if (timer->IsTimePassed(
                timer->GetCntAfterPeriod(_last_time_refresh, 100 * 1000))) {
          DrawScreen();
          _last_time_refresh = timer->ReadMainCnt();
        }
      }
    } else {
      _needsRedraw = true;
    }
  }
  void DrawScreen();
  void DrawScreenSub();
  void DoPeriodicRefresh(void *);
  void DisableTimeupDraw();
  void ScheduleRefresh() {
    task_ctrl->RegisterCallout(_refresh_callout, _draw_cpuid, 10 * 1000);
  }
  SpinLock _lock;
  FrameBufferInfo _fb_info;
  size_t _bytes_per_pixel;  // calculated from _fb_info
  CachedFont _font_normal;
  CachedFont _font_inverted;
  bool *_font_buffer;  // for Error printing

  // current position of error printing
  uint32_t _cex = 0;
  uint32_t _cey = 0;

  char *_shell_buffer = nullptr;
  size_t _shell_buffer_len = 0;

  // frameinfo to path DrawPoint()
  static DrawInfo _d_info;

  // use this until TaskCtrl runs.
  bool _timeup_draw = true;
  // for checking if time is up
  uint64_t _last_time_refresh = 0;

  CpuId _draw_cpuid;
  bool _needsRedraw =
      false;  // if true, there are some changes should be display.

  sptr<Callout> _refresh_callout;
};
