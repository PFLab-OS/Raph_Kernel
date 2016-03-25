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
 * Author: Liva
 *
 * 16/03/02: split from e1000.cc
 * 
 */

#include <stdint.h>
#include "e1000.h"
#include <mem/paging.h>
#include <timer.h>
#include <global.h>

void DevGbeIch8::SetupHw(uint16_t did) {
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

  this->Acquire();

  // disable interrupts
  _mmioAddr[kRegImc] = 0xffffffff;

  // software reset & general config (see ich8-gbe-controllers 11.4.2.1)
  _mmioAddr[kRegPba] = kRegPbaValue8K;
  _mmioAddr[kRegPbs] = kRegPbsValue16K;
  while(true) {
    volatile uint32_t v = _mmioAddr[kRegFwsm];
      if ((v & kRegFwsmFlagRspciphy) != 0) {
      break;
    }
  }
  _mmioAddr[kRegCtrl] |= kRegCtrlRstFlag | kRegCtrlPhyRstFlag;

  // after global reset, interrupts must be disabled again
  _mmioAddr[kRegImc] = 0xffffffff;

  timer->BusyUwait(15 * 1000);

  // PHY Initialization (see ich8-gbe-controllers 11.4.3.1)
  this->WritePhy(kPhyRegCtrl, this->ReadPhy(kPhyRegCtrl) | kPhyRegCtrlFlagReset);
  timer->BusyUwait(15 * 1000);

  // TODO: MAC/PHY Link Setup (see ich8-gbe-controllers 11.4.3.2
  // also see ich8-gbe-controllers 9.5.2 & Table 50

  // initialize receiver/transmitter ring buffer
  this->SetupRx();
  this->SetupTx();

  // register device to network device table
  kassert(netdev_ctrl->RegisterDevice(this));

  // enable interrupts
  // TODO: does this truely affect?
  _mmioAddr[kRegImc] = 0;

  this->Release();
}

void DevGbeIch8::SetupRx() {
  // see 14.6
  // program the Receive address register(s) per the station address
  uint16_t ethAddrLo = this->NvmRead(kEepromEthAddrLo);
  uint16_t ethAddrMd = this->NvmRead(kEepromEthAddrMd);
  uint16_t ethAddrHi = this->NvmRead(kEepromEthAddrHi);
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
    kRegImsLscFlag | kRegImsRxdmt0Flag |
    kRegImsRxoFlag | kRegImsRxt0Flag);

  // set appropriate value to RXDCTL
  _mmioAddr[kRegRxdctl] = 0;

  // initialize the Receive Control Register
  _mmioAddr[kRegRctl] = (kRegRctlDtyp |
                         kRegRctlUnicast | kRegRctlMulticast | kRegRctlBam |
                         kRegRctlBsize | kRegRctlBsex | kRegRctlSecrc);

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

void DevGbeIch8::SetupTx() {
  // see 14.7
  // set TXDCTL register (following value is suggested)
  _mmioAddr[kRegTxdctl] = (kRegTxdctlWthresh | kRegTxdctlGranDescriptor);
    /*    _mmioAddr[kRegTxdctl1] = (kRegTxdctlWthresh | kRegTxdctlGranDescriptor);
  _mmioAddr[kRegTxdctl] = 0x1f | (1 << 8) | (1 << 16) | (1 << 22) | (1 << 25) | (kRegTxdctlGranDescriptor);*/

  // set TCTL register
  _mmioAddr[kRegTctl] = (kRegTctlEnFlag | kRegTctlPsp |
                         kRegTctlCold | kRegTctlCt |
                         0x10000000);

  // allocate a region of memory for the transmit descriptor list
  // MEMO: at the moment, E1000 class has static array for this purpose
  // (see the definition of E1000 class)

  // set base address of ring buffer
  virt_addr tx_desc_buf_addr = ((virtmem_ctrl->Alloc(sizeof(E1000TxDesc) * kTxdescNumber + 15) + 15) / 16) * 16;
  tx_desc_buf_ = reinterpret_cast<E1000TxDesc *>(tx_desc_buf_addr);

  kassert((_mmioAddr[kRegTdbal] & 0xF) == 0);
  _mmioAddr[kRegTdbal] = k2p(tx_desc_buf_addr) & 0xffffffff;
  _mmioAddr[kRegTdbah] = k2p(tx_desc_buf_addr) >> 32;
  
  kassert(((kTxdescNumber * sizeof(E1000TxDesc)) % 128) == 0);
  // set the size of the desc ring
  _mmioAddr[kRegTdlen] = kTxdescNumber * sizeof(E1000TxDesc);

  // set head and tail pointer of ring
  _mmioAddr[kRegTdh] = 0;
  _mmioAddr[kRegTdt] = 0;

  // set TIPG register (see 13.3.60)
  _mmioAddr[kRegTipg] = 0x00702008;

  _mmioAddr[kRegTxdctl] |= (1 << 22);

  // for i217 packet loss issue
  _mmioAddr[0x24 / sizeof(uint32_t)] &= ~7;
  _mmioAddr[0x24 / sizeof(uint32_t)] |= 7;
    

  // initialize the tx desc registers (TDBAL, TDBAH, TDL, TDH, TDT)
  for(int i = 0; i < kTxdescNumber; i++) {
    volatile E1000TxDesc *txdesc = &tx_desc_buf_[i];
    txdesc->bufAddr = k2p(virtmem_ctrl->Alloc(kBufSize));
    txdesc->special = 0;
    txdesc->css = 0;
    txdesc->sta = 0;
    txdesc->cmd = 0;
    txdesc->cso = 0;
    txdesc->length = 0;
  }
}

uint16_t DevGbeIch8::FlashRead(uint16_t addr) {
  // see ich8-chipset datasheet 20.3 & official E1000 driver source code
  while(true) {
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
    for (int i = 0; i < 1000; i++) {
      // busy-wait
      volatile uint16_t data = _flashAddr16[kReg16Hsfs];
      if ((data & kReg16HsfsFlagFdone) != 0) {
        return _flashAddr[kRegFdata0] & 0xFFFF;
      }
      kassert((data & kReg16HsfsFlagFcerr) == 0);
      timer->BusyUwait(1);
    }
  }
}
