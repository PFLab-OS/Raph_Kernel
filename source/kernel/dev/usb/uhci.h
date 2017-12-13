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
#include <list.h>
#include <thread.h>

class DevUhci final : public DevPci {
public:
  DevUhci(uint8_t bus, uint8_t device, uint8_t function) : DevPci(bus, device, function), _controller_dev(this) {
  }
  static DevPci *InitPci(uint8_t bus, uint8_t device, uint8_t function);
  void Init();
  virtual void Attach() override;
private:
  uptr<Thread> _int_thread;

  class QueueHead;
  
  struct TdContainer {
    bool interrupt_on_complete;
    bool isochronous;
    bool low_speed;
    bool short_packet;
    UsbCtrl::PacketIdentification pid;
    int device_address;
    int endpoint;
    bool data_toggle;
    int total_bytes;
    uint8_t *buf;
  };
  class TransferDescriptor {
  public:
    enum class PacketIdentification : uint8_t {
      kIn = 0x69,
      kOut = 0xE1,
      kSetup = 0x2D,
    };
    static PacketIdentification ConvertPacketIdentification(UsbCtrl::PacketIdentification pid) {
      switch(pid) {
      case UsbCtrl::PacketIdentification::kOut: {
        return PacketIdentification::kOut;
      }
      case UsbCtrl::PacketIdentification::kIn: {
        return PacketIdentification::kIn;
      }
      case UsbCtrl::PacketIdentification::kSetup: {
        return PacketIdentification::kSetup;
      }
      default: {
        assert(false);
      }
      }
    }
    void Init() {
      _next = 1;
      _status = 0;
      _token = 0;
      _buf = 0;
    }
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
    void SetToken(UsbCtrl::PacketIdentification pid, int device_address, int endpoint, bool data_toggle, int max_len) {
      assert(max_len >= 0 && max_len <= 1280);
      max_len--;
      if (max_len < 0) {
        max_len = 0x7FF;
      }
      assert(endpoint >= 0 && endpoint <= 0xF);
      assert(device_address >= 0 && device_address <= 0x7F);

      _token = (max_len << 21) | (data_toggle ? (1 << 19) : 0) | (endpoint << 15) | (device_address << 8) | static_cast<uint8_t>(ConvertPacketIdentification(pid));
    }
    void SetBuffer(virt_addr buf, int offset) {
      SetBufferSub(k2p(buf + offset));
    }
    void SetBuffer(/* nullptr */) {
      SetBufferSub(0);
    }
    void SetContainer(TdContainer &container) {
      SetStatus(container.interrupt_on_complete, container.isochronous, container.low_speed, container.short_packet);
      SetToken(container.pid, container.device_address, container.endpoint, container.data_toggle, container.total_bytes);
      SetBuffer(ptr2virtaddr(container.buf), 0);
    }
    void SetFunc(uptr<GenericFunction<>> func) {
      _func = func;
    }
    void Execute() {
      _func->Execute();
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
    //  extra info for driver
    friend DevUhci;
    uptr<GenericFunction<>> _func;
  } __attribute__((__packed__)) __attribute__ ((aligned (16)));
  RingBuffer<TransferDescriptor *, 128> _td_buf;

  class QueueHead {
  public:
    void InitEmpty() {
      horizontal_next = 1 << 0;
      vertical_next = 1 << 0;
    }
    void SetHorizontalNext(QueueHead *next) {
      SetHorizontalNextSub(v2p(ptr2virtaddr(next)), true, false);
    }
    void SetHorizontalNext(TransferDescriptor *next) {
      SetHorizontalNextSub(v2p(ptr2virtaddr(next)), false, false);
    }
    void SetHorizontalNext(/* nullptr */) {
      SetHorizontalNextSub(0, false, true);
    }
    void InsertHorizontalNext(QueueHead *next) {
      next->SetHorizontalNext(horizontal_next);
      SetHorizontalNextSub(v2p(ptr2virtaddr(next)), true, false);
    }
    void SetVerticalNext(QueueHead *next) {
      SetVerticalNextSub(v2p(ptr2virtaddr(next)), true, false);
    }
    void SetVerticalNext(TransferDescriptor *next) {
      SetVerticalNextSub(v2p(ptr2virtaddr(next)), false, false);
    }
    void SetVerticalNext(/* nullptr */) {
      SetVerticalNextSub(0, false, true);
    }
    bool IsVerticalNextNull() {
      return (vertical_next & 1) != 0;
    }
  private:
    void SetHorizontalNextSub(phys_addr next, bool q, bool terminate) {
      assert(next <= 0xFFFFFFFF);
      assert((next & 0xF) == 0);
      horizontal_next = (next & 0xFFFFFFF0) | (q ? (1 << 1) : 0) | (terminate ? (1 << 0) : 0);
    }
    void SetVerticalNextSub(phys_addr next, bool q, bool terminate) {
      assert(next <= 0xFFFFFFFF);
      assert((next & 0xF) == 0);
      vertical_next = (next & 0xFFFFFFF0) | (q ? (1 << 1) : 0) | (terminate ? (1 << 0) : 0);
    }
    void SetHorizontalNext(uint32_t next) {
      horizontal_next = next;
    }
    uint32_t horizontal_next;
    uint32_t vertical_next;
    // for 16 byte boundaly
    uint32_t padding1;
    uint32_t padding2;
  } __attribute__((__packed__));
  static_assert(sizeof(QueueHead) == 16, "");
  RingBuffer<QueueHead *, 256> _qh_buf;

  class UhciManager : public DevUsbController::Manager {
  public:
    UhciManager(int num_td, QueueHead *qh, uptr<GenericFunction<uptr<Array<uint8_t>>>> func, DevUhci *master) : _num_td(num_td), _td_array(new TransferDescriptor*[num_td]), _container_array(new TdContainer[num_td]), _interrupt_qh(qh), _func(func), _master(master) {
    }
    virtual ~UhciManager() {
      delete[] _td_array;
      delete[] _container_array;
    }
    void CopyInfo(TransferDescriptor **td_array, TdContainer *container_array) {
      for (int i = 0; i < _num_td; i++) {
        _td_array[i] = td_array[i];
        _container_array[i] = container_array[i];
      }
    }
    static void HandleInterrupt(wptr<UhciManager> manager, int index) {
      manager->HandleInterruptSub(index);
    }
    void HandleInterruptSub(int index);
  private:
    UhciManager();
    const int _num_td;
    TransferDescriptor **_td_array;
    TdContainer *_container_array;
    QueueHead *_interrupt_qh;
    uptr<GenericFunction<uptr<Array<uint8_t>>>> _func;
    DevUhci * const _master;
  };
  List<TransferDescriptor *> _queueing_td_buf;
  
  class FramePointer {
  public:
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
    uint32_t framelist_pointer;
  } __attribute__((__packed__));

  static const int kFrameListEntryNum = 1024;
  struct FrameList {
    FramePointer entries[kFrameListEntryNum];
  } __attribute__((__packed__));
  FrameList *_frlist;
  QueueHead *_qh_int[kFrameListEntryNum]; // interrupt queue head
  QueueHead *_qh_cb; // control and bulk head
  
  // see 2.2 PCI Configuration Registers(USB)
  static const uint16_t kBaseAddressReg = 0x20;

  uint16_t _base_addr = 0;

  // see 2.1 USB I/O Registers
  uint16_t kCtrlRegCmd = 0x00;
  uint16_t kCtrlRegStatus = 0x02;
  uint16_t kCtrlRegIntr = 0x04;
  uint16_t kCtrlRegFrNum = 0x06;
  uint16_t kCtrlRegFlBaseAddr = 0x08;
  uint16_t kCtrlRegSof = 0x0C;
  uint16_t kCtrlRegPortBase = 0x10;

  uint16_t kCtrlRegCmdFlagRunStop = 1 << 0;
  uint16_t kCtrlRegCmdFlagHcReset = 1 << 1;
  uint16_t kCtrlRegCmdFlagGlobalReset = 1 << 2;
  uint16_t kCtrlRegCmdFlagMaxPacket = 1 << 7;
  uint16_t kCtrlRegStatusFlagInt = 1 << 0;
  uint16_t kCtrlRegStatusFlagHalted = 1 << 5;
  uint16_t kCtrlRegIntrFlagIoc = 1 << 2;
  uint16_t kCtrlRegPortFlagCurrentConnectStatus = 1 << 0;
  uint16_t kCtrlRegPortFlagEnable = 1 << 2;
  uint16_t kCtrlRegPortFlagReset = 1 << 9;

  void DisablePort(int port);
  void EnablePort(int port);
  void ResetPort(int port);

  template<class T> volatile T ReadControllerReg(uint16_t reg);

  template<class T> void WriteControllerReg(uint16_t reg, T data);

  uint16_t GetCurrentFameListIndex();

  bool SendControlTransfer(UsbCtrl::DeviceRequest *request, virt_addr data, size_t data_size, int device_addr);
  sptr<DevUsbController::Manager> SetupInterruptTransfer(uint8_t endpt_address, int device_addr, int interval, UsbCtrl::PacketIdentification direction, int max_packetsize, int num_td, uint8_t *buffer, uptr<GenericFunction<uptr<Array<uint8_t>>>> func);

  void HandlerSub();
  static void Handler(void *arg) {
    auto that = reinterpret_cast<DevUhci *>(arg);
    that->HandlerSub();
  }
  
  void CheckQueuedTdIfCompleted(void *);

  class DevUhciUsbController : public DevUsbController {
  public:
    DevUhciUsbController(DevUhci *dev_uhci) : _dev_uhci(dev_uhci) {
    }
    virtual ~DevUhciUsbController() {
    }
    virtual bool SendControlTransfer(UsbCtrl::DeviceRequest *request, virt_addr data, size_t data_size, int device_addr) override {
      return _dev_uhci->SendControlTransfer(request, data, data_size, device_addr);
    }
    virtual sptr<DevUsbController::Manager> SetupInterruptTransfer(uint8_t endpt_address, int device_addr, int interval, UsbCtrl::PacketIdentification direction, int max_packetsize, int num_td, uint8_t *buffer, uptr<GenericFunction<uptr<Array<uint8_t>>>> func) override {
      return _dev_uhci->SetupInterruptTransfer(endpt_address, device_addr, interval, direction, max_packetsize, num_td, buffer, func);
    }
  private:
    DevUhci *const _dev_uhci;
  } _controller_dev;
};

template <> inline volatile uint16_t DevUhci::ReadControllerReg<uint16_t>(uint16_t reg) {
  return inw(_base_addr + reg);
}

template <> inline void DevUhci::WriteControllerReg<uint16_t>(uint16_t reg, uint16_t data) {
  outw(_base_addr + reg, data);
}

template <> inline void DevUhci::WriteControllerReg<uint32_t>(uint16_t reg, uint32_t data) {
  outl(_base_addr + reg, data);
}

inline uint16_t DevUhci::GetCurrentFameListIndex() {
  return ReadControllerReg<uint16_t>(kCtrlRegFrNum) & 1023;
}

inline void DevUhci::DisablePort(int port) {
  while(true) {
    WriteControllerReg<uint16_t>(kCtrlRegPortBase + port * 2, ReadControllerReg<uint16_t>(kCtrlRegPortBase + port * 2) & ~kCtrlRegPortFlagEnable);
    if ((ReadControllerReg<uint16_t>(kCtrlRegPortBase + port * 2) & kCtrlRegPortFlagEnable) == 0) {
      break;
    }
    asm volatile("":::"memory");
  }
}

inline void DevUhci::EnablePort(int port) {
  while(true) {
    WriteControllerReg<uint16_t>(kCtrlRegPortBase + port * 2, ReadControllerReg<uint16_t>(kCtrlRegPortBase + port * 2) | kCtrlRegPortFlagEnable);
    if ((ReadControllerReg<uint16_t>(kCtrlRegPortBase + port * 2) & kCtrlRegPortFlagEnable) != 0) {
      break;
    }
    asm volatile("":::"memory");
  }
}

inline void DevUhci::ResetPort(int port) {
  DisablePort(port);
  WriteControllerReg<uint16_t>(kCtrlRegPortBase + port * 2, ReadControllerReg<uint16_t>(kCtrlRegPortBase + port * 2) | kCtrlRegPortFlagReset);
  // set reset bit for 50ms
  timer->BusyUwait(50*1000);
  // then unset reset bit 
  WriteControllerReg<uint16_t>(kCtrlRegPortBase + port * 2, ReadControllerReg<uint16_t>(kCtrlRegPortBase + port * 2) & ~kCtrlRegPortFlagReset);
  EnablePort(port);
}

#endif /* __RAPH_KERNEL_DEV_USB_UHCI_H__ */
