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
 * Author: levelfour
 * 
 */

#ifndef __RAPH_KERNEL_NET_SOCKET_H__
#define __RAPH_KERNEL_NET_SOCKET_H__


#include <net/pstack.h>


/**
 * Socket class which is the top layer of protocol stack, and serves as
 * the interface for users.
 * Socket constructs one stack connection in initial sequence.
 */
class Socket : public ProtocolStackLayer {
public:
  Socket() {}

  /**
   * Reserve the connection on protocol stack and construct the stack.
   */
  virtual int Open() = 0;

  virtual bool SetupSubclass() {
    // prevent adding sublayer to Socket class
    _next_layer = this;

    return true;
  }
};


#endif // __RAPH_KERNEL_NET_SOCKET_H__
