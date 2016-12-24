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

#include <x86.h>
#include <dev/pci.h>
#include <buf.h>
#include <dev/usb/usb.h>
#include <timer.h>

// for debug
#include <tty.h>
#include <global.h>

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
    virtual bool SendControlTransfer(UsbCtrl::DeviceRequest *request, virt_addr data, size_t data_size, int device_addr) override {
      return _dev_ehci->SendControlTransfer(request, data, data_size, device_addr);
    }
  private:
    DevEhci *const _dev_ehci;
  } _controller_dev;

  class DevEhciSub {
  public:
    virtual void Init() = 0;
    virtual phys_addr GetRepresentativeQueueHead() = 0;
    virtual bool SendControlTransfer(UsbCtrl::DeviceRequest *request, virt_addr data, size_t data_size, int device_addr) = 0;
  } *_sub;
  
  class DevEhciSub32 : public DevEhciSub {
  public:
    virtual void Init() override;
    virtual phys_addr GetRepresentativeQueueHead() override {
      return v2p(ptr2virtaddr(_qh0));
    }
    virtual bool SendControlTransfer(UsbCtrl::DeviceRequest *request, virt_addr data, size_t data_size, int device_addr) override;
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
      uint32_t GetToken() {
	return READ_MEM_VOLATILE(&_token);
      }
      bool IsActiveOfStatus() {
        return READ_MEM_VOLATILE(&_token) & (1 << 7);
      }
      bool IsDataBufferErrorOfStatus() {
        return READ_MEM_VOLATILE(&_token) & (1 << 5);
      }
      void SetTokenAndBuffer(bool interrupt_on_complete, bool data_toggle, PacketIdentification pid, int total_bytes, virt_addr buf) {
        SetTokenAndBufferSub(interrupt_on_complete, data_toggle, pid, total_bytes);
        phys_addr buf_p = k2p(buf);
        phys_addr buf_p_end = buf_p + total_bytes;
        _buffer_pointer[0] = buf_p;
        buf_p = ((buf_p + 4096) / 4096) * 4096;
        for (int i = 1; i < 5; i++) {
          if (buf_p <= buf_p_end) {
            _buffer_pointer[i] = buf_p;
            buf_p += 4096;
          } else {
            _buffer_pointer[i] = 0;
          }
        }
      }
      void SetTokenAndBuffer(bool interrupt_on_complete, bool data_toggle, PacketIdentification pid, int total_bytes) {
        SetTokenAndBufferSub(interrupt_on_complete, data_toggle, pid, total_bytes);
        for (int i = 0; i < 5; i++) {
          _buffer_pointer[i] = 0;
        }
      }
    private:
      void SetTokenAndBufferSub(bool interrupt_on_complete, bool data_toggle, PacketIdentification pid, int total_bytes) {
        _token = 0;
        _token |= data_toggle ? (1 << 31) : 0;
        assert(total_bytes <= 0x5000);
        _token |= total_bytes << 16;
        _token |= interrupt_on_complete ? (1 << 15) : 0;
        _token |= static_cast<uint8_t>(pid) << 8;
        _token |= 1 << 7; // active
      }
      uint32_t _next_td;
      uint32_t _alt_next_td;
      uint32_t _token;
      uint32_t _buffer_pointer[5];
      void SetNextSub(phys_addr next, bool terminate) {
        _next_td = next | (terminate ? (1 << 0) : 0);
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
        _characteristics |= static_cast<uint8_t>(_speed) << 12;
        _capabilities = 0;
        _current_td = 0;
        _next_td = 1;
        _alt_next_td = 1;
        _token = 0;
        for (int i = 0; i < 5; i++) {
          _buffer_pointer[i] = 0;
        }
      }
      void Init(EndpointSpeed speed) {
        _current_td = 0;
        _alt_next_td = 1;
        _token = 0;
        for (int i = 0; i < 5; i++) {
          _buffer_pointer[i] = 0;
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
        _next_td = v2p(ptr2virtaddr(next));
      }
      void SetNextTd(/* nullptr */) {
        _next_td = 1;
      }
      void SetCharacteristics(int max_packetsize, UsbCtrl::TransactionType ttype, bool head, bool data_toggle_control, uint8_t endpoint_num, bool inactivate, uint8_t device_address) {
        _characteristics = 0;
        if (_speed == EndpointSpeed::kHigh) {
          assert(!inactivate);
        } else {
          if (ttype == UsbCtrl::TransactionType::kControl) {
            _characteristics |= (1 << 27); // control endpoint flag
          }
        }
        assert(max_packetsize <= 0x400);
        _characteristics |= max_packetsize << 16;
        _characteristics |= head ? (1 << 15) : 0;
        _characteristics |= data_toggle_control ? (1 << 14) : 0;
        _characteristics |= static_cast<uint8_t>(_speed) << 12;
        assert(endpoint_num < 0x10);
        _characteristics |= endpoint_num << 8;
        _characteristics |= inactivate ? (1 << 7) : 0;
        assert(device_address < 0x80);
        _characteristics |= device_address;
        _characteristics |= 0x40000000;
      }
      void SetCapabilities(uint8_t mult, uint8_t port_number, uint8_t hub_addr, uint8_t split_completion_mask, uint8_t interrupt_schedule_mask) {
        _capabilities = 0;
        assert(mult > 0 && mult <= 3);
        _capabilities |= mult << 30;
        if (_speed == EndpointSpeed::kHigh) {
          assert(port_number < 0x80);
          _capabilities |= port_number << 23;
          assert(hub_addr < 0x80);
          _capabilities |= hub_addr << 16;
          _capabilities |= split_completion_mask << 8;
        }
        _capabilities |= interrupt_schedule_mask;
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
  };

  class DevEhciSub64 : public DevEhciSub {
  public:
    virtual void Init() override;
    virtual phys_addr GetRepresentativeQueueHead() override {
      assert(false);
      return 0;
    }
    virtual bool SendControlTransfer(UsbCtrl::DeviceRequest *request, virt_addr data, size_t data_size, int device_addr) override {
      assert(false);
      return true;
    }
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

  // see Table2-9. USBCMD USB Command Register Bit Definitions
  uint32_t kOperationalRegUsbCmdFlagRunStop = 1 << 0;
  uint32_t kOperationalRegUsbCmdFlagHcReset = 1 << 1;
  uint32_t kOperationalRegUsbCmdFlagsPeriodicScheduleEnable = 1 << 4;
  uint32_t kOperationalRegUsbCmdFlagAsynchronousScheduleEnable = 1 << 5;

  // see Table2-10. USBSTS USB Status Register Bit Definitions
  uint32_t kOperationalRegUsbStsFlagHcHalted = 1 << 12;
  uint32_t kOperationalRegUsbStsFlagAsynchronousSchedule = 1 << 15;

  // see Table2-16. PORTSC Port Status and Control
  uint32_t kOperationalRegPortScFlagCurrentConnectStatus = 1 << 0;
  uint32_t kOperationalRegPortScFlagPortEnable = 1 << 2;
  uint32_t kOperationalRegPortScFlagPortReset = 1 << 8;

  bool _is_64bit_addressing;
  bool SendControlTransfer(UsbCtrl::DeviceRequest *request, virt_addr data, size_t data_size, int device_addr) {
    return _sub->SendControlTransfer(request, data, data_size, device_addr);
  }

  void DisablePort(int port) {
    _operational_reg_base_addr[kOperationalRegOffsetPortScBase + port] &= ~kOperationalRegPortScFlagPortEnable;
  }
  void ResetPort(int port) {
    DisablePort(port);
    _operational_reg_base_addr[kOperationalRegOffsetPortScBase + port] |= kOperationalRegPortScFlagPortReset;
    // set reset bit for 50ms
    timer->BusyUwait(50*1000);
    // then unset reset bit
    _operational_reg_base_addr[kOperationalRegOffsetPortScBase + port] &= ~kOperationalRegPortScFlagPortReset;
    // wait until end of reset sequence
    while ((READ_MEM_VOLATILE(&_operational_reg_base_addr[kOperationalRegOffsetPortScBase + port]) & kOperationalRegPortScFlagPortEnable) == 0) {
    }
  }
};

#endif /* __RAPH_KERNEL_DEV_USB_EHCI_H__ */
