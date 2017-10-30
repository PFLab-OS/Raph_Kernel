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
 */

#pragma once

#include <stdint.h>
#include <raph.h>
#include <ptr.h>
#include <function.h>
#include <string.h>

class UdpCtrl {
public:
  static void Init();
  static UdpCtrl &GetCtrl() {
    kassert(_udp_ctrl != nullptr);
    return *_udp_ctrl;
  }
  void SetupServer();
  void Send(uint8_t (*target_addr)[4], uint16_t target_port, const char *data, size_t len);
  void SendStr(uint8_t (*target_addr)[4], uint16_t target_port, const char *data) {
    Send(target_addr, target_port, data, strlen(data));
  }
  class Socket {
  public:
    uint16_t port;
    GenericFunction<> func;
  };
private:
  static UdpCtrl *_udp_ctrl;
  uptr<Socket> _socket[10];
};
