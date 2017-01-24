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
#include <tty.h>
#include <list.h>

class DevEhci final : public DevPci {
public:
  DevEhci(uint8_t bus, uint8_t device, uint8_t function) : DevPci(bus, device, function), _int_task(new Task), _controller_dev(this) {
  }
  static DevPci *InitPci(uint8_t bus, uint8_t device, uint8_t function);
  void Init();
private:
  sptr<Task> _int_task;

  class DevEhciSubBase {
  public:
    virtual void Init() = 0;
    virtual phys_addr GetRepresentativeQueueHead() = 0;
    virtual bool SendControlTransfer(UsbCtrl::DeviceRequest *request, virt_addr data, size_t data_size, int device_addr) = 0;
    virtual phys_addr GetPeriodicFrameList() = 0;
    virtual sptr<DevUsbController::Manager> SetupInterruptTransfer(uint8_t endpt_address, int device_addr, int interval, UsbCtrl::PacketIdentification direction, int max_packetsize, int num_td, uint8_t *buffer, uptr<GenericFunction<uptr<Array<uint8_t>>>> func) = 0;
    virtual int GetPeriodicFrameListEntryNum() {
      return 1024;
    }
    virtual void CheckQueuedTdIfCompleted() = 0;
  } *_sub;
  
  class QueueHead32;
  class QueueHead64;

  class FrameListElementPointer {
  public:
    void Set(/* nullptr */) {
      SetSub(0, 0, true);
    }
    void Set(QueueHead32 *ptr) {
      SetSub(v2p(ptr2virtaddr(ptr)), 1, false);
    }
    void Set(QueueHead64 *ptr) {
      SetSub(v2p(ptr2virtaddr(ptr)), 1, false);
    }
  private:
    void SetSub(phys_addr ptr, uint8_t type, bool terminate) {
      _ptr = ptr | (type << 1) | (terminate ? 1 : 0);
    }
    uint32_t _ptr;
  };
  static_assert(sizeof(FrameListElementPointer) == 4, "");

  struct TdContainer {
    bool interrupt_on_complete;
    bool data_toggle;
    UsbCtrl::PacketIdentification pid;
    int total_bytes;
    uint8_t *buf;
  };
  class TransferDescriptor32 {
  public:
    void Init() {
      _alt_next_td = 1;
      _func = make_uptr<GenericFunction<>>();
    }
    void SetNext(TransferDescriptor32 *next) {
      SetNextSub(v2p(ptr2virtaddr(next)), false);
    }
    void SetNext(/* nullptr */) {
      SetNextSub(0, true);
    }
    uint32_t GetToken() {
      return _token;
    }
    uint8_t GetStatus() {
      return _token & 0xFF;
    }
    bool IsActiveOfStatus() {
      return _token & (1 << 7);
    }
    bool IsDataBufferErrorOfStatus() {
      return _token & (1 << 5);
    }
    virt_addr GetBuffer() {
      return p2v(_buffer_pointer[0]);
    }
    void SetTokenAndBuffer(TdContainer &container) {
      SetTokenAndBuffer(container.interrupt_on_complete, container.data_toggle, container.pid, container.total_bytes, ptr2virtaddr(container.buf));
    }
    void SetTokenAndBuffer(bool interrupt_on_complete, bool data_toggle, UsbCtrl::PacketIdentification pid, int total_bytes, virt_addr buf) {
      SetTokenAndBufferSub(interrupt_on_complete, data_toggle, pid, total_bytes);
      phys_addr buf_p = k2p(buf);
      phys_addr buf_p_end = buf_p + total_bytes;
      if (buf_p_end > 0xFFFFFFFF) {
        kernel_panic("Ehci", "64-bit address is not supported");
      }
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
    void SetTokenAndBuffer(bool interrupt_on_complete, bool data_toggle, UsbCtrl::PacketIdentification pid, int total_bytes) {
      SetTokenAndBufferSub(interrupt_on_complete, data_toggle, pid, total_bytes);
      for (int i = 0; i < 5; i++) {
        _buffer_pointer[i] = 0;
      }
    }
    void SetFunc(uptr<GenericFunction<>> func) {
      _func = func;
    }
    void Execute() {
      _func->Execute();
    }
  private:
    void SetTokenAndBufferSub(bool interrupt_on_complete, bool data_toggle, UsbCtrl::PacketIdentification pid, int total_bytes) {
      _token = 0;
      _token |= data_toggle ? (1 << 31) : 0;
      assert(total_bytes <= 0x5000);
      _token |= total_bytes << 16;
      _token |= interrupt_on_complete ? (1 << 15) : 0;
      _token |= 3 << 10;
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
    //  extra info for driver
    friend DevEhciSubBase;
    uptr<GenericFunction<>> _func;
  } __attribute__((__packed__)) __attribute__ ((aligned (32)));

  class TransferDescriptor64 {
  public:
    void Init() {
      _alt_next_td = 1;
      _func = make_uptr<GenericFunction<>>();
    }
    void SetNext(TransferDescriptor64 *next) {
      SetNextSub(v2p(ptr2virtaddr(next)), false);
    }
    void SetNext(/* nullptr */) {
      SetNextSub(0, true);
    }
    uint32_t GetToken() {
      return _token;
    }
    uint8_t GetStatus() {
      return _token & 0xFF;
    }
    bool IsActiveOfStatus() {
      return _token & (1 << 7);
    }
    bool IsDataBufferErrorOfStatus() {
      return _token & (1 << 5);
    }
    virt_addr GetBuffer() {
      return p2v(_buffer_pointer[0]);
    }
    void SetTokenAndBuffer(TdContainer &container) {
      SetTokenAndBuffer(container.interrupt_on_complete, container.data_toggle, container.pid, container.total_bytes, ptr2virtaddr(container.buf));
    }
    void SetTokenAndBuffer(bool interrupt_on_complete, bool data_toggle, UsbCtrl::PacketIdentification pid, int total_bytes, virt_addr buf) {
      SetTokenAndBufferSub(interrupt_on_complete, data_toggle, pid, total_bytes);
      phys_addr buf_p = k2p(buf);
      phys_addr buf_p_end = buf_p + total_bytes;
      _buffer_pointer[0] = buf_p;
      _buffer_pointer_ex[0] = buf_p >> 32;
      buf_p = ((buf_p + 4096) / 4096) * 4096;
      for (int i = 1; i < 5; i++) {
        if (buf_p <= buf_p_end) {
          _buffer_pointer[i] = buf_p;
          _buffer_pointer_ex[i] = buf_p >> 32;
          buf_p += 4096;
        } else {
          _buffer_pointer[i] = 0;
          _buffer_pointer_ex[i] = 0;
        }
      }
    }
    void SetTokenAndBuffer(bool interrupt_on_complete, bool data_toggle, UsbCtrl::PacketIdentification pid, int total_bytes) {
      SetTokenAndBufferSub(interrupt_on_complete, data_toggle, pid, total_bytes);
      for (int i = 0; i < 5; i++) {
        _buffer_pointer[i] = 0;
      }
    }
    void SetFunc(uptr<GenericFunction<>> func) {
      _func = func;
    }
    void Execute() {
      _func->Execute();
    }
  private:
    void SetTokenAndBufferSub(bool interrupt_on_complete, bool data_toggle, UsbCtrl::PacketIdentification pid, int total_bytes) {
      _token = 0;
      _token |= data_toggle ? (1 << 31) : 0;
      assert(total_bytes <= 0x5000);
      _token |= total_bytes << 16;
      _token |= interrupt_on_complete ? (1 << 15) : 0;
      _token |= 3 << 10;
      _token |= static_cast<uint8_t>(pid) << 8;
      _token |= 1 << 7; // active
    }
    uint32_t _next_td;
    uint32_t _alt_next_td;
    uint32_t _token;
    uint32_t _buffer_pointer[5];
    uint32_t _buffer_pointer_ex[5];
    void SetNextSub(phys_addr next, bool terminate) {
      _next_td = next | (terminate ? (1 << 0) : 0);
    }
    //  extra info for driver
    friend DevEhciSubBase;
    uptr<GenericFunction<>> _func;
  } __attribute__((__packed__)) __attribute__ ((aligned (32)));

  class QueueHead32 {
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
    void SetHorizontalNext(QueueHead32 *next) {
      SetHorizontalNextSub(v2p(ptr2virtaddr(next)), 1, false);
    }
    void SetHorizontalNext(/* nullptr */) {
      SetHorizontalNextSub(0, 0, true);
    }
    void SetNextTd(TransferDescriptor32 *next) {
      _next_td = v2p(ptr2virtaddr(next));
    }
    void SetNextTd(/* nullptr */) {
      _next_td = 1;
    }
    bool IsNextTdNull() {
      return (_next_td & 1) != 0;
    }
    void SetCharacteristics(int max_packetsize, UsbCtrl::TransferType ttype, bool head, bool data_toggle_control, uint8_t endpoint_num, bool inactivate, uint8_t device_address) {
      _characteristics = 0;
      if (_speed == EndpointSpeed::kHigh) {
        assert(!inactivate);
      } else {
        if (ttype == UsbCtrl::TransferType::kControl) {
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
    //  extra info for driver
    EndpointSpeed _speed;
  } __attribute__((__packed__)) __attribute__ ((aligned (32)));

  class QueueHead64 {
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
    void SetHorizontalNext(QueueHead64 *next) {
      SetHorizontalNextSub(v2p(ptr2virtaddr(next)), 1, false);
    }
    void SetHorizontalNext(/* nullptr */) {
      SetHorizontalNextSub(0, 0, true);
    }
    void SetNextTd(TransferDescriptor64 *next) {
      _next_td = v2p(ptr2virtaddr(next));
    }
    void SetNextTd(/* nullptr */) {
      _next_td = 1;
    }
    bool IsNextTdNull() {
      return (_next_td & 1) != 0;
    }
    void SetCharacteristics(int max_packetsize, UsbCtrl::TransferType ttype, bool head, bool data_toggle_control, uint8_t endpoint_num, bool inactivate, uint8_t device_address) {
      _characteristics = 0;
      if (_speed == EndpointSpeed::kHigh) {
        assert(!inactivate);
      } else {
        if (ttype == UsbCtrl::TransferType::kControl) {
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
    uint32_t _buffer_pointer_ex[5];
    //  extra info for driver
    EndpointSpeed _speed;
  } __attribute__((__packed__)) __attribute__ ((aligned (32)));

  class DevEhciUsbController : public DevUsbController {
  public:
    DevEhciUsbController(DevEhci *dev_ehci) : _dev_ehci(dev_ehci) {
    }
    virtual ~DevEhciUsbController() {
    }
    virtual bool SendControlTransfer(UsbCtrl::DeviceRequest *request, virt_addr data, size_t data_size, int device_addr) override {
      return _dev_ehci->SendControlTransfer(request, data, data_size, device_addr);
    }
    virtual sptr<DevUsbController::Manager> SetupInterruptTransfer(uint8_t endpt_address, int device_addr, int interval, UsbCtrl::PacketIdentification direction, int max_packetsize, int num_td, uint8_t *buffer, uptr<GenericFunction<uptr<Array<uint8_t>>>> func) override {
      return _dev_ehci->SetupInterruptTransfer(endpt_address, device_addr, interval, direction, max_packetsize, num_td, buffer, func);
    }
  private:
    DevEhci *const _dev_ehci;
  } _controller_dev;

  template<class QueueHead, class TransferDescriptor>
  class DevEhciSub : public DevEhciSubBase {
  public:
    virtual void Init() override;
    virtual phys_addr GetRepresentativeQueueHead() override {
      return v2p(ptr2virtaddr(_qh0));
    }
    virtual phys_addr GetPeriodicFrameList() override {
      return v2p(ptr2virtaddr(_periodic_frame_list));
    }
    virtual bool SendControlTransfer(UsbCtrl::DeviceRequest *request, virt_addr data, size_t data_size, int device_addr) override;
    virtual sptr<DevUsbController::Manager> SetupInterruptTransfer(uint8_t endpt_address, int device_addr, int interval, UsbCtrl::PacketIdentification direction, int max_packetsize, int num_td, uint8_t *buffer, uptr<GenericFunction<uptr<Array<uint8_t>>>> func) override;
    virtual void CheckQueuedTdIfCompleted() override;
  private:
    class EhciManager : public DevUsbController::Manager {
    public:
      EhciManager(int num_td, QueueHead *qh, uptr<GenericFunction<uptr<Array<uint8_t>>>> func, DevEhciSub *master) : _num_td(num_td), _td_array(new TransferDescriptor*[num_td]), _container_array(new TdContainer[num_td]), _interrupt_qh(qh), _func(func), _master(master) {
      }
      virtual ~EhciManager() {
        delete[] _td_array;
        delete[] _container_array;
      }
      void CopyInfo(TransferDescriptor **td_array, TdContainer *container_array) {
        for (int i = 0; i < _num_td; i++) {
          _td_array[i] = td_array[i];
          _container_array[i] = container_array[i];
        }
      }
      static void HandleInterrupt(wptr<EhciManager> manager, int index) {
        manager->HandleInterruptSub(index);
      }
      void HandleInterruptSub(int index);
    private:
      EhciManager();
      const int _num_td;
      TransferDescriptor **_td_array;
      TdContainer *_container_array;
      QueueHead *_interrupt_qh;
      uptr<GenericFunction<uptr<Array<uint8_t>>>> _func;
      DevEhciSub * const _master;
    };
    
    FrameListElementPointer *_periodic_frame_list;
    QueueHead *_qh0; // empty
    RingBuffer<QueueHead *, PagingCtrl::kPageSize / sizeof(QueueHead)> _qh_buf;
    RingBuffer<TransferDescriptor *, PagingCtrl::kPageSize / sizeof(TransferDescriptor)> _td_buf;
    List<TransferDescriptor *> _queueing_td_buf;
    
    void HandleCompletedStruct(QueueHead *qh , uptr<Array<TransferDescriptor *>> td_array);
  };
  using DevEhciSub32 = DevEhciSub<QueueHead32, TransferDescriptor32>;
  using DevEhciSub64 = DevEhciSub<QueueHead64, TransferDescriptor64>;

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

  volatile uint32_t *_op_reg_base_addr;
  // see 2.3 Host Controller Op Registers
  int kOpRegOffsetUsbCmd = 0x00;
  int kOpRegOffsetUsbSts = 0x01;
  int kOpRegOffsetUsbIntr = 0x02;
  int kOpRegOffsetFrIndex = 0x03;
  int kOpRegOffsetCtrlDsSegment = 0x04;
  int kOpRegOffsetPeriodicListBase = 0x05;
  int kOpRegOffsetAsyncListAddr = 0x06;
  int kOpRegOffsetConfigFlag = 0x10;
  int kOpRegOffsetPortScBase = 0x11;

  // see Table2-9. USBCMD USB Command Register Bit Definitions
  uint32_t kOpRegUsbCmdFlagRunStop = 1 << 0;
  uint32_t kOpRegUsbCmdFlagHcReset = 1 << 1;
  uint32_t kOpRegUsbCmdOffsetFrameListSize = 0b11 << 2;
  uint32_t kOpRegUsbCmdFlagPeriodicScheduleEnable = 1 << 4;
  uint32_t kOpRegUsbCmdFlagAsynchronousScheduleEnable = 1 << 5;

  // see Table2-10. USBSTS USB Status Register Bit Definitions
  uint32_t kOpRegUsbStsFlagInterrupt = 1 << 0;
  uint32_t kOpRegUsbStsFlagHcHalted = 1 << 12;
  uint32_t kOpRegUsbStsFlagPeriodicSchedule = 1 << 14;
  uint32_t kOpRegUsbStsFlagAsynchronousSchedule = 1 << 15;

  // see Table2-11. USBINTR USB Interrupt Enable Register
  uint32_t kOpRegUsbIntrFlagInterruptEnable = 1 << 0;
  uint32_t kOpRegUsbIntrFlagErrorInterruptEnable = 1 << 1;
  uint32_t kOpRegUsbIntrFlagPortChangeInterruptEnable = 1 << 2;
  uint32_t kOpRegUsbIntrFlagFrameListRolloverEnable = 1 << 3;
  uint32_t kOpRegUsbIntrFlagHostSystemErrorEnable = 1 << 4;
  uint32_t kOpRegUsbIntrFlagAsyncAdvanceEnable = 1 << 5;

  // see Table2-15. CONFIGFLAG Configure Flag Register Bit Definitions
  uint32_t kOpRegConfigFlag = 1 << 0;

  // see Table2-16. PORTSC Port Status and Control
  uint32_t kOpRegPortScFlagCurrentConnectStatus = 1 << 0;
  uint32_t kOpRegPortScFlagPortEnable = 1 << 2;
  uint32_t kOpRegPortScFlagPortReset = 1 << 8;

  bool _is_64bit_addressing;
  bool SendControlTransfer(UsbCtrl::DeviceRequest *request, virt_addr data, size_t data_size, int device_addr) {
    return _sub->SendControlTransfer(request, data, data_size, device_addr);
  }
  virtual sptr<DevUsbController::Manager> SetupInterruptTransfer(uint8_t endpt_address, int device_addr, int interval, UsbCtrl::PacketIdentification direction, int max_packetsize, int num_td, uint8_t *buffer, uptr<GenericFunction<uptr<Array<uint8_t>>>> func) {
    return _sub->SetupInterruptTransfer(endpt_address, device_addr, interval, direction, max_packetsize, num_td, buffer, func);
  }

  void DisablePort(int port) {
    _op_reg_base_addr[kOpRegOffsetPortScBase + port] &= ~kOpRegPortScFlagPortEnable;
  }
  void ResetPort(int port) {
    DisablePort(port);
    _op_reg_base_addr[kOpRegOffsetPortScBase + port] |= kOpRegPortScFlagPortReset;
    // set reset bit for 50ms
    timer->BusyUwait(50*1000);
    // then unset reset bit
    _op_reg_base_addr[kOpRegOffsetPortScBase + port] &= ~kOpRegPortScFlagPortReset;
    // wait until end of reset sequence
    while ((_op_reg_base_addr[kOpRegOffsetPortScBase + port] & kOpRegPortScFlagPortEnable) == 0) {
      asm volatile("":::"memory");
    }
  }
  void HandlerSub();
  static void Handler(void *arg) {
    auto that = reinterpret_cast<DevEhci *>(arg);
    that->HandlerSub();
  }
  void CheckQueuedTdIfCompleted(void *) {
    // tell controller to acknowledge interrupt
    _op_reg_base_addr[kOpRegOffsetUsbSts] &= ~kOpRegUsbStsFlagInterrupt;
    asm volatile("":::"memory");
    // enable new interrupt
    _op_reg_base_addr[kOpRegOffsetUsbSts] |= kOpRegUsbStsFlagInterrupt;
    _sub->CheckQueuedTdIfCompleted();
    // TODO check not only TDs but also all interrupt sources before enable interrupt
  }
};

#endif /* __RAPH_KERNEL_DEV_USB_EHCI_H__ */
