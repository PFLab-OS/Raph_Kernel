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
  void Setup();
  virtual int GetRow() override {
    return 0;
  }
  virtual int GetColumn() override {
    return 0;
  }
private:
  class StringBuffer {
  public:
    StringBuffer() = delete;
    StringBuffer(const FrameBufferInfo *info, const Font *font) : _info(info), _font(font) {
    }
    ~StringBuffer() {
      if (_next != nullptr) {
        delete _next;
      }
    }
    void Write(uint32_t index) {
      if (_length == 1024) {
        if (_next == nullptr) {
          _next = new StringBuffer(_info, _font);
        }
        _next->Write(index);
      } else {
        _cur_width += _font->GetWidth(index);
        _length++;
        if (index == kNewLineSignature) {
          _buffer[_length - 1] = index;
          _cur_width = 0;
          _row++;
        } else if (_cur_width > _info->width) {
          _buffer[_length - 1] = kNewLineSignature;
          _cur_width = 0;
          _row++;
          Write(index);
        } else {
          _buffer[_length - 1] = index;
        }
      }
    }
    uint32_t _buffer[1024];
    int _length = 0;
    int _row = 0;
    int _cur_width = 0;
    StringBuffer *_next = nullptr;
    const FrameBufferInfo *_info;
    const Font *_font;
    static const uint32_t kNewLineSignature = 0xFFFFFFFF;
  } *_info_buffer;
  
  virtual void Write(char c) override;
  virtual void WriteErr(char c) override;
  virtual void PrintShell(const char *str) override;
  virtual void Flush() override {
    Locker locker(_lock);
    if (_timeup_draw) {
      DrawScreen();
      _last_time_refresh = timer->ReadMainCnt();
    } else {
      DrawScreen();
      _redraw = false;
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
  uint32_t GetColor() {
    return 0x00FFFFFF;
  }
  void Refresh() {
    if (_timeup_draw) {
      if (task_ctrl->GetState(_draw_cpuid) != TaskCtrl::TaskQueueState::kNotStarted) {
        DisableTimeupDraw();
      } else {
        if (timer->IsTimePassed(timer->GetCntAfterPeriod(_last_time_refresh, 50 * 1000))) {
          DrawScreen();
          _last_time_refresh = timer->ReadMainCnt();
        }
      }
    } else {
      _redraw = true;
    }
  }
  void DrawScreen();
  void DrawScreenSub();
  void DoPeriodicRefresh(void *);
  void DisableTimeupDraw();
  void ScheduleRefresh() {
    task_ctrl->RegisterCallout(_refresh_callout, _draw_cpuid, 50);
  }
  SpinLock _lock;
  FrameBufferInfo _fb_info;
  Font _font;
  bool *_font_buffer;  // for Error printing
  int _info_row; // row of information display area

  // current position of error printing
  int _cex = 0;
  int _cey = 0;

  char *_shell_buffer = nullptr;
  size_t _shell_buffer_len = 0;

  // frameinfo to path DrawPoint()
  static DrawInfo _d_info;

  // use this until TaskCtrl runs.
  bool _timeup_draw = true;
  // for checking if time is up
  uint64_t _last_time_refresh = 0;
  
  CpuId _draw_cpuid;
  bool _redraw = false;

  sptr<Callout> _refresh_callout;
};
