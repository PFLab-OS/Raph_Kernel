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
#include "../mem/physmem.h"
#include "../global.h"
#include "../tty.h"

void E1000::Setup() {
  // the following sequence is indicated in pcie-gbe-controllers 14.3

  // disable interrupts (see 13.3.32)
  _mmioAddr[kRegImc] = 0xffffffff;

  // software reset
  this->Reset();

  // after global reset, interrupts must be disabled again (see 14.4)
  _mmioAddr[kRegImc] = 0xffffffff;

  // enable link (connection between L1(PHY) and L2(MAC), see 14.8)
  _mmioAddr[kRegCtrl] |= kRegCtrlSluFlag;

  // general config (82571x -> 14.5, 8254x -> 14.3)
  switch(kDeviceId) {
    case kI8257x:
      // disable link mode (see 12.5)
      _mmioAddr[kRegCtrlExt] &= (~kRegCtrlExtLinkModeMask);
      _mmioAddr[kRegCtrl] &= (~kRegCtrlIlosFlag);
      _mmioAddr[kRegTxdctl] |= (1 << 22);
      _mmioAddr[kRegTxdctl1] |= (1 << 22);
      break;
    case kI8254x:
      _mmioAddr[kRegCtrl] &= (~kRegCtrlPhyRstFlag | ~kRegCtrlVmeFlag);
      break;
  }

  // initialize receiver/transmitter ring buffer
  this->SetupRx();
  this->SetupTx();

  // enable interrupts
  _mmioAddr[kRegImc] = 0;
}

uint32_t E1000::ReceivePacket(uint8_t *buffer, uint32_t size) {
  E1000RxDesc *rxdesc;
  uint32_t rdh = _mmioAddr[kRegRdh0];
  uint32_t rdt = _mmioAddr[kRegRdt0];
  uint32_t length;
  int rx_available = (kRxdescNumber - rdt + rdh) % kRxdescNumber;

  if(rx_available > 0) {
    // if the packet is on the wire
    rxdesc = rx_desc_buf_ + (rdt % kRxdescNumber);
    length = length < rxdesc->length ? length : rxdesc->length;
    memcpy(buffer, reinterpret_cast<uint8_t *>(p2v(reinterpret_cast<phys_addr>(rxdesc->bufAddr))), length);
    _mmioAddr[kRegRdt0] = (rdt + 1) % kRxdescNumber;
    return length;
  } else {
    // if rx_desc_buf_ is full, fails
	// please retry again
    return -1;
  }
}

uint32_t E1000::TransmitPacket(const uint8_t *packet, uint32_t length) {
  E1000TxDesc *txdesc;
  uint32_t tdh = _mmioAddr[kRegTdh];
  uint32_t tdt = _mmioAddr[kRegTdt];
  int tx_available = kTxdescNumber - ((kTxdescNumber - tdh + tdt) % kTxdescNumber);
  
  if(tx_available > 0) {
    // if tx_desc_buf_ is not full
    txdesc = tx_desc_buf_ + (tdt % kTxdescNumber);
    memcpy(reinterpret_cast<uint8_t *>(p2v(reinterpret_cast<phys_addr>(txdesc->bufAddr))), packet, length);
    txdesc->length = length;
    txdesc->sta = 0;
    txdesc->css = 0;
    txdesc->cso = 0;
    txdesc->special = 0;
    txdesc->cmd = 0xd;

    _mmioAddr[kRegTdt] = (tdt + 1) % kTxdescNumber;

    return length;
  } else {
    // if tx_desc_buf_ is full, fails
	// please retry again
    return -1;
  }
}

void E1000::Reset() {
  // see 14.9
  _mmioAddr[kRegCtrl] |= kRegCtrlRstFlag;
}

void E1000::SetupRx() {
  // see 14.6
  // program the Receive address register(s) per the station address
  _mmioAddr[kRegRal0] = this->EepromRead(kEepromEthAddrLo);
  _mmioAddr[kRegRal0] |= this->EepromRead(kEepromEthAddrMd) << 16;
  _mmioAddr[kRegRah0] = this->EepromRead(kEepromEthAddrHi);
  _mmioAddr[kRegRah0] |= (kRegRahAselDestAddr | kRegRahAvFlag);

  // initialize the MTA (Multicast Table Array) to 0
  uint32_t *mta = _mmioAddr + kRegMta;
  for(int i = 0; i < 128; i++) mta[i] = 0;

  // set appropriate value to IMS (this value is suggested value)
  _mmioAddr[kRegIms] = (
    kRegImsLscFlag | kRegImsRxseqFlag | kRegImsRxdmt0Flag |
    kRegImsRxoFlag | kRegImsRxt0Flag);

  // set appropriate value to RXDCTL
  _mmioAddr[kRegRxdctl] = 0;

  // initialize the Receive Control Register
  _mmioAddr[kRegRctl] = (
    kRegRctlDtyp | ~kRegRctlVfeFlag |
    kRegRctlBsize | kRegRctlBsex);

  // allocate a region of memory for the rx desc list
  // MEMO: at the moment, E1000 class has static array for this purpose
  // (see the definition of E1000 class)

  // set base address of ring buffer
  _mmioAddr[kRegRdbal0] = v2p(reinterpret_cast<virt_addr>(rx_desc_buf_)) & 0xffffffff; // TODO: must be 16B-aligned
  _mmioAddr[kRegRdbah0] = v2p(reinterpret_cast<virt_addr>(rx_desc_buf_)) >> 32;

  // set the size of the desc ring
  _mmioAddr[kRegRdlen0] = kRxdescNumber * sizeof(E1000RxDesc);

  // set head and tail pointer of ring
  _mmioAddr[kRegRdh0] = 0;
  _mmioAddr[kRegRdt0] = kRxdescNumber; // tailptr should be one desc beyond the end (14.6.1)

  // initialize rx desc ring buffer
  for(int i = 0; i < kRxdescNumber; i++) {
    E1000RxDesc *rxdesc = &rx_desc_buf_[i];
    rxdesc->bufAddr = reinterpret_cast<uint8_t *>(v2p(reinterpret_cast<virt_addr>(rx_buf_)));
    rxdesc->vlanTag = 0;
    rxdesc->errors = 0;
    rxdesc->status = 0;
    rxdesc->checkSum = 0;
    rxdesc->length = 0;
  }

  // enable (this operation must be done after the initialization of rx desc ring)
  _mmioAddr[kRegRctl] |= kRegRctlEnFlag;
}

void E1000::SetupTx() {
  // see 14.7
  // set TXDCTL register (following value is suggested)
  _mmioAddr[kRegTxdctl] = (kRegTxdctlWthresh | kRegTxdctlGranDescriptor);

  // set TCTL register
  _mmioAddr[kRegTctl] = (
    kRegTctlEnFlag | kRegTctlPsp |
    kRegTctlCt | kRegTctlPsp);

  // allocate a region of memory for the transmit descriptor list
  // MEMO: at the moment, E1000 class has static array for this purpose
  // (see the definition of E1000 class)

  // set base address of ring buffer
  _mmioAddr[kRegTdbal] = v2p(reinterpret_cast<virt_addr>(tx_desc_buf_)) & 0xffffffff; // TODO: must be 16B-aligned
  _mmioAddr[kRegTdbah] = v2p(reinterpret_cast<virt_addr>(tx_desc_buf_)) >> 32;
  
  // set the size of the desc ring
  _mmioAddr[kRegTdlen] = kTxdescNumber * sizeof(E1000TxDesc);

  // set head and tail pointer of ring
  _mmioAddr[kRegTdh] = 0;
  _mmioAddr[kRegTdt] = 0;

  // set TIPG register (see 13.3.60)
  _mmioAddr[kRegTipg] = 0x00702008;

  // initialize the tx desc registers (TDBAL, TDBAH, TDL, TDH, TDT)
  for(int i = 0; i < kTxdescNumber; i++) {
    E1000TxDesc *txdesc = &tx_desc_buf_[i];
    txdesc->bufAddr = reinterpret_cast<uint8_t *>(v2p(reinterpret_cast<virt_addr>(tx_buf_)));
    txdesc->special = 0;
    txdesc->css = 0;
    txdesc->rsv = 0;
    txdesc->sta = 0;
    txdesc->cmd = 0;
    txdesc->cso = 0;
    txdesc->length = 0;
  }
}

uint16_t E1000::EepromRead(uint16_t addr) {
  // EEPROM is a kind of non-volatile memory storing config info
  // see pcie-gbe-controllers 5.2.1 (i8257x) or 5.3.1 (i8254x)

  // notify start bit and addr
  switch(kDeviceId) {
    case kI8254x:
      _mmioAddr[kRegEerd] = (((addr & 0xff) << 8) | 1);
      // polling
      while(!(_mmioAddr[kRegEerd] & (1 << 4))) {
        // busy-wait
      }
      break;
    case kI8257x:
      _mmioAddr[kRegEerd] = (((addr & 0x3fff) << 2) | 1);
      // polling
      while(!(_mmioAddr[kRegEerd] & (1 << 1))) {
        // busy-wait
      }
      break;
  }

  return (_mmioAddr[kRegEerd] >> 16) & 0xffff;
}

void E1000::PrintEthAddr() {
  uint16_t ethaddr_lo = this->EepromRead(kEepromEthAddrLo);
  uint16_t ethaddr_md = this->EepromRead(kEepromEthAddrMd);
  uint16_t ethaddr_hi = this->EepromRead(kEepromEthAddrHi);
  gtty->Printf("s", "MAC Address ... ");
  gtty->Printf("x", (ethaddr_hi & 0xff), "s", ":");
  gtty->Printf("x", (ethaddr_hi >> 8) & 0xff, "s", ":");
  gtty->Printf("x", (ethaddr_md & 0xff), "s", ":");
  gtty->Printf("x", (ethaddr_md >> 8) & 0xff, "s", ":");
  gtty->Printf("x", (ethaddr_lo & 0xff), "s", ":");
  gtty->Printf("x", (ethaddr_lo >> 8) & 0xff, "s", "\n");
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
    0x52, 0x54, 0x00, 0x12, 0x34, 0x56, // Source MAC Address
    0x08, 0x06, // Type: ARP
    // ARP Packet
    0x00, 0x01, // HardwareType: Ethernet
    0x08, 0x00, // ProtocolType: IPv4
    0x06, // HardwareLength
    0x04, // ProtocolLength
    0x00, 0x01, // Operation: ARP Request
    0x52, 0x54, 0x00, 0x12, 0x34, 0x56, // Source Hardware Address
    0x0a, 0x00, 0x02, 0x0f, // Source Protocol Address
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Target Hardware Address
    0x0a, 0x00, 0x02, 0x03, // Target Protocol Address
  };
  uint32_t len = sizeof(data)/sizeof(uint8_t);
  this->TransmitPacket(data, len);
  gtty->Printf("s", "Packet sent (length = ", "d", len, "s", ")\n");
}
