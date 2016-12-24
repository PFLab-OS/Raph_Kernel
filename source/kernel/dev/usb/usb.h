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
 * reference: Universal Serial Bus Specification Revision 1.1
 * 
 */

#ifndef __RAPH_KERNEL_DEV_USB_USB11_H__
#define __RAPH_KERNEL_DEV_USB_USB11_H__

#include <stdint.h>
#include <buf.h>
#include <mem/physmem.h>

// maximum data payload size for Control Transfers
// quoted from 5.5.3 Control Transfer Packet Size Constraints
//
// The USB defines the allowable maximum control data payload sizes
// for full-speed devices to be either 8, 16, 32, or 64 bytes.
// Low-speed devices are limited to only an eight-byte maximum data
// payload size.

class UsbCtrl {
public:
  // see Table 9-4 Standard Request Codes
  enum class RequestCode : uint8_t {
    kGetStatus = 0,
    kClearFeature = 1,
    kSetFeature = 3,
    kSetAddress = 5,
    kGetDescriptor = 6,
    kSetDescriptor = 7,
    kGetConfiguration = 8,
    kSetConfiguration = 9,
    kGetInterface = 10,
    kSetInterface = 11,
    kSynchFrame = 12,
  };

  enum class TransactionType {
    kBulk,
    kControl,
    kInterrupt,
    kIsochronous,
  };
  
  // see Table 9-5 Descriptor Types
  enum class DescriptorType : uint8_t {
    kDevice = 0x1,
    kConfiguration = 0x2,
    kString = 0x3,
    kInterface = 0x4,
    kEndpoint = 0x5,
  };
  
  // see Table 9-7 Standard Device Descriptor
  class DeviceDescriptor {
  public:
    uint8_t length;
    uint8_t type;
    uint16_t usb_release_number;
    uint8_t class_code;
    uint8_t subclass_code;
    uint8_t protocol_code;
    uint8_t max_packet_size;
    uint16_t vendor_id;
    uint16_t product_id;
    uint16_t device_release_number;
    uint8_t manufacture_index;
    uint8_t product_index;
    uint8_t serialnum_index;
    uint8_t config_num;
  } __attribute__((__packed__));
  static_assert(sizeof(DeviceDescriptor) == 18, "");

  // see Table 9-8 Standard Configuration Descriptor
  class ConfigurationDescriptor {
  public:
    uint8_t length;
    uint8_t type;
    uint16_t total_length;
    uint8_t num_interfaces;
    uint8_t configuration_value;
    uint8_t configuration_index;
    uint8_t attributes;
    uint8_t max_power;
  } __attribute__((__packed__));
  static_assert(sizeof(ConfigurationDescriptor) == 9, "");

  // see Table 9-9 Standard Interface Descriptor
  class InterfaceDescriptor {
  public:
    uint8_t length;
    uint8_t type;
    uint8_t interface_number;
    uint8_t alternate_setting;
    uint8_t num_endpoints;
    uint8_t class_code;
    uint8_t subclass_code;
    uint8_t protocol_code;
    uint8_t index;
  } __attribute__((__packed__));
  static_assert(sizeof(InterfaceDescriptor) == 9, "");

  // see Table 9-10 Standard Endpoint Desciptor
  class EndpointDescriptor {
  public:
    uint8_t length;
    uint8_t type;
    uint8_t address;
    uint8_t attributes;
    uint16_t max_packet_size;
    uint8_t interval;
  } __attribute__((__packed__));
  static_assert(sizeof(EndpointDescriptor) == 7, "");

  class DummyDescriptor {
  public:
    uint8_t length;
    uint8_t type;
  } __attribute__((__packed__));
  static_assert(sizeof(DummyDescriptor) == 2, "");

  // see Table 9-2
  class DeviceRequest {
  public:
    phys_addr GetPhysAddr() {
      return v2p(ptr2virtaddr(this));
    }
    void MakePacketOfGetDescriptorRequest(DescriptorType desc_type, uint16_t index, uint8_t length) {
      // see 9.4.3
      _request_type = 0b10000000;
      _request = static_cast<uint8_t>(RequestCode::kGetDescriptor);
      _value = (static_cast<uint16_t>(desc_type) << 8) + index;
      _index = 0;
      _length = length;
    }
    void MakePacketOfSetAddress(uint16_t device_address) {
      // see 9.4.6
      _request_type = 0b00000000;
      _request = static_cast<uint8_t>(RequestCode::kSetAddress);
      _value = device_address;
      _index = 0;
      _length = 0;
    }
    bool IsDirectionDeviceToHost() {
      return (_request_type & 0b10000000) != 0;
    }
  private:
    uint8_t _request_type;
    uint8_t _request;
    uint16_t _value;
    uint16_t _index;
    uint16_t _length;
  } __attribute__((__packed__));
  static_assert(sizeof(DeviceRequest) == 8, "");

  static UsbCtrl &GetCtrl() {
    kassert(_is_initialized);
    return _ctrl;
  }
  static void Init();
  bool AllocDeviceRequest(DeviceRequest *&request) {
    return _dr_buf.Pop(request);
  }
  bool ReuseDeviceRequest(DeviceRequest *request) {
    return _dr_buf.Push(request);
  }
private:
  static UsbCtrl _ctrl;
  static bool _is_initialized;

  RingBuffer<DeviceRequest *, 512> _dr_buf;
};

class DevUsbController {
public:
  virtual bool SendControlTransfer(UsbCtrl::DeviceRequest *request, virt_addr data, size_t data_size, int device_addr) = 0;
};

// !!! important !!!
// child class of DevUsb must implement a following function.
// static DevUsb *InitUsb(DevUsbController *controller, int addr);
class DevUsb {
public:
  DevUsb(DevUsbController *controller, int addr) : _controller(controller), _addr(addr) {
  }
  ~DevUsb() {
    delete[] _combined_desc;
  }
protected:
  void LoadDeviceDescriptor();
  void LoadCombinedDescriptors();
  UsbCtrl::DeviceDescriptor *GetDeviceDescriptor() {
    return &_device_desc;
  }
  UsbCtrl::InterfaceDescriptor *GetInterfaceDescriptorInCombinedDescriptors(int desc_index) {
    return reinterpret_cast<UsbCtrl::InterfaceDescriptor *>(GetDescriptorInCombinedDescriptors(UsbCtrl::DescriptorType::kInterface, desc_index));
  }
  UsbCtrl::EndpointDescriptor *GetEndpointDescriptorInCombinedDescriptors(int desc_index) {
    return reinterpret_cast<UsbCtrl::EndpointDescriptor *>(GetDescriptorInCombinedDescriptors(UsbCtrl::DescriptorType::kEndpoint, desc_index));
  }
  int GetDescriptorNumInCombinedDescriptors(UsbCtrl::DescriptorType type);
private:
  UsbCtrl::DummyDescriptor *GetDescriptorInCombinedDescriptors(UsbCtrl::DescriptorType type, int desc_index);
  DevUsbController * const _controller;
  const int _addr;
  UsbCtrl::DeviceDescriptor _device_desc;
  uint8_t *_combined_desc;
};

class DevUsbKeyboard : public DevUsb {
public:
  DevUsbKeyboard(DevUsbController *controller, int addr) : DevUsb(controller, addr) {
  }
  static DevUsb *InitUsb(DevUsbController *controller, int addr);
  void Init();
private:
  DevUsbKeyboard();
};

#endif // __RAPH_KERNEL_DEV_USB_USB11_H__

