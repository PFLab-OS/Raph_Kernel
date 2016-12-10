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
 * reference: Enhanced Host Controller Interface Specification for Universal Serial Bus
 * 
 */

#ifndef __RAPH_KERNEL_DEV_USB_EHCI_H__
#define __RAPH_KERNEL_DEV_USB_EHCI_H__

#include <dev/pci.h>
#include <buf.h>
#include <dev/usb/usb11.h>

class DevEhci final : public DevPci {
public:
  DevEhci(uint8_t bus, uint8_t device, uint8_t function) : DevPci(bus, device, function), _controller_dev(this) {
  }
  static DevPci *InitPci(uint8_t bus, uint8_t device, uint8_t function);
  void Init();
private:
  class DevEhciUsbController : public DevUsbController {
  public:
    DevEhciUsbController(DevEhci *dev_ehci) : _dev_ehci(dev_ehci) {
    }
    virtual ~DevEhciUsbController() {
    }
    virtual bool GetDeviceDescriptor(Usb11Ctrl::DeviceDescriptor *desc, int device_addr) override {
      return _dev_ehci->GetDeviceDescriptor(desc, device_addr);
    }
  private:
    DevEhci *const _dev_ehci;
  } _controller_dev;

  class DevEhciSub {
  public:
    virtual void Init() = 0;
  } *_sub;
  
  class DevEhciSub32 : public DevEhciSub {
  public:
    virtual void Init() override;

  private:
    class QueueHead;
  
    class TransferDescriptor {
    public:
      enum class PacketIdentification : uint8_t {
        kOut = 0b00,
        kIn = 0b01,
        kSetup = 0b10,
      };
      void Init() {
        _alt_next_td = 1;
      }
      void SetNext(TransferDescriptor *next) {
        SetNextSub(v2p(ptr2virtaddr(next)), false);
      }
      void SetNext(/* nullptr */) {
        SetNextSub(0, true);
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
      void SetTokenAndBuffer(bool interrupt_on_complete, bool data_toggle, PacketIdentification pid, int total_bytes, virt_addr buf) {
        _token = 0;
        _token |= data_toggle ? (1 << 31) : 0;
        assert(total_bytes <= 0x5000);
        _token |= total_bytes << 
        _token |= interrupt_on_complete ? (1 << 15) : 0;
        _token |= 1 << 7; // active
      }
    private:
      uint32_t _next_td;
      uint32_t _alt_next_td;
      uint32_t _token;
      uint32_t _buffer_pointer[5];
      void SetNextSub(phys_addr next, bool terminate) {
        _next = next | (terminate ? (1 << 0) : 0);
      }
      void SetBufferSub(phys_addr pointer) {
        _buf = pointer;
      }
    } __attribute__((__packed__));
    static_assert(sizeof(TransferDescriptor) == 32, "");
    RingBuffer<TransferDescriptor *, 128> _td_buf;

    class QueueHead {
    public:
      enum class EndpointSpeed : uint8_t {
        kFull = 0,
        kLow = 1,
        kHigh = 2,
      };
      void InitEmpty() {
        _speed = EndpointSpeed::kHigh;
        _horizontal_next = v2p(ptr2virtaddr(this)) | (1 << 1);
        _characteristics = 0;
        _characteristics |= 1 << 15;
        _characteristics |= static_assert<uint8_t>(_speed) << 12;
        _capabilities = 0;
        _current_td = 0;
        _next_td = 1;
        _alt_next_td = 1;
        _token = 0;
        for (int i = 0; i < 5; i++) {
          buffer_pointer[i] = 0;
        }
      }
      void Init(EndpointSpeed speed) {
        current_td = 0;
        alt_net_td = 1;
        assert(false);
        _token = 0;
        for (int i = 0; i < 5; i++) {
          buffer_pointer[i] = 0;
        }
        _speed = speed;
      }
      void SetHorizontalNext(QueueHead *next) {
        SetHorizontalNextSub(v2p(ptr2virtaddr(next)), 1, false);
      }
      void SetHorizontalNext(/* nullptr */) {
        SetHorizontalNextSub(0, 0, true);
      }
      void SetNextTd(TransferDescriptor *next) {
        phys_addr next = v2p(ptr2virtaddr(next));
        _next_td = next;
      }
      void SetNextTd(/* nullptr */) {
        _next_td = 1;
      }
      void SetCharacteristics(int maxpacket_size, Usb11Ctrl::TransactionType ttype, bool head, bool data_toggle_control, uint8_t endpoint_num, bool inactivate, uint8_t device_address) {
        _characteristics = 0;
        if (_speed == kHigh) {
          assert(!inactivate);
        } else {
          if (ttype == kControl) {
            _characteristics |= (1 << 27); // control endpoint flag
          }
        }
        assert(max_packetsize <= 0x400);
        _characteristics |= max_packetsize << 16;
        _characteristics |= head ? (1 << 15) : 0;
        _characteristics |= data_toggle_control ? (1 << 14) : 0;
        _characteristics |= static_assert<uint8_t>(_speed) << 12;
        assert(endpoint_num < 0x10);
        _characteristics |= endpoint_num << 8;
        _characteristics |= inactivate ? (1 << 7) : 0;
        assert(device_address < 0x80);
        _characteristics |= device_address;
      }
      void SetCapabilities(uint8_t mult, uint8_t port_number, uint8_t hub_addr, uint8_t split_completion_mask, uint8_t interrupt_schedule_mask) {
        _capabilities = 0;
        assert(mult > 0 && mult <= 3);
        _capabilities |= mult << 30;
        if (speed == kHigh) {
          assert(port_number < 0x80);
          _capabilities |= port_number << 23;
          assert(hub_addr < 0x80);
          _capabilities |= hub_addr << 16;
          _capabilities |= split_completion_mask << 8;
        }
        _capabilities |= interrupt_schedule_mask;
      }
      void SetToken() {
      }
    private:
      void SetHorizontalNextSub(phys_addr next, int type, bool terminate) {
        assert(type < 4);
        _horizontal_next = next | (type << 1) | (terminate ? (1 << 0) : 0);
      }
      uint32_t _horizontal_next;
      uint32_t _characteristics;
      uint32_t _capabilities;
      uint32_t _current_td;
      uint32_t _next_td;
      uint32_t _alt_next_td;
      uint32_t _token;
      uint32_t _buffer_pointer[5];
      // for padding & extra info for driver
      EndpointSpeed _speed;
      uint8_t _padding_1;
      uint8_t _padding_2;
      uint8_t _padding_3;
      uint32_t _padding[3];
    } __attribute__((__packed__));
    static_assert(sizeof(QueueHead) == 64, "");
    QueueHead *_qh0; // empty
    RingBuffer<QueueHead *, 63> _qh_buf;

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
        assert((next & 0xF) == 0);
        framelist_pointer = (next & 0xFFFFFFF0) | (q ? (1 << 1) : 0);
      }
    } __attribute__((__packed__));

    struct FrameList {
      FramePointer entries[1024];
    } __attribute__((__packed__));
    FrameList *_frlist;
  };

  class DevEhciSub64 : public DevEhciSub {
  public:
    virtual void Init() override;
  };
    
  // see 2.1.3 USBBASE - Register Space Base Address Register
  static const uint16_t kBaseAddressReg = 0x10;

  volatile uint8_t *_capability_reg_base_addr;
  // see 2.2 Host Controller Capability Registers
  volatile uint8_t ReadCapabilityRegCapLength() {
    return _capability_reg_base_addr[0x0];
  }
  volatile uint16_t ReadCapabilityRegHciVersion() {
    return *(reinterpret_cast<volatile uint16_t *>(_capability_reg_base_addr + 0x2));
  }
  volatile uint32_t ReadCapabilityRegHcsParams() {
    return *(reinterpret_cast<volatile uint32_t *>(_capability_reg_base_addr + 0x4));
  }
  volatile uint32_t ReadCapabilityRegHccParams() {
    return *(reinterpret_cast<volatile uint32_t *>(_capability_reg_base_addr + 0x8));
  }
  volatile uint64_t ReadCapabilityRegHcspPortRoute() {
    return *(reinterpret_cast<volatile uint64_t *>(_capability_reg_base_addr + 0xC));
  }

  volatile uint32_t *_operational_reg_base_addr;
  // see 2.3 Host Controller Operational Registers
  int kOperationalRegOffsetUsbCmd = 0x00;
  int kOperationalRegOffsetUsbSts = 0x01;
  int kOperationalRegOffsetUsbIntr = 0x02;
  int kOperationalRegOffsetFrIndex = 0x03;
  int kOperationalRegOffsetCtrlDsSegment = 0x04;
  int kOperationalRegOffsetPeriodicListBase = 0x05;
  int kOperationalRegOffsetAsyncListAddr = 0x06;
  int kOperationalRegOffsetConfigFlag = 0x10;
  int kOperationalRegOffsetPortScBase = 0x11;

  bool GetDeviceDescriptor(Usb11Ctrl::DeviceDescriptor *desc, int device_addr);
};

#endif /* __RAPH_KERNEL_DEV_USB_EHCI_H__ */
