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
#include <net/eth.h>
#include <net/ptcl.h>

void ProtocolStack::Setup() {
  // init socket table
  for(uint32_t i = 0; i < kMaxSocketNumber; i++) {
    socket_table[i].in_use = false;
  }

  RegisterPolling();
}

bool ProtocolStack::RegisterSocket(NetSocket *socket, uint16_t l3_ptcl) {
  if(_current_socket_number < kMaxSocketNumber) {
    for(uint32_t id = 0; id < kMaxSocketNumber; id++) {
      if(!socket_table[id].in_use) {
        // set id to socket
        socket->SetProtocolStackId(id);
        socket_table[id].in_use = true;
        socket_table[id].l3_ptcl = l3_ptcl;
      }
    }
    _current_socket_number++;
    return true;
  } else {
    // no enough buffer for the new socket
    return false;
  }
}

bool ProtocolStack::RemoveSocket(NetSocket *socket) {
  uint32_t id = socket->GetProtocolStackId();
  socket_table[id].in_use = false;
  _current_socket_number--;
  socket->SetProtocolStackId(-1);
  return true;
}

bool ProtocolStack::ReceivePacket(uint32_t socket_id, NetDev::Packet *&packet) {
  if(!socket_table[socket_id].in_use) {
    // invalid socket id
    return false;
  }

  return socket_table[socket_id].dup_queue.Pop(packet);
}

void ProtocolStack::FreeRxBuffer(NetDev::Packet *packet) {
  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(packet));
}

void ProtocolStack::Handle() {
  Locker locker(_lock);

  NetDev::Packet *packet = nullptr;

  if(_device->ReceivePacket(packet)) {
    NetDev::Packet *dup_packet = reinterpret_cast<NetDev::Packet*>(virtmem_ctrl->Alloc(sizeof(NetDev::Packet)));
    dup_packet->len = packet->len;
    memcpy(dup_packet->buf, packet->buf, packet->len);

    // insert into main queue at first
    _main_queue.Push(dup_packet);

    // free network device buffer as soon as possible
    _device->ReuseRxBuffer(packet);
  }

  if(!_main_queue.IsEmpty()) {
    // new packets arrived from network device
    NetDev::Packet *new_packet;
    kassert(_main_queue.Pop(new_packet));

    if(FilterPacket(new_packet)) {
      for(uint32_t i = 0; i < kMaxSocketNumber; i++) {
        if(socket_table[i].in_use && socket_table[i].l3_ptcl == GetL3PtclType(new_packet->buf)) {
          // distribute the received packet to duplicated queues
          NetDev::Packet *dup_packet = reinterpret_cast<NetDev::Packet*>(virtmem_ctrl->Alloc(sizeof(NetDev::Packet)));
          dup_packet->len = new_packet->len;
          memcpy(dup_packet->buf, new_packet->buf, new_packet->len);
          socket_table[i].dup_queue.Push(dup_packet);
        }
      }
    }

    virtmem_ctrl->Free(reinterpret_cast<virt_addr>(new_packet));
  }
}

void ProtocolStack::SetDevice(DevEthernet *dev) {
  _device = dev;
  _device->GetEthAddr(_eth_addr);
}

void ProtocolStack::InitPacketQueue() {
  
}

bool ProtocolStack::FilterPacket(NetDev::Packet *packet) {
  // filter Ethernet address
  if(!EthFilterPacket(packet->buf, nullptr, _eth_addr, 0)) {
    return false;
  }

  return true;
}
