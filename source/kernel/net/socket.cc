/*
 *
 * Copyright (c) 2016 Raphine Project
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
 * Author: Levelfour
 * 
 */

#include "socket.h"
#include "eth.h"
#include "ip.h"
#include "udp.h"

#define __NETCTRL__
#include "global.h"

int32_t Socket::Open() {
  _dev = netdev_ctrl->GetDevice();
  if(!_dev) {
    return -1;
  } else {
    return 0;
  }
}

int32_t Socket::ReceivePacket(uint8_t *data, uint32_t length) {
  return -1;
}

int32_t Socket::TransmitPacket(const uint8_t *data, uint32_t length) {
  return -1;
}
