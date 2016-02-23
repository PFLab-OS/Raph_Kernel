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
 * Author: Liva
 * 
 */

#ifndef __RAPH_KERNEL_DEV_L2_H__
#define __RAPH_KERNEL_DEV_L2_H__

#include <stdint.h>
#include "../spinlock.h"

class DevNetL2 {
public:
  virtual int32_t ReceivePacket(uint8_t *buffer, uint32_t size) = 0;
  virtual int32_t TransmitPacket(const uint8_t *packet, uint32_t length) = 0;
protected:
  DevNetL2() {}
  SpinLock _lock;
};

#endif /* __RAPH_KERNEL_DEV_L2_H__ */
