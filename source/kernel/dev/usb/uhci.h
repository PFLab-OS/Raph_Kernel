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
 * reference: Universal Host Controller Interface (UHCI) Design Guide REVISION 1.1
 * 
 */

#ifndef __RAPH_KERNEL_DEV_USB_UHCI_H__
#define __RAPH_KERNEL_DEV_USB_UHCI_H__

#include <dev/pci.h>
#include <buf.h>
#include <dev/usb/usb.h>

class DevUhci final : public DevPci {
public:
  DevUhci(uint8_t bus, uint8_t device, uint8_t function) : DevPci(bus, device, function) {
  }
  static DevPci *InitPci(uint8_t bus, uint8_t device, uint8_t function);
  void Init();
private:

  class QueueHead;
  
  class TransferDescriptor {
  public:
    enum class PacketIdentification : uint8_t {
      kIn = 0x69,
      kOut = 0xE1,
      kSetup = 0x2D,
    };
    void SetNext(TransferDescriptor *next, bool depth_first) {
      SetNextSub(v2p(ptr2virtaddr(next)), depth_first, false, false);
    }
    void SetNext(QueueHead *next, bool depth_first) {
      SetNextSub(v2p(ptr2virtaddr(next)), depth_first, true, false);
    }
    void SetNext(/* nullptr */ bool depth_first) {
      SetNextSub(0, depth_first, false, true);
    }
    void SetStatus(bool interrupt, bool isochronous, bool low_speed, bool short_packet) {
      _status = (short_packet ? (1 << 29) : 0) | (low_speed ? (1 << 26) : 0) | (isochronous ? (1 << 25) : 0) | (interrupt ? (1 << 24) : 0) | (1 << 23);
    }
    bool IsActiveOfStatus() {
      return __sync_fetch_and_or(&_status, 0) & (1 << 23);
    }
    bool IsStalledOfStatus() {
      return __sync_fetch_and_or(&_status, 0) & (1 << 22);
    }
    bool IsCrcErrorOfStatus() {
      return __sync_fetch_and_or(&_status, 0) & (1 << 18);
    }
    // for debug
    uint32_t GetStatus() {
      return __sync_fetch_and_or(&_status, 0);
    }
    void SetToken(PacketIdentification pid, int device_address, int endpoint, bool data_toggle, int max_len) {
      assert(max_len >= 0 && max_len <= 1280);
      max_len--;
      if (max_len < 0) {
        max_len = 0x7FF;
      }
      assert(endpoint >= 0 && endpoint <= 0xF);
      assert(device_address >= 0 && device_address <= 0x7F);
      
      _token = (max_len << 21) | (data_toggle ? (1 << 19) : 0) | (endpoint << 15) | (device_address << 8) | static_cast<uint8_t>(pid);
    }
    void SetBuffer(UsbCtrl::DeviceRequest *pointer, int offset) {
      SetBufferSub(v2p(ptr2virtaddr(pointer)) + offset);
    }
    void SetBuffer(UsbCtrl::DeviceDescriptor *pointer, int offset) {
      SetBufferSub(v2p(ptr2virtaddr(pointer)) + offset);
    }
    void SetBuffer(/* nullptr */) {
      SetBufferSub(0);
    }
    uint32_t _next;
    uint32_t _status;
    uint32_t _token;
    uint32_t _buf;
  private:
    void SetNextSub(phys_addr next, bool depth_first, bool q, bool terminate) {
      assert(next <= 0xFFFFFFFF);
      assert((next & 0xF) == 0);
      _next = (next & 0xFFFFFFF0) | (depth_first ? (1 << 2) : 0) | (q ? (1 << 1) : 0) | (terminate ? (1 << 0) : 0);
    }
    void SetBufferSub(phys_addr pointer) {
      assert(pointer <= 0xFFFFFFFF);
      _buf = pointer;
    }
  } __attribute__((__packed__));
  static_assert(sizeof(TransferDescriptor) == 16, "");
  RingBuffer<TransferDescriptor *, 256> _td_buf;

  class QueueHead {
  public:
    void SetHorizontalNext(QueueHead *next) {
      SetHorizontalNextSub(v2p(ptr2virtaddr(next)), true, false);
    }
    void SetHorizontalNext(TransferDescriptor *next) {
      SetHorizontalNextSub(v2p(ptr2virtaddr(next)), false, false);
    }
    void SetHorizontalNext(/* nullptr */) {
      SetHorizontalNextSub(0, false, true);
    }
    void SetVerticalNext(QueueHead *next) {
      SetVerticalNextSub(v2p(ptr2virtaddr(next)), true);
    }
    void SetVerticalNext(TransferDescriptor *next) {
      SetVerticalNextSub(v2p(ptr2virtaddr(next)), false);
    }
  private:
    void SetHorizontalNextSub(phys_addr next, bool q, bool terminate) {
      assert(next <= 0xFFFFFFFF);
      assert((next & 0xF) == 0);
      horizontal_next = (next & 0xFFFFFFF0) | (q ? (1 << 1) : 0) | (terminate ? (1 << 0) : 0);
    }
    void SetVerticalNextSub(phys_addr next, bool q) {
      assert(next <= 0xFFFFFFFF);
      assert((next & 0xF) == 0);
      vertical_next = (next & 0xFFFFFFF0) | (q ? (1 << 1) : 0);
    }
    uint32_t horizontal_next;
    uint32_t vertical_next;
    // for 16 byte boundaly
    uint32_t padding1;
    uint32_t padding2;
  } __attribute__((__packed__));
  static_assert(sizeof(QueueHead) == 16, "");
  RingBuffer<QueueHead *, 256> _qh_buf;

  class FramePointer {
  public:
    uint32_t framelist_pointer;
    void Set() {
      // nullptr
      framelist_pointer = 0x1;
    }
    void Set(QueueHead *next) {
      SetSub(v2p(ptr2virtaddr(next)), true);
    }
    void Set(TransferDescriptor *next) {
      SetSub(v2p(ptr2virtaddr(next)), false);
    }
  private:
    void SetSub(phys_addr next, bool q) {
      assert(next <= 0xFFFFFFFF);
      assert((next & 0xF) == 0);
      framelist_pointer = (next & 0xFFFFFFF0) | (q ? (1 << 1) : 0);
    }
  } __attribute__((__packed__));

  struct FrameList {
    FramePointer entries[1024];
  } __attribute__((__packed__));
  FrameList *_frlist;
  
  // see 2.2 PCI Configuration Registers(USB)
  static const uint16_t kBaseAddressReg = 0x20;

  uint16_t _base_addr = 0;

  // see 2.1 USB I/O Registers
  uint16_t kCtrlRegCmd = 0x00;
  uint16_t kCtrlRegStatus = 0x02;
  uint16_t kCtrlRegIntrEn = 0x04;
  uint16_t kCtrlRegFrNum = 0x06;
  uint16_t kCtrlRegFlBaseAddr = 0x08;
  uint16_t kCtrlRegSof = 0x0C;
  uint16_t kCtrlRegPort1 = 0x10;
  uint16_t kCtrlRegPort2 = 0x12;

  uint16_t kCtrlRegStatusFlagHalted = 1 << 5;

  template<class T> volatile T ReadControllerReg(uint16_t reg);

  template<class T> void WriteControllerReg(uint16_t reg, T data);

  uint16_t GetCurrentFameListIndex();

  bool SendControlTransfer(UsbCtrl::DeviceRequest *request, virt_addr data, size_t data_size, int device_addr);
};

template <> inline volatile uint16_t DevUhci::ReadControllerReg<uint16_t>(uint16_t reg) {
  return inw(_base_addr + reg);
}

template <> inline void DevUhci::WriteControllerReg<uint16_t>(uint16_t reg, uint16_t data) {
  outw(_base_addr + reg, data);
}

template <> inline void DevUhci::WriteControllerReg(uint16_t reg, uint32_t data) {
  outl(_base_addr + reg, data);
}

inline uint16_t DevUhci::GetCurrentFameListIndex() {
  return ReadControllerReg<uint16_t>(kCtrlRegFrNum) & 1023;
}

#endif /* __RAPH_KERNEL_DEV_USB_UHCI_H__ */
