/*
 *
 * Copyright (c) 2017 Raphine Project
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * Author: mumumu
 *
 * This code is a devece driver for rtl8139 which is old realtek NIC.
 * Tested Environment:
 *    Qemu Emulation(only)
 *
 */

#pragma once

#include <polling.h>
#include <mem/paging.h>
#include <mem/virtmem.h>
#include <mem/physmem.h>
#include <tty.h>
#include <dev/netdev.h>
#include <dev/eth.h>
#include <dev/pci.h>
#include <global.h>
#include <stdint.h>
#include <thread.h>

class Rtl8139 : public DevPci {
 public:
  Rtl8139(uint8_t bus, uint8_t device, uint8_t function)
      : DevPci(bus, device, function), _eth(*this) {}

  static DevPci *InitPci(uint8_t bus, uint8_t device, uint8_t function);

 private:
  class Rtl8139Ethernet : public DevEthernet {
   public:
    Rtl8139Ethernet(Rtl8139 &master) : _master(master) {}

    static void PollingHandler(Rtl8139 *that);
    static void InterruptHandler(void *p);

    virtual void UpdateLinkStatus() override;

    // allocate 6 byte before call
    virtual void GetEthAddr(uint8_t *buffer) override;
    virtual void ChangeHandleMethodToPolling() override;
    virtual void ChangeHandleMethodToInt() override;
    virtual void Transmit(void *) override;
    void SetRxTxConfigRegs();
    void Setup();
    void Start();
    uint16_t ReadEeprom(uint16_t, uint16_t);
    static void CheckHwState(Rtl8139Ethernet *);

   private:
    Rtl8139 &_master;
    uint32_t _tx_descriptor_status = 0b1111;
    uint32_t _current_tx_descriptor = 0;
    PhysAddr _tx_buffer[4];
    PhysAddr _rx_buffer;
    uint32_t _rx_buffer_offset = 0;

    bool _tx_buffered_blocked = false;

    uptr<Thread> _status_check_thread;

    // Registers see datasheet p16
    static const uint32_t kRegTxAddr = 0x20;    // dw * 4
    static const uint32_t kRegTxStatus = 0x10;  // dw * 4
    static const uint32_t kRegTxConfig = 0x40;  // dw
    static const uint32_t kRegRxAddr = 0x30;
    static const uint32_t kRegRxConfig = 0x44;  // dw
    static const uint32_t kRegRxCAP = 0x38;
    static const uint32_t kRegCommand = 0x37;   // b
    static const uint32_t kRegIrStatus = 0x3e;  // w
    static const uint32_t kRegIrMask = 0x3c;    // w
    static const uint32_t kReg93C46Cmd = 0x50;  // b
    static const uint32_t kRegCurrentAddr = 0x3a;

    // Command  see datasheet p21
    static const uint8_t kCmdTxEnable = 0x4;
    static const uint8_t kCmdRxEnable = 0x8;
    static const uint8_t kCmdReset = 0x10;
    static const uint8_t kCmdRxBufEmpty = 0x1;

    // Ir Status
    static const uint16_t kIsrRok = 0b1;
    static const uint16_t kIsrTok = 0b100;
  };

  static const uint16_t kVendorId = 0x10ec;
  static const uint16_t kDeviceId = 0x8139;

  template <class T>
  void WriteReg(uint32_t offset, T data);

  template <class T>
  T ReadReg(uint32_t offset);

  uint32_t _mmio_addr = 0;

  virtual void Attach() override;

  Rtl8139Ethernet _eth;
};
