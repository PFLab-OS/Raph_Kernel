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
#include "../timer.h"
#include "../mem/paging.h"
#include "../tty.h"
#include "../global.h"

void E1000::InitPCI(uint16_t vid, uint16_t did, uint8_t bus, uint8_t device, bool mf) {
  if (vid == kVendorId) {
    E1000 *e1000 = nullptr;
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
    }
    if (e1000 != nullptr) {
      e1000->Setup(did);
      polling_ctrl->Register(e1000);
      e1000->TxTest();
    }
  }
}

void E1000::WritePhy(uint16_t addr, uint16_t value) {
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

volatile uint16_t E1000::ReadPhy(uint16_t addr) {
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

int32_t E1000::ReceivePacket(uint8_t *buffer, uint32_t size) {
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
    return length;
  } else {
    // if rx_desc_buf_ is empty, fails
	// please retry again
    return -1;
  }
}

int32_t E1000::TransmitPacket(const uint8_t *packet, uint32_t length) {
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

void E1000::GetEthAddr(uint8_t *buffer) {
  uint16_t ethaddr_lo = this->NvmRead(kEepromEthAddrLo);
  uint16_t ethaddr_md = this->NvmRead(kEepromEthAddrMd);
  uint16_t ethaddr_hi = this->NvmRead(kEepromEthAddrHi);
  buffer[0] = ethaddr_hi & 0xff;
  buffer[1] = (ethaddr_hi >> 8) & 0xff;
  buffer[2] = ethaddr_md & 0xff;
  buffer[3] = (ethaddr_md >> 8) & 0xff;
  buffer[4] = ethaddr_lo & 0xff;
  buffer[5] = (ethaddr_lo >> 8) & 0xff;
}

uint32_t E1000::Crc32b(uint8_t *message) {
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

void E1000::TxTest() {
  uint8_t data[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Target MAC Address
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source MAC Address
    0x08, 0x06, // Type: ARP
    // ARP Packet
    0x00, 0x01, // HardwareType: Ethernet
    0x08, 0x00, // ProtocolType: IPv4
    0x06, // HardwareLength
    0x04, // ProtocolLength
    0x00, 0x01, // Operation: ARP Request
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source Hardware Address
    0xC0, 0xA8, 0x64, 0x74, // Source Protocol Address
    //0x0A, 0x00, 0x02, 0x05,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Target Hardware Address
    0xC0, 0xA8, 0x64, 0x64, // Target Protocol Address
    //0x0A, 0x00, 0x02, 0x0F,
  };
  GetEthAddr(data + 6);
  memcpy(data + 22, data + 6, 6);

  uint32_t len = sizeof(data)/sizeof(uint8_t);
  this->TransmitPacket(data, len);
  gtty->Printf("s", "[debug] info: Packet sent (length = ", "d", len, "s", ")\n");
}

void E1000::Handle() {
  const uint32_t kBufsize = 256;
  virt_addr vaddr = virtmem_ctrl->Alloc(sizeof(uint8_t) * kBufsize);
  uint8_t *buf = reinterpret_cast<uint8_t*>(k2p(vaddr));
  if(this->ReceivePacket(buf, kBufsize) == -1) {
    virtmem_ctrl->Free(vaddr);
    return;
  } 
  // received packet
  if(buf[12] == 0x08 && buf[13] == 0x06) {
    // ARP packet
    gtty->Printf(
                 "s", "[debug] info: ARP Reply received; ",
                 "x", buf[6], "s", ":",
                 "x", buf[7], "s", ":",
                 "x", buf[8], "s", ":",
                 "x", buf[9], "s", ":",
                 "x", buf[10], "s", ":",
                 "x", buf[11], "s", " -> ",
                 "d", buf[28], "s", ".",
                 "d", buf[29], "s", ".",
                 "d", buf[30], "s", ".",
                 "d", buf[31], "s", "\n");
  }
  virtmem_ctrl->Free(vaddr);
}
