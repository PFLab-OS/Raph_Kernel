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
#include "../mem/paging.h"
#include "../global.h"
#include "../tty.h"

#include "../timer.h"
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


void DevGbeI8254::Setup(uint16_t did) {
  _did = did;

  // the following sequence is indicated in pcie-gbe-controllers 14.3

  // get PCI Base Address Registers
  phys_addr bar = this->ReadReg<uint32_t>(PCICtrl::kBaseAddressReg0);
  kassert((bar & 0xF) == 0);
  phys_addr mmio_addr = bar & 0xFFFFFFF0;
  _mmioAddr = reinterpret_cast<uint32_t*>(p2v(mmio_addr));

  // Enable BusMaster
  this->WriteReg<uint16_t>(PCICtrl::kCommandReg, this->ReadReg<uint16_t>(PCICtrl::kCommandReg) | PCICtrl::kCommandRegBusMasterEnableFlag | (1 << 10));

  // disable interrupts (see 13.3.32)
  _mmioAddr[kRegImc] = 0xffffffff;

  // software reset
  this->Reset();

  // after global reset, interrupts must be disabled again (see 14.4)
  _mmioAddr[kRegImc] = 0xffffffff;

  // enable link (connection between L1(PHY) and L2(MAC), see 14.8)
  _mmioAddr[kRegCtrl] |= kRegCtrlSluFlag;

  // general config (82571x -> 14.5, 8254x -> 14.3)
  _mmioAddr[kRegCtrl] &= (~kRegCtrlPhyRstFlag | ~kRegCtrlVmeFlag | ~kRegCtrlIlosFlag);

  // initialize receiver/transmitter ring buffer
  this->SetupRx();
  this->SetupTx();

  // enable interrupts
  //  _mmioAddr[kRegImc] = 0;
}

void DevGbeI8257::Setup(uint16_t did) {
  _did = did;

  // the following sequence is indicated in pcie-gbe-controllers 14.3

  // get PCI Base Address Registers
  phys_addr bar = this->ReadReg<uint32_t>(PCICtrl::kBaseAddressReg0);
  kassert((bar & 0xF) == 0);
  phys_addr mmio_addr = bar & 0xFFFFFFF0;
  _mmioAddr = reinterpret_cast<uint32_t*>(p2v(mmio_addr));

  // Enable BusMaster
  this->WriteReg<uint16_t>(PCICtrl::kCommandReg, this->ReadReg<uint16_t>(PCICtrl::kCommandReg) | PCICtrl::kCommandRegBusMasterEnableFlag | (1 << 10));

  // disable interrupts (see 13.3.32)
  _mmioAddr[kRegImc] = 0xffffffff;

  // software reset
  this->Reset();

  // after global reset, interrupts must be disabled again (see 14.4)
  _mmioAddr[kRegImc] = 0xffffffff;

  // enable link (connection between L1(PHY) and L2(MAC), see 14.8)
  _mmioAddr[kRegCtrl] |= kRegCtrlSluFlag;

  // general config (82571x -> 14.5, 8254x -> 14.3)
  _mmioAddr[kRegCtrlExt] &= (~kRegCtrlExtLinkModeMask);
  _mmioAddr[kRegCtrl] &= (~kRegCtrlIlosFlag);
  _mmioAddr[kRegTxdctl] |= (1 << 22);
  _mmioAddr[kRegTxdctl1] |= (1 << 22);

  // initialize receiver/transmitter ring buffer
  this->SetupRx();
  this->SetupTx();

  // enable interrupts
  _mmioAddr[kRegImc] = 0;
}

void DevGbeIch8::Setup(uint16_t did) {
  _did = did;

  // the following sequence is indicated in ich8-gbe-controllers 11.4

  // get PCI Base Address Registers
  phys_addr bar = this->ReadReg<uint32_t>(PCICtrl::kBaseAddressReg0);
  kassert((bar & 0xF) == 0);
  phys_addr mmio_addr = bar & 0xFFFFFFF0;
  _mmioAddr = reinterpret_cast<uint32_t*>(p2v(mmio_addr));

  bar = this->ReadReg<uint32_t>(PCICtrl::kBaseAddressReg1);
  kassert((bar & 0xF) == 0);
  mmio_addr = bar & 0xFFFFFFF0;
  _flashAddr = reinterpret_cast<uint32_t*>(p2v(mmio_addr));
  _flashAddr16 = reinterpret_cast<uint16_t*>(p2v(mmio_addr));

  // Enable BusMaster
  this->WriteReg<uint16_t>(PCICtrl::kCommandReg, this->ReadReg<uint16_t>(PCICtrl::kCommandReg) | PCICtrl::kCommandRegBusMasterEnableFlag | (1 << 10));

  // disable interrupts
  _mmioAddr[kRegImc] = 0xffffffff;

  // software reset
  this->Reset();

  // after global reset, interrupts must be disabled again
  _mmioAddr[kRegImc] = 0xffffffff;

  // general config (see ich8-gbe-controllers 11.4.2.1)
  _mmioAddr[kRegPba] = kRegPbaValue8K;
  _mmioAddr[kRegPbs] = kRegPbsValue16K;
  _mmioAddr[kRegCtrl] |= kRegCtrlRstFlag | kRegCtrlPhyRstFlag;
  
  timer->BusyUwait(15 * 1000);

  // PHY Initialization (see ich8-gbe-controllers 11.4.3.1)
  this->WritePhy(kPhyRegCtrl, this->ReadPhy(kPhyRegCtrl) | kPhyRegCtrlFlagReset);
  timer->BusyUwait(15 * 1000);

  // MAC/PHY Link Setup (see ich8-gbe-controllers 11.4.3.2
  // also see ich8-gbe-controllers 9.5.2 & Table 50
  // TODO: need to be more strict
  if ((this->ReadPhy(kPhyRegAutoNegAdvertisement) & kPhyRegAutoNegAdvertisementFlagPauseCapable) &
      (this->ReadPhy(kPhyRegAutoNegLinkPartenerAbility) & kPhyRegAutoNegLinkPartnerAbilityFlagPauseCapable)) {
    // enable flow control
    _mmioAddr[kRegCtrl] |= kRegCtrlRfceFlag | kRegCtrlTfceFlag;
  } else {
    // disable flow control
    _mmioAddr[kRegCtrl] &= ~(kRegCtrlRfceFlag | kRegCtrlTfceFlag);
  }  

  // initialize receiver/transmitter ring buffer
  this->SetupRx();
  this->SetupTx();

  // enable interrupts
  _mmioAddr[kRegImc] = 0;
}

void E1000::WritePhy(uint16_t addr, uint16_t value) {
  // TODO check register and set page if need
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

uint16_t E1000::ReadPhy(uint16_t addr) {
  // TODO check register and set page if need
  _mmioAddr[kRegMdic] = kRegMdicValueOpcRead | (addr << kRegMdicOffsetAddr);
  while(true) {
    timer->BusyUwait(50);
    volatile uint32_t result = _mmioAddr[kRegMdic];
    if ((result & kRegMdicFlagReady) != 0) {
      return static_cast<uint16_t>(result & kRegMdicMaskData);
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
    // if rx_desc_buf_ is full, fails
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

 void E1000::SetupRx() {
  // see 14.6
  // program the Receive address register(s) per the station address
  _mmioAddr[kRegRal0] = this->NvmRead(kEepromEthAddrLo);
  _mmioAddr[kRegRal0] |= this->NvmRead(kEepromEthAddrMd) << 16;
  _mmioAddr[kRegRah0] = this->NvmRead(kEepromEthAddrHi);
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

uint16_t DevGbeI8254::EepromRead(uint16_t addr) {
  // EEPROM is a kind of non-volatile memory storing config info
  // see pcie-gbe-controllers 5.3.1 (i8254x)

  _mmioAddr[kRegEerd] = (((addr & 0xff) << 8) | 1);
  // polling
  while(true) {
    // busy-wait
    volatile uint32_t data = _mmioAddr[kRegEerd];
    if ((data & (1 << 4)) != 0) {
      break;
    }
  }
  
  return (_mmioAddr[kRegEerd] >> 16) & 0xffff;
}

uint16_t DevGbeI8257::EepromRead(uint16_t addr) {
  // EEPROM is a kind of non-volatile memory storing config info
  // see pcie-gbe-controllers 5.2.1 (i8257x)

  _mmioAddr[kRegEerd] = (((addr & 0x3fff) << 2) | 1);
  // polling
  while(true) {
    // busy-wait
    volatile uint32_t data = _mmioAddr[kRegEerd];
    if ((data & (1 << 1)) != 0) {
      break;
    }
  }

  return (_mmioAddr[kRegEerd] >> 16) & 0xffff;
}

uint16_t DevGbeIch8::FlashRead(uint16_t addr) {
  // see ich8-chipset datasheet 20.3 & official E1000 driver source code

  // init flash cycle

  // clear status
  kassert((_flashAddr16[kReg16Hsfs] & kReg16HsfsFlagFdv) != 0);
  _flashAddr16[kReg16Hsfs] |= kReg16HsfsFlagAel | kReg16HsfsFlagFcerr;

  // check no running cycle
  kassert((_flashAddr16[kReg16Hsfs] & kReg16HsfsFlagScip) == 0);
  _flashAddr16[kReg16Hsfs] |= kReg16HsfsFlagFdone;
  
  _flashAddr16[kReg16Hsfc] |= kReg16HsfcFlagFdbc16 | kReg16HsfcFlagFcycleRead;
  
  _flashAddr[kRegFaddr] = GetPrb() + addr * 2;

  // start cycle
  _flashAddr16[kReg16Hsfc] |= kReg16HsfcFlagFgo;
  // polling
  while(true) {
    // busy-wait
    volatile uint16_t data = _flashAddr16[kReg16Hsfs];
    if ((data & kReg16HsfsFlagFdone) != 0) {
      break;
    }
    kassert((data & kReg16HsfsFlagFcerr) == 0);
    timer->BusyUwait(1);
  }

  return _flashAddr[kRegFdata0] & 0xFFFF;
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
    0x0a, 0x00, 0x02, 0x07, // Source Protocol Address
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Target Hardware Address
    0x0a, 0x00, 0x02, 0x0f, // Target Protocol Address
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
