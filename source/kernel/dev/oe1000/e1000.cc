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

#include <stdint.h>
#include "e1000.h"
#include <timer.h>
#include <mem/paging.h>
#include <tty.h>
#include <global.h>
extern uint32_t cnt;

extern bE1000 *eth;

bool oE1000::InitPCI(uint16_t vid, uint16_t did, uint8_t bus, uint8_t device, bool mf) {
  if (vid == kVendorId) {
    oE1000 *e1000 = nullptr;
    switch(did) {
    case kI8254x:
      {
        DevGbeI8254 *addr = reinterpret_cast<DevGbeI8254 *>(virtmem_ctrl->Alloc(sizeof(DevGbeI8254)));
        e1000 = new(addr) DevGbeI8254(bus, device, mf);
      }
      break;
    case kI8257x:
      {
        DevGbeI8257 *addr = reinterpret_cast<DevGbeI8257 *>(virtmem_ctrl->Alloc(sizeof(DevGbeI8257)));
        e1000 = new(addr) DevGbeI8257(bus, device, mf);
      }
      break;
    case 0x153A:
      {
        DevGbeIch8 *addr = reinterpret_cast<DevGbeIch8 *>(virtmem_ctrl->Alloc(sizeof(DevGbeIch8)));
        e1000 = new(addr) DevGbeIch8(bus, device, mf);
      }
      break;
    default:
      return false;
    }
    kassert(e1000 != nullptr);
    e1000->Setup(did);
    e1000->RegisterPolling();
    timer->BusyUwait(3000*1000);
    eth = e1000;
    return true;
  }
  return false;
}

void oE1000::WritePhy(uint16_t addr, uint16_t value) {
  // TODO: check register and set page if need
  _mmioAddr[kRegMdic] = kRegMdicValueOpcWrite | (addr << kRegMdicOffsetAddr) | (value << kRegMdicOffsetData);
  while(true) {
    timer->BusyUwait(50);
    volatile uint32_t result = _mmioAddr[kRegMdic];
    if ((result & kRegMdicFlagReady) != 0) {
      return;
    }
    kassert((result & kRegMdicFlagErr) == 0);
  }
}

volatile uint16_t oE1000::ReadPhy(uint16_t addr) {
  // TODO: check register and set page if need
  _mmioAddr[kRegMdic] = kRegMdicValueOpcRead | (addr << kRegMdicOffsetAddr);
  while(true) {
    timer->BusyUwait(50);
    volatile uint32_t result = _mmioAddr[kRegMdic];
    if ((result & kRegMdicFlagReady) != 0) {
      return static_cast<volatile uint16_t>(result & kRegMdicMaskData);
    }
    kassert((result & kRegMdicFlagErr) == 0);
  }
}

int32_t oE1000::Receive(uint8_t *buffer, uint32_t size) {
  E1000RxDesc *rxdesc;
  uint32_t rdh = _mmioAddr[kRegRdh0];
  uint32_t rdt = _mmioAddr[kRegRdt0];
  uint32_t length;
  int rx_available = (kRxdescNumber - rdt + rdh) % kRxdescNumber;

  if(rx_available > 0) {
    // if the packet is on the wire
    rxdesc = rx_desc_buf_ + (rdt % kRxdescNumber);
    length = size < rxdesc->length ? size : rxdesc->length;
    memcpy(buffer, reinterpret_cast<uint8_t *>(p2v(rxdesc->bufAddr)), length);
    _mmioAddr[kRegRdt0] = (rdt + 1) % kRxdescNumber;

    gtty->Printf("s", "e1000 rx dump; (",
                 "d", rdh, "s", " ~ ", "d", rdt,
                 "s", ") 0x",
                 "x", reinterpret_cast<uint64_t>(rxdesc->bufAddr),
                 "s", "\n");
    for(uint32_t i = 0; i < length; i++) {
	  if(buffer[i] < 0x10) gtty->Printf("d", 0);
	  gtty->Printf("x", buffer[i]);
	  if((i+1) % 16 == 0) gtty->Printf("s", "\n");
	  else if((i+1) % 16 == 8) gtty->Printf("s", ":");
	  else                gtty->Printf("s", " ");
	}
	gtty->Printf("s", "\n");

    return length;
  } else {
    // if rx_desc_buf_ is empty, fails
	// please retry again
    return -1;
  }
}

int32_t oE1000::Transmit(const uint8_t *packet, uint32_t length) {
  E1000TxDesc *txdesc;
  uint32_t tdh = _mmioAddr[kRegTdh];
  uint32_t tdt = _mmioAddr[kRegTdt];
  int tx_available = kTxdescNumber - ((kTxdescNumber - tdh + tdt) % kTxdescNumber);

  if(tx_available > 0) {
    // if tx_desc_buf_ is not full
    txdesc = tx_desc_buf_ + (tdt % kTxdescNumber);
    memcpy(reinterpret_cast<uint8_t *>(p2v(txdesc->bufAddr)), packet, length);
    txdesc->length = length;
    txdesc->sta = 0;
    txdesc->css = 0;
    txdesc->cmd = 0xb;
    txdesc->special = 0;
    txdesc->cso = 0;
    _mmioAddr[kRegTdt] = (tdt + 1) % kTxdescNumber;
    return length;
  } else {
    // if tx_desc_buf_ is full, fails
	// please retry again
    return -1;
  }
}

void oE1000::GetEthAddr(uint8_t *buffer) {
  memcpy(buffer, _ethAddr, 6);
}

uint32_t oE1000::Crc32b(uint8_t *message) {
  int32_t i, j;
  uint32_t byte, crc, mask;

  i = 0;
  crc = 0xFFFFFFFF;
  while (message[i] != 0) {
    byte = message[i];  // Get next byte.
    crc = crc ^ byte;
    for (j = 7; j >= 0; j--) {  // Do eight times.
      mask = -(crc & 1);
      crc = (crc >> 1) ^ (0xEDB88320 & mask);
    }
    i = i + 1;
  }
  return ~crc;
}

void oE1000::Handle() {
  uint8_t buf[kBufSize];
  int32_t len;
  if ((len = this->Receive(buf, kBufSize)) != -1) {
    bE1000::Packet *packet;
    if (this->_rx_reserved.Pop(packet)) {
      memcpy(packet->buf, buf, len);
      packet->len = len;
      if (!this->_rx_buffered.Push(packet)) {
        kassert(this->_rx_reserved.Push(packet));
      }
    }
  }

  if (!this->_tx_buffered.IsEmpty()) {
    bE1000::Packet *packet;
    kassert(this->_tx_buffered.Pop(packet));
    this->Transmit(packet->buf, packet->len);
    this->ReuseTxBuffer(packet);
  }
}
