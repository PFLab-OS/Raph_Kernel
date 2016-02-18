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

#ifndef __RAPH_KERNEL_DEV_E1000_H__
#define __RAPH_KERNEL_DEV_E1000_H__

#include <stdint.h>
#include "../mem/physmem.h"
#include "../mem/virtmem.h"
#include "eth.h"

/*
 * e1000 is the driver for Intel 8254x/8256x/8257x and so on.
 * This implementation mainly takes care of 8257x.
 * These chipsets is loaded on Intel PRO/1000 series NIC,
 * and provide the way to receive/transmit packets.
 *
 * Main reference is PCIe* GbE Controllers Open Source Software Developer’s Manual.
 *   URL: http://www.intel.com/content/www/us/en/embedded/products/networking/pcie-gbe-controllers-open-source-manual.html
 * "pcie-gbe-controllers" in the source codes refers to this reference.
 * (if there's no notations, this means the reference of i8257x, not i8254x)
 */

/*
 * Receiver Descriptor (legacy type)
 *   data structure that contains the receiver data buffer address
 *   and fields for hardware to store packet information
 *   see pcie-gbe-controllers 3.2.4
 */
struct E1000RxDesc {
  // buffer address
  phys_addr bufAddr;
  // buffer length
  uint16_t length;
  // check sum
  uint16_t checkSum;
  // see Table 3-2
  uint8_t  status;
  // see Table 3-3
  uint8_t  errors;
  // additional information for VLAN
  uint16_t vlanTag;
} __attribute__ ((packed));

/*
 * Transmit Descriptor (legacy type)
 *   data structure that contains the transmit data buffer address
 *   see pcie-gbe-controllers 3.4.2
 */
struct E1000TxDesc {
  // buffer address
  phys_addr bufAddr;
  // segment length
  uint16_t length;
  // checksum offset
  uint8_t  cso;
  // command
  uint8_t  cmd;
  // status
  uint8_t  sta;
  // reserved (set to be 0x0)
  uint8_t  rsv;
  // checksum start
  uint8_t  css;
  // special field
  uint16_t special;
} __attribute__ ((packed));

class E1000 : public DevEthernet {
public:
 E1000(uint8_t bus, uint8_t device, bool mf) : DevEthernet(bus, device, mf) {}
  static void InitPCI(uint16_t vid, uint16_t did, uint8_t bus, uint8_t device, bool mf) {
    if (vid == kVendorId) {
      switch(did) {
      case kI8254x:
      case kI8257x:
        E1000 *addr = reinterpret_cast<E1000 *>(virtmem_ctrl->Alloc(sizeof(E1000)));
        E1000 *e1000 = new(addr) E1000(bus, device, mf);
        e1000->Setup();
        e1000->PrintEthAddr();
        e1000->TxTest();
        e1000->RxTest();
        break;
      }
    }
  }
  // init sequence of e1000 device (see pcie-gbe-controllers 14.3)
  void Setup();
  // see pcie-gbe-controllers 3.2
  int32_t ReceivePacket(uint8_t *buffer, uint32_t size);
  // see pcie-gbe-controllers 3.3, 3.4
  int32_t TransmitPacket(const uint8_t *packet, uint32_t length);
  // buffer size
  static const int kBufSize = 2048;
  // for debugging
  void PrintEthAddr();
private:
  // Memory Mapped I/O Base Address
  uint32_t *_mmioAddr = nullptr;
  // software reset of e1000 device
  void Reset();
  // initialize receiver
  void SetupRx();
  // initialize transmitter
  void SetupTx();
  // read data from EEPROM
  uint16_t EepromRead(uint16_t addr);

  // packet transmit/receive test
  uint32_t Crc32b(uint8_t *message);
  void TxTest();
  void RxTest();

  static const uint16_t kVendorId = 0x8086;

  // Device ID (TODO: this must be fetched from PCIe device list)
  static const uint16_t kDeviceId = 0x100e; // for QEMU

  // Device ID list
  static const uint16_t kI8254x = 0x100e;
  static const uint16_t kI8257x = 0x105e;

  // the number of receiver descriptors
  static const int kRxdescNumber = 8;
  // the buffer for receiver descriptors
  E1000RxDesc *rx_desc_buf_;
  // the number of transmit descriptors
  static const int kTxdescNumber = 8;
  // the buffer for transmit descriptors
  E1000TxDesc *tx_desc_buf_;

  // Ethernet Controller EEPROM Map (see pcie-gbe-controllers Table 5-2)
  static const int kEepromEthAddrHi = 0x00;
  static const int kEepromEthAddrMd = 0x01;
  static const int kEepromEthAddrLo = 0x02;
  static const int kEepromPciDeviceId = 0x0d;
  static const int kEepromPciVendorId = 0x0e;

  // Ethernet Controller Register Summary (see pcie-gbe-controllers Table 13-3)
  static const int kRegCtrl = 0x00000 / sizeof(uint32_t);
  static const int kRegEerd = 0x00014 / sizeof(uint32_t);
  static const int kRegCtrlExt = 0x00018 / sizeof(uint32_t);
  static const int kRegIms = 0x000d0 / sizeof(uint32_t);
  static const int kRegImc = 0x000d8 / sizeof(uint32_t);
  static const int kRegRctl = 0x00100 / sizeof(uint32_t);
  static const int kRegTctl = 0x00400 / sizeof(uint32_t);
  static const int kRegTipg = 0x00410 / sizeof(uint32_t);
  static const int kRegRdbal0 = 0x02800 / sizeof(uint32_t);
  static const int kRegRdbah0 = 0x02804 / sizeof(uint32_t);
  static const int kRegRdlen0 = 0x02808 / sizeof(uint32_t);
  static const int kRegRdh0 = 0x02810 / sizeof(uint32_t);
  static const int kRegRdt0 = 0x02818 / sizeof(uint32_t);
  static const int kRegRxdctl = 0x02828 / sizeof(uint32_t);
  static const int kRegTdbal = 0x03800 / sizeof(uint32_t);
  static const int kRegTdbah = 0x03804 / sizeof(uint32_t);
  static const int kRegTdlen = 0x03808 / sizeof(uint32_t);
  static const int kRegTdh = 0x03810 / sizeof(uint32_t);
  static const int kRegTdt = 0x03818 / sizeof(uint32_t);
  static const int kRegTxdctl = 0x03828 / sizeof(uint32_t);
  static const int kRegTxdctl1 = 0x03928 / sizeof(uint32_t);
  static const int kRegMta = 0x05200 / sizeof(uint32_t);
  static const int kRegRal0 = 0x05400 / sizeof(uint32_t);
  static const int kRegRah0 = 0x05404 / sizeof(uint32_t);

  // CTRL Register Bit Description (see pcie-gbe-controllers Table 13-4)
  static const int kRegCtrlSluFlag = 1 << 6;
  static const int kRegCtrlIlosFlag = 1 << 7; // see Table 5-4
  static const int kRegCtrlRstFlag = 1 << 26;
  static const int kRegCtrlVmeFlag = 1 << 30;
  static const int kRegCtrlPhyRstFlag = 1 << 31;

  // CTRL_EXT Register Bit Description (see pcie-gbe-controllers Table 13-9)
  static const int kRegCtrlExtLinkModeMask = 3 << 22;

  // IMS Register Bit Description (see pcie-gbe-controllers Table 13-101)
  static const int kRegImsLscFlag = 1 << 2;
  static const int kRegImsRxseqFlag = 1 << 3;
  static const int kRegImsRxdmt0Flag = 1 << 4;
  static const int kRegImsRxoFlag = 1 << 6;
  static const int kRegImsRxt0Flag = 1 << 7;

  // RCTL Register Bit Description (see pcie-gbe-controllers Table 13-104)
  static const int kRegRctlEnFlag = 1 << 2;
  static const int kRegRctlRdmts = 0 << 8; // half of RDLEN
  static const int kRegRctlDtyp = 0 << 10; // legacy description type
  static const int kRegRctlVfeFlag = 1 << 18;
  static const int kRegRctlBsize = 0 << 16; // if BSEX=0 => 2048[Bytes]
  static const int kRegRctlBsex = 0 << 25;

  // TCTL Register Bit Description (see pcie-gbe-controllers Table 13-123)
  static const int kRegTctlEnFlag = 1 << 1;
  static const int kRegTctlPsp = 1 << 3;
  static const int kRegTctlCt = 0x0f << 4; // suggested
  static const int kRegTctlCold = 0x3f << 12; // suggested for full-duplex

  // TXDCTL Register Bit Description (see pcie-gbe-controllers Table 13-132)
  static const int kRegTxdctlWthresh = 0x01 << 16;
  static const int kRegTxdctlGranCacheLine = 0 << 24;
  static const int kRegTxdctlGranDescriptor = 1 << 24;

  // RAH Register Bit Description (see pcie-gbe-controllers Table 13-141)
  static const int kRegRahAselDestAddr = 0 << 16;
  static const int kRegRahAselSourceAddr = 1 << 16;
  static const int kRegRahAvFlag = 1 << 31;
};

#endif /* __RAPH_KERNEL_DEV_E1000_H__ */
