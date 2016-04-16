/*
 *
 * Copyright (c) 2016 Project Raphine
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

#include <string.h>
#include <mem/virtmem.h>
#include <mem/physmem.h>
#include <net/ptcl.h>

void ProtocolStack::Setup() {
}

bool ProtocolStack::RegisterSocket(NetSocket *socket) {
  if(_current_socket_number < kMaxSocketNumber) {
    // set id to socket
    socket->SetProtocolStackId(_current_socket_number++);
    return true;
  } else {
    // no enough buffer for the new socket
    return false;
  }
}

bool ProtocolStack::DelegatePacket(NetDev *dev, NetDev::Packet *packet) {
  Locker locker(_lock);

  NetDev::Packet *dup_packet = reinterpret_cast<NetDev::Packet*>(virtmem_ctrl->Alloc(sizeof(NetDev::Packet)));
  memcpy(dup_packet, packet, sizeof(NetDev::Packet));

  // insert into main queue at first
  bool result = _main_queue.Push(packet);
  dev->ReuseRxBuffer(packet);

  return result;
}

bool ProtocolStack::ReceivePacket(uint32_t socket_id, NetDev::Packet *packet) {
  if(socket_id >= _current_socket_number) {
    // invalid socket id
    return false;
  }

  return _duplicated_queue[socket_id].Pop(packet);
}

void ProtocolStack::Handle() {
  Locker locker(_lock);

  while(!_main_queue.IsEmpty()) {
    // new packets arrived from network device
    NetDev::Packet *new_packet;
    kassert(_main_queue.Pop(new_packet));

    if(_main_queue.Pop(new_packet)) {
      for(uint32_t i = 0; i < _current_socket_number; i++) {
        // distribute the received packet to duplicated queues
        NetDev::Packet *dup_packet = reinterpret_cast<NetDev::Packet*>(virtmem_ctrl->Alloc(sizeof(NetDev::Packet)));
        memcpy(dup_packet, new_packet, sizeof(NetDev::Packet));
        _duplicated_queue[i].Push(dup_packet);
      }

      virtmem_ctrl->Free(reinterpret_cast<virt_addr>(new_packet));
    }
  }
}
