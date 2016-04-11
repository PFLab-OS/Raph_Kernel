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
#include <global.h>

void DevGbeI8257::SetupHw(uint16_t did) {
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

  // register device to network device table
  kassert(netdev_ctrl->RegisterDevice(this));

  // enable interrupts
  // TODO: does this truely affect?
  _mmioAddr[kRegImc] = 0;
}

void DevGbeI8257::SetupRx() {
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

void DevGbeI8257::SetupTx() {
  // see 14.7
  // set TXDCTL register (following value is suggested)
  _mmioAddr[kRegTxdctl] = (kRegTxdctlWthresh | kRegTxdctlGranDescriptor);

  // set TCTL register
  _mmioAddr[kRegTctl] = (
    kRegTctlEnFlag | kRegTctlPsp |
    kRegTctlCold | kRegTctlCt);

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
    txdesc->sta = 0;
    txdesc->cmd = 0;
    txdesc->cso = 0;
    txdesc->length = 0;
  }
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
