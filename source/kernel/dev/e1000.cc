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

//  _mmioAddr = reinterpret_cast<uint32_t *>(*kPCIeBAR);

  // disable interrupts (see 13.3.32)
  _mmioAddr[kRegImc] = 0xffffffff;

  // software reset
  this->Reset();

  // after global reset, interrupts must be disabled again (see 14.4)
  _mmioAddr[kRegImc] = 0xffffffff;

  // enable link (connection between L1(PHY) and L2(MAC), see 14.8)
  _mmioAddr[kRegCtrl] |= kRegCtrlSluFlag;

  // general config (mainly focus on Intel 82571EB, see 14.5)
  // TODO: handle other chips appropriately
  // disable link mode (see 12.5)
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

void E1000::ReceivePacket() {
}

void E1000::TransmitPacket() {
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
  _mmioAddr[kRegRdbal0] = v2p(reinterpret_cast<virt_addr>(rx_desc_buf_)) & 0xffffffff; // must be 16B-aligned
  _mmioAddr[kRegRdbah0] = v2p(reinterpret_cast<virt_addr>(rx_desc_buf_)) >> 32;

  // set the size of the desc ring
  _mmioAddr[kRegRdlen0] = kRxdescNumber * sizeof(E1000RxDesc);

  // set head and tail pointer of ring
  _mmioAddr[kRegRdh0] = 0;
  _mmioAddr[kRegRdt0] = kRxdescNumber; // tailptr should be one desc beyond the end (14.6.1)

  // initialize rx desc ring buffer
  for(int i = 0; i < kRxdescNumber; i++) {
    E1000RxDesc *rxdesc = &rx_desc_buf_[i];
    rxdesc->bufAddr = rx_buf_;
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
  _mmioAddr[kRegTdbal] = v2p(reinterpret_cast<virt_addr>(tx_desc_buf_)) & 0xffffffff; // must be 16B-aligned
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
    txdesc->bufAddr = tx_buf_;
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
  // see pcie-gbe-controllers 5.2.1

  // notify start bit and addr
  _mmioAddr[kRegEerd] = (((addr << 8) & 0xff) | 1);
  
  // polling
//  while(!(_mmioAddr[kRegEerd] & (1 << 1))) {
  while(!(_mmioAddr[kRegEerd] & (1 << 4))) {
    // busy-wait
  }

  return (_mmioAddr[kRegEerd] >> 16) & 0xffff;
}