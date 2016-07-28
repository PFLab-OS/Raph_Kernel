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
 * 16/01/15: created by Levelfour
 * 16/02/28: add Ich8 support by Liva
 * 
 */

#ifndef __RAPH_KERNEL_DEV_E1000_H__
#define __RAPH_KERNEL_DEV_E1000_H__

#include <stdint.h>
#include <mem/physmem.h>
#include <mem/virtmem.h>
#include <mem/paging.h>
#include <polling.h>
#include <global.h>
#include <dev/pci.h>
#include "../e1000/bem.h"

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
  // status & reserved
  uint8_t  sta;
  // checksum start
  uint8_t  css;
  // special field
  uint16_t special;
} __attribute__ ((packed));

class oE1000 : public bE1000, Polling {
public:
 oE1000(uint8_t bus, uint8_t device, bool mf) : bE1000(bus, device, mf) {}
  static bool InitPCI(uint16_t vid, uint16_t did, uint8_t bus, uint8_t device, bool mf);
  // from Polling
  void Handle() override;
  void Setup(uint16_t did) {
    this->SetupHw(did);
    this->InitTxPacketBuffer();
    this->InitRxPacketBuffer();
    // fetch ethernet address from EEPROM
    uint16_t ethaddr_lo = this->NvmRead(kEepromEthAddrLo);
    uint16_t ethaddr_md = this->NvmRead(kEepromEthAddrMd);
    uint16_t ethaddr_hi = this->NvmRead(kEepromEthAddrHi);
    _ethAddr[0] = ethaddr_hi & 0xff;
    _ethAddr[1] = (ethaddr_hi >> 8) & 0xff;
    _ethAddr[2] = ethaddr_md & 0xff;
    _ethAddr[3] = (ethaddr_md >> 8) & 0xff;
    _ethAddr[4] = ethaddr_lo & 0xff;
    _ethAddr[5] = (ethaddr_lo >> 8) & 0xff;
  }
  // buffer size
  static const int kBufSize = 2048;
  virtual void GetEthAddr(uint8_t *buffer) override;
  virtual void UpdateLinkStatus() override {}
 protected:
  // Memory Mapped I/O Base Address
  volatile uint32_t *_mmioAddr = nullptr;

  // see pcie-gbe-controllers 3.2
  int32_t Receive(uint8_t *buffer, uint32_t size);
  // see pcie-gbe-controllers 3.3, 3.4
  int32_t Transmit(const uint8_t *packet, uint32_t length);

  // init sequence of e1000 device (see pcie-gbe-controllers 14.3)
  virtual void SetupHw(uint16_t did) = 0;
  // software reset of e1000 device
  void Reset() {
    // see 14.9
    // see 11.2 (ich8-gbe-controllers)
    _mmioAddr[kRegCtrl] |= kRegCtrlRstFlag;
  }
  void Acquire() {
    _mmioAddr[kRegExtcnfCtrl] |= kRegExtcnfCtrlFlagSw;
    while(true) {
      volatile uint32_t v = _mmioAddr[kRegExtcnfCtrl];
      if ((v & kRegExtcnfCtrlFlagSw) != 0) {
        return;
      }
    }
  }
  void Release() {
    _mmioAddr[kRegExtcnfCtrl] &= kRegExtcnfCtrlFlagSw;
  }
  void WritePhy(uint16_t addr, uint16_t value);
  volatile uint16_t ReadPhy(uint16_t addr);
  virtual uint16_t NvmRead(uint16_t addr) = 0;

  // packet transmit/receive test
  uint32_t Crc32b(uint8_t *message);

  static const uint16_t kVendorId = 0x8086;

  // Device ID
  uint16_t _did = 0;

  // Device ID list
  static const uint16_t kI8254x = 0x100e;
  static const uint16_t kI8257x = 0x105e;

  // the number of receiver descriptors
  static const int kRxdescNumber = 16;
  // the buffer for receiver descriptors
  E1000RxDesc *rx_desc_buf_;
  // the number of transmit descriptors
  static const int kTxdescNumber = 16;
  // the buffer for transmit descriptors
  E1000TxDesc *tx_desc_buf_;
  // ethernet address
  uint8_t _ethAddr[6];

  // Ethernet Controller EEPROM Map (see pcie-gbe-controllers Table 5-2)
  static const int kEepromEthAddrHi = 0x00;
  static const int kEepromEthAddrMd = 0x01;
  static const int kEepromEthAddrLo = 0x02;
  static const int kEepromPciDeviceId = 0x0d;
  static const int kEepromPciVendorId = 0x0e;

  // Ethernet Controller Register Summary (see pcie-gbe-controllers Table 13-3)
  static const int kRegCtrl = 0x00000 / sizeof(uint32_t);
  static const int kRegStatus = 0x00008 / sizeof(uint32_t);
  static const int kRegEerd = 0x00014 / sizeof(uint32_t);
  static const int kRegCtrlExt = 0x00018 / sizeof(uint32_t);
  static const int kRegMdic = 0x00020 / sizeof(uint32_t);
  static const int kRegIms = 0x000d0 / sizeof(uint32_t);
  static const int kRegImc = 0x000d8 / sizeof(uint32_t);
  static const int kRegRctl = 0x00100 / sizeof(uint32_t);
  static const int kRegTctl = 0x00400 / sizeof(uint32_t);
  static const int kRegTipg = 0x00410 / sizeof(uint32_t);
  static const int kRegExtcnfCtrl = 0x00F00 / sizeof(uint32_t);
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
  static const int kRegFwsm = 0x05B54 / sizeof(uint32_t);

  // CTRL Register Bit Description (see pcie-gbe-controllers Table 13-4)
  static const uint32_t kRegCtrlSluFlag = 1 << 6;
  static const uint32_t kRegCtrlIlosFlag = 1 << 7; // see Table 5-4
  static const uint32_t kRegCtrlRstFlag = 1 << 26;
  static const uint32_t kRegCtrlRfceFlag = 1 << 27;
  static const uint32_t kRegCtrlTfceFlag = 1 << 28;
  static const uint32_t kRegCtrlVmeFlag = 1 << 30;
  static const uint32_t kRegCtrlPhyRstFlag = 1 << 31;

  // CTRL_EXT Register Bit Description (see pcie-gbe-controllers Table 13-9)
  static const uint32_t kRegCtrlExtLinkModeMask = 3 << 22;

  // MDI Control Register Bit Description (see pcie-gbe-controllers Table 13-12)
  static const uint32_t kRegMdicMaskData = 0xFFFF;
  static const uint32_t kRegMdicOffsetData = 0;
  static const uint32_t kRegMdicOffsetAddr = 16;  // this driver treats addr as set of phyaddr and regaddr
  static const uint32_t kRegMdicValueOpcWrite = 1 << 26;
  static const uint32_t kRegMdicValueOpcRead = 2 << 26;
  static const uint32_t kRegMdicFlagReady = 1 << 28;
  static const uint32_t kRegMdicFlagInterrupt = 1 << 29;
  static const uint32_t kRegMdicFlagErr = 1 << 30;

  // IMS Register Bit Description (see pcie-gbe-controllers Table 13-101)
  static const uint32_t kRegImsLscFlag = 1 << 2;
  static const uint32_t kRegImsRxseqFlag = 1 << 3;
  static const uint32_t kRegImsRxdmt0Flag = 1 << 4;
  static const uint32_t kRegImsRxoFlag = 1 << 6;
  static const uint32_t kRegImsRxt0Flag = 1 << 7;

  // Extended Configuration Control Register Bit Description (see ich8-geb-controllers Table 65)
  static const uint32_t kRegExtcnfCtrlFlagSw = 1 << 5;

  // RCTL Register Bit Description (see pcie-gbe-controllers Table 13-104)
  static const uint32_t kRegRctlEnFlag = 1 << 1;
  static const uint32_t kRegRctlUnicast = 1 << 3;
  static const uint32_t kRegRctlMulticast = 1 << 4;
  static const uint32_t kRegRctlRdmts = 0 << 8; // half of RDLEN
  static const uint32_t kRegRctlDtyp = 0 << 10; // legacy description type
  static const uint32_t kRegRctlBam = 1 << 15;
  static const uint32_t kRegRctlBsize = 0 << 16; // if BSEX=0 => 2048[Bytes]
  static const uint32_t kRegRctlVfeFlag = 1 << 18;
  static const uint32_t kRegRctlBsex = 0 << 25;
  static const uint32_t kRegRctlSecrc = 1 << 26;

  // TCTL Register Bit Description (see pcie-gbe-controllers Table 13-123)
  static const uint32_t kRegTctlEnFlag = 1 << 1;
  static const uint32_t kRegTctlPsp = 1 << 3;
  static const uint32_t kRegTctlCt = 0x0f << 4; // suggested

  // TXDCTL Register Bit Description (see pcie-gbe-controllers Table 13-132)
  static const uint32_t kRegTxdctlWthresh = 0x01 << 16;
  static const uint32_t kRegTxdctlGranCacheLine = 0 << 24;
  static const uint32_t kRegTxdctlGranDescriptor = 1 << 24;

  // RAH Register Bit Description (see pcie-gbe-controllers Table 13-141)
  static const uint32_t kRegRahAselDestAddr = 0 << 16;
  static const uint32_t kRegRahAselSourceAddr = 1 << 16;
  static const uint32_t kRegRahAvFlag = 1 << 31;

  // Firmware Semaphore Register Bit Description (see ich8-gbe-controllers 10.6.10)
  static const uint32_t kRegFwsmFlagRspciphy = 1 << 6;
};

class DevGbeI8254 : public oE1000 {
 public:
 DevGbeI8254(uint8_t bus, uint8_t device, bool mf) : oE1000(bus, device, mf) {}
 private:
  virtual uint16_t NvmRead(uint16_t addr) override {
    return this->EepromRead(addr);
  }
  // read data from EEPROM
  uint16_t EepromRead(uint16_t addr);
  virtual void SetupHw(uint16_t did) override;
  void SetupRx();
  void SetupTx();

  // TCTL Register Bit Description (see pci-gbe-controllers Table 13-123)
  static const uint32_t kRegTctlCold = 0x40 << 12; // suggested for full-duplex
};

class DevGbeI8257 : public oE1000 {
 public:
 DevGbeI8257(uint8_t bus, uint8_t device, bool mf) : oE1000(bus, device, mf) {}
 private:
  virtual uint16_t NvmRead(uint16_t addr) override {
    return this->EepromRead(addr);
  }
  // read data from EEPROM
  uint16_t EepromRead(uint16_t addr);
  virtual void SetupHw(uint16_t did) override;
  void SetupRx();
  void SetupTx();

  // TCTL Register Bit Description (see pcie-gbe-controllers Table 13-123)
  static const uint32_t kRegTctlCold = 0x3f << 12; // suggested for full-duplex
};

class DevGbeIch8 : public oE1000 {
 public:
 DevGbeIch8(uint8_t bus, uint8_t device, bool mf) : oE1000(bus, device, mf) {}
 private:
  virtual uint16_t NvmRead(uint16_t addr) override {
    return this->FlashRead(addr);
  }
  // read data from Flash
  uint16_t FlashRead(uint16_t addr);
  virtual void SetupHw(uint16_t did) override;
  void SetupRx();
  void SetupTx();

  // spi flash mmio
  volatile uint32_t *_flashAddr = nullptr;
  volatile uint16_t *_flashAddr16 = nullptr;

  // TCTL Register Bit Description (see ich8-gbe-controllers Table 10.4.52.0.1)
  static const uint32_t kRegTctlCold = 0x3f << 12; // suggested for full-duplex

  // Ethernet Controller Register Summary (see ich8-gbe-controllers Table 55)
  static const int kRegPba = 0x01000 / sizeof(uint32_t);
  static const int kRegPbs = 0x01008 / sizeof(uint32_t);

  static const uint32_t kRegPbaValue8K = 0x0008;
  static const uint32_t kRegPbsValue16K = 0x0010;

  // GbE SPI Flash Program Registers (see ich8-gbe-controllers Table 21-1)
  static const int kRegGlfpr = 0x00 / sizeof(uint32_t);
  static const int kReg16Hsfs = 0x04 / sizeof(uint16_t);
  static const int kReg16Hsfc = 0x06 / sizeof(uint16_t);
  static const int kRegFaddr = 0x08 / sizeof(uint32_t);
  static const int kRegFdata0 = 0x10 / sizeof(uint32_t);

  uint32_t GetPrb () {
    return (_flashAddr[kRegGlfpr] & 0x1FFF) << 12;
  }

  static const uint16_t kReg16HsfsFlagFdone = 1 << 0;
  static const uint16_t kReg16HsfsFlagFcerr = 1 << 1;
  static const uint16_t kReg16HsfsFlagAel = 1 << 2;
  static const uint16_t kReg16HsfsFlagScip = 1 << 5;
  static const uint16_t kReg16HsfsFlagFdv = 1 << 14;
 
  static const uint16_t kReg16HsfcFlagFdbc16 = 1 << 8; // 2 byte
  static const uint16_t kReg16HsfcFlagFcycleRead = 0 << 1;
  static const uint16_t kReg16HsfcFlagFgo = 1 << 0;

  // PHY Register Summary (see I217 datasheet 8.4)
  // !important! this driver treats addr as set of phyaddr and regaddr
  static const uint16_t kPhyRegCtrl =  (2 << 5) | 0;
  static const uint16_t kPhyRegAutoNegAdvertisement =  (2 << 5) | 4;
  static const uint16_t kPhyRegAutoNegLinkPartenerAbility =  (2 << 5) | 5;

  // PHY Control Register Bit Description (see I217 datasheet 8.5 Table 1)
  static const uint16_t kPhyRegCtrlFlagReset = 1 << 15;

  // PHY Auto Negotiation Advertisement Register Bit Description (see I217 datasheet 8.5 Table 5)
  static const uint16_t kPhyRegAutoNegAdvertisementFlagPauseCapable = 1 << 10;
  static const uint16_t kPhyRegAutoNegAdvertisementFlagAsymmetricPause = 1 << 11;

  // PHY Auto Negotiation Link Partner Ability Register Bit Description (see I217 datasheet 8.5 Table 6)
  static const uint16_t kPhyRegAutoNegLinkPartnerAbilityFlagPauseCapable = 1 << 10;
  static const uint16_t kPhyRegAutoNegLinkPartnerAbilityFlagAsymmetricPause = 1 << 11;
};


#endif /* __RAPH_KERNEL_DEV_E1000_H__ */
