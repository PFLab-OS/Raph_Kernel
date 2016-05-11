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

void DeviceBufferHandler(void *self) {
  ProtocolStack *ptcl_stack = reinterpret_cast<ProtocolStack*>(self);

  NetDev::Packet *packet = nullptr;

  while(ptcl_stack->_device->ReceivePacket(packet)) {
    NetDev::Packet *dup_packet;
    kassert(ptcl_stack->GetMainQueuePacket(dup_packet));

    dup_packet->len = packet->len;
    memcpy(dup_packet->buf, packet->buf, packet->len);

    // insert into main queue at first
    ptcl_stack->_main_queue.Push(dup_packet);

    // free network device buffer as soon as possible
    ptcl_stack->_device->ReuseRxBuffer(packet);
  }
}

void MainQueueHandler(void *self) {
  ProtocolStack *ptcl_stack = reinterpret_cast<ProtocolStack*>(self);

  NetDev::Packet *new_packet;
  kassert(ptcl_stack->_main_queue.Pop(new_packet));

  for(uint32_t i = 0; i < ptcl_stack->kMaxSocketNumber; i++) {
    if(ptcl_stack->_socket_table[i].in_use && ptcl_stack->_socket_table[i].l3_ptcl == GetL3PtclType(new_packet->buf)) {
      // distribute the received packet to duplicated queues
      if(!ptcl_stack->_socket_table[i].dup_queue.IsFull()) {
        NetDev::Packet *dup_packet;
        kassert(ptcl_stack->_socket_table[i].reserved_queue.Pop(dup_packet));

        dup_packet->len = new_packet->len;
        memcpy(dup_packet->buf, new_packet->buf, new_packet->len);
        ptcl_stack->_socket_table[i].dup_queue.Push(dup_packet);
      }
    }
  }

  ptcl_stack->ReuseMainQueuePacket(new_packet);
}

void ProtocolStack::Setup() {
  InitPacketQueue();

  // init socket table
  for(uint32_t i = 0; i < kMaxSocketNumber; i++) {
    _socket_table[i].in_use = false;
  }

  // set callback functions
  Function main_queue_callback;
  main_queue_callback.Init(MainQueueHandler, this);
  _main_queue.SetFunction(2, main_queue_callback);
}

bool ProtocolStack::RegisterSocket(NetSocket *socket, uint16_t l3_ptcl) {
  if(_current_socket_number < kMaxSocketNumber) {
    int32_t socket_id = -1;

    for(uint32_t id = 0; id < kMaxSocketNumber; id++) {
      if(!_socket_table[id].in_use) {
        // set id to socket
        socket->SetProtocolStackId(id);
        _socket_table[id].in_use = true;
        _socket_table[id].l3_ptcl = l3_ptcl;
        socket_id = id;
        break;
      }
    }

    // initialize reserved buffer for duplicated queue
    kassert(socket_id != -1);
    while(!_socket_table[socket_id].reserved_queue.IsFull()) {
      NetDev::Packet *packet = reinterpret_cast<NetDev::Packet *>(virtmem_ctrl->Alloc(sizeof(NetDev::Packet)));
      kassert(_socket_table[socket_id].reserved_queue.Push(packet));
    }

    _current_socket_number++;
    return true;
  } else {
    // no enough buffer for the new socket
    return false;
  }

  return false;
}

bool ProtocolStack::RemoveSocket(NetSocket *socket) {
  uint32_t id = socket->GetProtocolStackId();
  _socket_table[id].in_use = false;
  _current_socket_number--;
  socket->SetProtocolStackId(-1);
  return true;
}

bool ProtocolStack::ReceivePacket(uint32_t socket_id, NetDev::Packet *&packet) {
  if(!_socket_table[socket_id].in_use) {
    // invalid socket id
    return false;
  }

  return _socket_table[socket_id].dup_queue.Pop(packet);
}

void ProtocolStack::FreeRxBuffer(uint32_t socket_id, NetDev::Packet *packet) {
  kassert(_socket_table[socket_id].reserved_queue.Push(packet));
}

void ProtocolStack::SetDevice(NetDev *dev) {
  _device = dev;

  // set callback function
  Function network_device_callback;
  network_device_callback.Init(DeviceBufferHandler, this);
  _device->SetReceiveCallback(2, network_device_callback);
}

void ProtocolStack::InitPacketQueue() {
  while(!_reserved_main_queue.IsFull()) {
    NetDev::Packet *packet = reinterpret_cast<NetDev::Packet *>(virtmem_ctrl->Alloc(sizeof(NetDev::Packet)));
    kassert(_reserved_main_queue.Push(packet));
  }
}

bool ProtocolStack::GetMainQueuePacket(NetDev::Packet *&packet) {
  return _reserved_main_queue.Pop(packet);
}

void ProtocolStack::ReuseMainQueuePacket(NetDev::Packet *packet) {
  kassert(_reserved_main_queue.Push(packet));
}
