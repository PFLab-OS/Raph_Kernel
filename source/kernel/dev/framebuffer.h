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
  void Refresh() {
    if (_timeup_draw) {
      if (task_ctrl->GetState(_draw_cpuid) != TaskCtrl::TaskQueueState::kNotStarted) {
        DisableTimeupDraw();
      } else {
        if (_last_time_refresh == 0 ||
            timer->IsTimePassed(timer->GetCntAfterPeriod(_last_time_refresh, 500 * 1000))) {
          memcpy(_fb_info.buffer, _hidden_buffer, _fb_info.width * _fb_info.height * (_fb_info.bpp / 8));
          _last_time_refresh = timer->ReadMainCnt();
        }
      }
    } else {
      _redraw = true;
    }
  }
  void Scroll();
  void DoPeriodicRefresh(void *);
  void DisableTimeupDraw();
  void ScheduleRefresh() {
    task_ctrl->RegisterCallout(_refresh_callout, _draw_cpuid, 300);
  }
  SpinLock _lock;
  FrameBufferInfo _fb_info;
  Font _font;
  int _column;
  int _info_row; // row of information display area
  int _cx, _cy; // current position of the cursor

  // frameinfo to path DrawPoint()
  static DrawInfo _d_info;

  // use this until TaskCtrl runs.
  bool _timeup_draw = true;
  // for checking if time is up
  uint64_t _last_time_refresh = 0;
  
  CpuId _draw_cpuid;
  // use this buffer for drawing
  // TaskCtrl periodically copies this to framebuffer
  uint8_t *_hidden_buffer = nullptr;
  bool _redraw = false;

  sptr<Callout> _refresh_callout;
};
