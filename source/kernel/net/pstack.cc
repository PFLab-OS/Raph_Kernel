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


#include <net/pstack.h>


bool ProtocolStack::SetBaseLayer(ProtocolStackBaseLayer *layer) {
  for (int i = 0; i < kMaxConnectionNumber; i++) {
    if (!_base_layers[i]) {
      _base_layers[i] = layer;
      layer->SetProtocolStack(this);

      return true;
    }
  }

  return false;
}

/**
 * Callback which fetches newly arrived packets from the network device,
 * duplicates and inserts into the main queue. Set to NetDev RX queue.
 */
void ProtocolStack::DeviceBufferHandler(void *_) {
  NetDev::Packet *packet = nullptr;

  while (_device->ReceivePacket(packet)) {
    // duplicates the packet and inserts into main queue
    NetDev::Packet *dup_packet = nullptr;
    kassert(_reserved_queue.Pop(dup_packet));

    dup_packet->len = packet->len;
    memcpy(dup_packet->buf, packet->buf, packet->len);

    _buffered_queue.Push(dup_packet);

    _device->ReuseRxBuffer(packet);
  }
}

/**
 * Callback which fetches newly arrived packets from the main queue,
 * duplicates and inserts into base layer queue. Set to main queue.
 */
void ProtocolStack::EscalationHandler(void *_) {
  NetDev::Packet *new_packet = nullptr;
  kassert(_buffered_queue.Pop(new_packet));

  for (int i = 0; i < kMaxConnectionNumber; i++) {
    ProtocolStackBaseLayer *blayer = _base_layers[i];

    if (blayer && !blayer->IsFull()) {
      // duplicates the packet and inserts into each base layer
      NetDev::Packet *dup_packet = nullptr;
      kassert(blayer->FetchRxBuffer(dup_packet));

      dup_packet->len = new_packet->len;
      memcpy(dup_packet->buf, new_packet->buf, new_packet->len);

      blayer->Delegate(dup_packet);
    }
  }

  _reserved_queue.Push(new_packet);
}
