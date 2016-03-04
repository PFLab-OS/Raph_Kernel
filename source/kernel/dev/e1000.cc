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
#include "../net/eth.h"
#include "../mem/physmem.h"
#include "../mem/paging.h"
#include "../global.h"
#include "../tty.h"

#include "../timer.h"

#define __NETCTRL__
#include "../net/global.h"

void E1000::Setup(uint16_t did) {
  _did = did;

  // the following sequence is indicated in pcie-gbe-controllers 14.3

  // get PCI Base Address Registers
  phys_addr bar = this->ReadReg<uint32_t>(PCICtrl::kBaseAddressReg0);
  kassert((bar & 0xF) == 0);
  phys_addr mmio_addr = bar & 0xFFFFFFF0; // TODO : もうちょっと厳密にやるべき
  this->WriteReg<uint16_t>(PCICtrl::kCommandReg, this->ReadReg<uint16_t>(PCICtrl::kCommandReg) | PCICtrl::kCommandRegBusMasterEnableFlag | (1 << 10));
  _mmioAddr = reinterpret_cast<uint32_t*>(p2v(mmio_addr));
  // disable interrupts (see 13.3.32)
  _mmioAddr[kRegImc] = 0xffffffff;

  // software reset
  this->Reset();

  // after global reset, interrupts must be disabled again (see 14.4)
  _mmioAddr[kRegImc] = 0xffffffff;

  // enable link (connection between L1(PHY) and L2(MAC), see 14.8)
  _mmioAddr[kRegCtrl] |= kRegCtrlSluFlag;

  // general config (82571x -> 14.5, 8254x -> 14.3)
  switch(_did) {
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

  // register device to network device table
  kassert(netdev_ctrl->RegisterDevice(this));

  // enable interrupts
  _mmioAddr[kRegImc] = 0;
}

int32_t E1000::ReceivePacket(uint8_t *buffer, uint32_t size) {
  int32_t rval;
  WRITE_LOCK(_lock) {
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
      rval = length;
    } else {
      // if rx_desc_buf_ is full, fails
      // please retry again
      rval = -1;
    }
  }
  return rval;
}

int32_t E1000::TransmitPacket(const uint8_t *packet, uint32_t length) {
  int32_t rval;
  WRITE_LOCK(_lock) {
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


      gtty->Printf(
                   "x", packet[0], "s", " ", "x", packet[1], "s", " ",
                   "x", packet[2], "s", " ", "x", packet[3], "s", " ",
                   "x", packet[4], "s", " ", "x", packet[5], "s", " ",
                   "x", packet[6], "s", " ", "x", packet[7], "s", "\n");
      gtty->Printf(
                   "x", packet[8], "s", " ", "x", packet[9], "s", " ",
                   "x", packet[10], "s", " ", "x", packet[11], "s", " ",
                   "x", packet[12], "s", " ", "x", packet[13], "s", " ",
                   "x", packet[14], "s", " ", "x", packet[15], "s", "\n");
      gtty->Printf(
                   "x", packet[16], "s", " ", "x", packet[17], "s", " ",
                   "x", packet[18], "s", " ", "x", packet[19], "s", " ",
                   "x", packet[20], "s", " ", "x", packet[21], "s", " ",
                   "x", packet[22], "s", " ", "x", packet[23], "s", "\n");
      gtty->Printf(
                   "x", packet[24], "s", " ", "x", packet[25], "s", " ",
                   "x", packet[26], "s", " ", "x", packet[27], "s", " ",
                   "x", packet[28], "s", " ", "x", packet[29], "s", " ",
                   "x", packet[30], "s", " ", "x", packet[31], "s", "\n");
      gtty->Printf(
                   "x", packet[32], "s", " ", "x", packet[33], "s", " ",
                   "x", packet[34], "s", " ", "x", packet[35], "s", " ",
                   "x", packet[36], "s", " ", "x", packet[37], "s", " ",
                   "x", packet[38], "s", " ", "x", packet[39], "s", "\n");
      gtty->Printf(
                   "x", packet[40], "s", " ", "x", packet[41], "s", " ",
                   "x", packet[42], "s", " ", "x", packet[43], "s", " ",
                   "x", packet[44], "s", " ", "x", packet[45], "s", " ",
                   "x", packet[46], "s", " ", "x", packet[47], "s", "\n");
      gtty->Printf(
                   "x", packet[48], "s", " ", "x", packet[49], "s", " ",
                   "x", packet[50], "s", " ", "x", packet[51], "s", " ",
                   "x", packet[52], "s", " ", "x", packet[53], "s", " ",
                   "x", packet[53], "s", " ", "x", packet[55], "s", "\n");
      gtty->Printf(
                   "x", packet[56], "s", " ", "x", packet[57], "s", " ",
                   "x", packet[58], "s", " ", "x", packet[59], "s", " ",
                   "x", packet[60], "s", " ", "x", packet[61], "s", " ",
                   "x", packet[62], "s", " ", "x", packet[63], "s", "\n");

      rval = length;
    } else {
      // if tx_desc_buf_ is full, fails
      // please retry again
      rval = -1;
    }
  }
  return rval;
}

void E1000::Reset() {
  // see 14.9
  _mmioAddr[kRegCtrl] |= kRegCtrlRstFlag;
}

void E1000::SetupRx() {
  // see 14.6
  // program the Receive address register(s) per the station address
  uint16_t ethAddrLo = this->EepromRead(kEepromEthAddrLo); 
  uint16_t ethAddrMd = this->EepromRead(kEepromEthAddrMd); 
  uint16_t ethAddrHi = this->EepromRead(kEepromEthAddrHi); 
  _ethAddr[0] = ethAddrHi & 0xff;
  _ethAddr[1] = ethAddrHi >> 8;
  _ethAddr[2] = ethAddrMd & 0xff;
  _ethAddr[3] = ethAddrMd >> 8;
  _ethAddr[4] = ethAddrLo & 0xff;
  _ethAddr[5] = ethAddrLo >> 8;
  _mmioAddr[kRegRal0] = ethAddrLo;
  _mmioAddr[kRegRal0] |= ethAddrMd << 16;
  _mmioAddr[kRegRah0] = ethAddrHi;
  _mmioAddr[kRegRah0] |= (kRegRahAselDestAddr | kRegRahAvFlag);

  // initialize the MTA (Multicast Table Array) to 0
  volatile uint32_t *mta = _mmioAddr + kRegMta;
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
  PhysAddr paddr;
  physmem_ctrl->Alloc(paddr, PagingCtrl::ConvertNumToPageSize(sizeof(E1000RxDesc) * kRxdescNumber));
  phys_addr rx_desc_buf_paddr = paddr.GetAddr();
  virt_addr rx_desc_buf_vaddr = p2v(rx_desc_buf_paddr);
  rx_desc_buf_ = reinterpret_cast<E1000RxDesc *>(rx_desc_buf_vaddr);
  _mmioAddr[kRegRdbal0] = rx_desc_buf_paddr & 0xffffffff; // TODO: must be 16B-aligned
  _mmioAddr[kRegRdbah0] = rx_desc_buf_paddr >> 32;

  // set the size of the desc ring
  _mmioAddr[kRegRdlen0] = kRxdescNumber * sizeof(E1000RxDesc);

  // set head and tail pointer of ring
  _mmioAddr[kRegRdh0] = 0;
  _mmioAddr[kRegRdt0] = kRxdescNumber; // tailptr should be one desc beyond the end (14.6.1)

  // initialize rx desc ring buffer
  for(int i = 0; i < kRxdescNumber; i++) {
    E1000RxDesc *rxdesc = &rx_desc_buf_[i];
    virt_addr tmp = virtmem_ctrl->Alloc(kBufSize);
    rxdesc->bufAddr = k2p(tmp);
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
  virt_addr tx_desc_buf_addr = ((virtmem_ctrl->Alloc(sizeof(E1000TxDesc) * kTxdescNumber + 15) + 15) / 16) * 16;
  tx_desc_buf_ = reinterpret_cast<E1000TxDesc *>(tx_desc_buf_addr);

  _mmioAddr[kRegTdbal] = k2p(tx_desc_buf_addr) & 0xffffffff;
  _mmioAddr[kRegTdbah] = k2p(tx_desc_buf_addr) >> 32;
  
  // set the size of the desc ring
  _mmioAddr[kRegTdlen] = kTxdescNumber * sizeof(E1000TxDesc);

  // set head and tail pointer of ring
  _mmioAddr[kRegTdh] = 0;
  _mmioAddr[kRegTdt] = 0;

  // set TIPG register (see 13.3.60)
  _mmioAddr[kRegTipg] = 0x00702008;

  // initialize the tx desc registers (TDBAL, TDBAH, TDL, TDH, TDT)
  for(int i = 0; i < kTxdescNumber; i++) {
    volatile E1000TxDesc *txdesc = &tx_desc_buf_[i];
    txdesc->bufAddr = k2p(virtmem_ctrl->Alloc(kBufSize));
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
  switch(_did) {
    case kI8254x:
      _mmioAddr[kRegEerd] = (((addr & 0xff) << 8) | 1);
      // polling
      while(true) {
        // busy-wait
        volatile uint32_t data = _mmioAddr[kRegEerd];
        if (data & (1 << 4)) {
          break;
        }
      }
      break;
    case kI8257x:
      _mmioAddr[kRegEerd] = (((addr & 0x3fff) << 2) | 1);
      // polling
      while(true) {
        // busy-wait
        volatile uint32_t data = _mmioAddr[kRegEerd];
        if (data & (1 << 1)) {
          break;
        }
      }
      break;
  }

  return (_mmioAddr[kRegEerd] >> 16) & 0xffff;
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
    0x0a, 0x00, 0x02, 0x0f, // Source Protocol Address
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Target Hardware Address
    0x0a, 0x00, 0x02, 0x03, // Target Protocol Address
  };
  GetEthAddr(data + 6);
  memcpy(data + 22, data + 6, 6);

  uint32_t len = sizeof(data)/sizeof(uint8_t);
  this->TransmitPacket(data, len);
  gtty->Printf("s", "Packet sent (length = ", "d", len, "s", ")\n");
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
                 "s", "ARP Reply received; ",
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
