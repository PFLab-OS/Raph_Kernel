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
 */

#ifndef __RAPH_KERNEL_NETDEV_H__
#define __RAPH_KERNEL_NETDEV_H__

#include <stdint.h>
#include <string.h>
#include <buf.h>
#include <function.h>
#include <spinlock.h>
#include <polling.h>
#include <freebsd/sys/param.h>
#include <_cpu.h>
#include <ptr.h>
#include <array.h>

class ProtocolStack;
class DevEthernet;

/**
 * A class representing network device.
 * This class is an abstruct class, and must be derived.
 */
class NetDev {
public:

  /**
   * Network packet structure.
   *
   * In protocol stack layers, Packet::buf will be incremented in order to
   * represent each layer packet. For example, in IP layer, which is
   * L3 protocol, Packet::buf will be incremented by 14, which is the length
   * of Ethernet header. Thus Packet::buf will represent an IP packet.
   */
  struct Packet {
  public:
    Packet() {}
    size_t len;
    uint8_t *GetBuffer() {
      return _data;
    }
  private:
    uint8_t _data[MCLBYTES];  // only accessed via buf
  };

  enum class LinkStatus {
    kUp,
    kDown,
  };

  enum class HandleMethod {
    kInt,
    kPolling,
  };

  typedef RingBuffer<Packet *, 300> NetDevRingBuffer;
  typedef FunctionalRingBuffer<Packet *, 300> NetDevFunctionalRingBuffer;

  // rxパケットの処理の流れ
  // 0. rx_reservedを初期化、バッファを満タンにしておく
  // 1. Receiveハンドラがパケットを受信すると、rx_reservedから一つ取り出し、
  //    memcpyの上、rx_bufferedに詰む
  // 2. プロトコル・スタックはReceivePacket関数を呼ぶ
  // 3. ReceivePacket関数はrx_bufferedからパケットを取得する
  // 4. プロトコル・スタックは取得したパケットを処理した上でReuseRxBufferを呼ぶ
  // 5. ReuseRxBufferはrx_reservedにバッファを返す
  // 6. 1に戻る
  //
  // プロトコル・スタックがReuseRxBufferを呼ばないと
  // そのうちrx_reservedが枯渇して、一切のパケットの受信ができなくなるるよ♪
  NetDevRingBuffer _rx_reserved;
  NetDevFunctionalRingBuffer _rx_buffered;

  // txパケットの処理の流れ
  // 0. tx_reservedを初期化、バッファを満タンにしておく
  // 1. プロトコル・スタックはGetTxBufferを呼び出す
  // 2. GetTxBufferはtx_reservedからバッファを取得する
  // 3. プロトコル・スタックはバッファにmemcpyして、TransmitPacket関数を呼ぶ
  // 4. TransmitPacket関数はtx_bufferedにパケットを詰む
  // 5. Transmitハンドラがパケットを処理した上でtx_reservedに返す
  // 6. 1に戻る
  //
  // プロトコル・スタックはGetTxBufferで確保したバッファを必ずTransmitPacketするか
  // ReuseTxBufferで開放しなければならない。サボるとそのうちtx_reservedが枯渇
  // して、一切のパケットの送信ができなくなるよ♪
  NetDevRingBuffer _tx_reserved;
  NetDevFunctionalRingBuffer _tx_buffered;

  /**
   * Return received packets to the network device.
   * DO NOT forget to call this method after received packets,
   * or network device will be out of resources for receiving.
   *
   * @param packet a packet will be freed.
   */
  void ReuseRxBuffer(Packet *packet) {
    kassert(_rx_reserved.Push(packet));
  }

  /**
   * Return transmitted packets to the network device.
   * DO NOT forget to call this method if you do not transmit the packet
   * once fetched with NetDev::GetTxPacket, or network device will be
   * out of resources for transmission.
   *
   * @param packet a packet will be freed.
   */
  void ReuseTxBuffer(Packet *packet) {
    kassert(_tx_reserved.Push(packet));
  }

  /**
   * Fetch a buffer for packet transmission.
   * If you receives false, you should call it again.
   * DO NOT forget to call either NetDev::TransmitPacket or NetDev::ReuseTxBuffer.
   *
   * @param packet
   * @return if fetched buffer successfully.
   */
  bool GetTxPacket(Packet *&packet) {
    if (_tx_reserved.Pop(packet)) {
      packet->len = MCLBYTES;
      return true;
    } else {
      return false;
    }
  }

  /**
   * Transmit a packet.
   * This method just queues packet into the buffer.
   *
   * @param packet
   * @return if packet is queued into the buffer successfully.
   */
  bool TransmitPacket(Packet *packet) {
    return _tx_buffered.Push(packet);
  }

  /**
   * Receive a packet.
   * Usually, you shuold use this method in the callback function set by
   * NetDev::SetReceiveCallback. That callback will be called when some packets
   * arrived.
   *
   * @param packet
   * @param if the received packet is fetched successfully.
   */
  bool ReceivePacket(Packet *&packet) {
    if (_rx_buffered.Pop(packet)) {
      return true;
    } else {
      return false;
    }
  }

  /**
   * Set a callback function called when some packets arrived.
   *
   * @param cpuid specify a core executing the callback.
   * @param func callback function.
   */
  void SetReceiveCallback(CpuId cpuid, uptr<GenericFunction<>> func) {
    _rx_buffered.SetFunction(cpuid, func);
  }

  /**
   * Initialize transmission packet buffer.
   */
  void InitTxPacketBuffer() {
    while(!_tx_reserved.IsFull()) {
      Packet *packet_addr = reinterpret_cast<Packet *>(virtmem_ctrl->Alloc(sizeof(Packet)));
      Packet *packet = new(packet_addr) Packet();
      kassert(_tx_reserved.Push(packet));
    }
  }

  /**
   * Initialize reception packet buffer.
   */
  void InitRxPacketBuffer() {
    while(!_rx_reserved.IsFull()) {
      Packet *packet_addr = reinterpret_cast<Packet *>(virtmem_ctrl->Alloc(sizeof(Packet)));
      Packet *packet = new(packet_addr) Packet();
      kassert(_rx_reserved.Push(packet));
    }
  }

  /**
   * Update link status of the network device, which depends on specific
   * physical devices.
   */
  virtual void UpdateLinkStatus() = 0;

  /**
   * Set link status.
   *
   * @param status
   */
  void SetStatus(LinkStatus status) {
    _status = status;
  }

  /**
   * Get link status.
   *
   * @return status
   */
  volatile LinkStatus GetStatus() {
    return _status;
  }

  /**
   * Check if link is up.
   *
   * @return if link is already up.
   */
  virtual bool IsLinkUp() = 0;

  /**
   * Set network interface name.
   *
   * @param name interface name.
   */
  void SetName(const char *name) {
    strncpy(_name, name, kNetworkInterfaceNameLen);
  }

  /**
   * Get network interface name.
   *
   * @return name
   */
  const char *GetName() { return _name; }

  void SetHandleMethod(HandleMethod method) {
    switch(method) {
    case HandleMethod::kInt: {
      ChangeHandleMethodToInt();
      break;
    }
    case HandleMethod::kPolling: {
      ChangeHandleMethodToPolling();
      break;
    }
    default: {
      kassert(false);
    }
    }
    _method = method;
  }

  HandleMethod GetHandleMethod() {
    return _method;
  }

  /**
   * Set up the network interface. Usually you must register this interface
   * to network device controller (NetDevCtrl).
   * Other initialization can be done in this method.
   *
   * @param prefix name of interface
   */
  virtual void SetupNetInterface(const char *prefix) = 0;

  /**
   * Simplified method of SetupNetInterface(const char *)
   */
  void SetupNetInterface() {
    SetupNetInterface("eth");
  }

  /**
   * Set protocol stack to this device.
   *
   * @param stack
   */
  void SetProtocolStack(ProtocolStack *stack) { _ptcl_stack = stack; }

  /**
   * Assign IPv4 address to the device.
   *
   * @param addr IPv4 address.
   * @return if the device supports IPv4 address or not.
   */
  virtual bool AssignIpv4Address(uint32_t addr) {
    return false;
  }

  /**
   * Get IPv4 address.
   *
   * @param addr buffer to return.
   * @return if the device supports IPv4 address or not.
   */
  virtual bool GetIpv4Address(uint32_t &addr) {
    return false;
  }

protected:
  NetDev();
  virtual void ChangeHandleMethodToPolling() = 0;
  virtual void ChangeHandleMethodToInt() = 0;
  virtual void Transmit(void *) = 0;
  SpinLock _lock;
  PollingFunc _polling;
  volatile LinkStatus _status = LinkStatus::kDown;
  HandleMethod _method = HandleMethod::kInt;

private:
  static const uint32_t kNetworkInterfaceNameLen = 8;
  // network interface name
  char _name[kNetworkInterfaceNameLen];

  // reference to protocol stack
  ProtocolStack *_ptcl_stack;
};


/**
 * A class which manages network devices (NetDev instances).
 * This class is controller, which must be singleton.
 *
 * After NetDev is instantiated, it has to be registered to this controller.
 * Network devices can be fetched, using methods provided by this controller.
 */
class NetDevCtrl {
public:
  struct NetDevInfo {
    NetDev *device;
    ProtocolStack *ptcl_stack;
  };

  NetDevCtrl() {}

  /**
   * Register network device to this controller.
   * Network device must be instantiated beforehand.
   *
   * @param dev instantiated network device (subclass of NetDev).
   * @param prefix prefix of the interface name, e.g., "en", "wl", etc.
   * @return if registered successfully.
   */
  bool RegisterDevice(NetDev *dev, const char *prefix);

  /**
   * Fetch the network device information specified by its interface name.
   *
   * @param name interface name.
   * @return netdev_info the pair of network device and corresponding protocol stack.
   */
  NetDevInfo *GetDeviceInfo(const char *name);
  
  /**
   * Get names list of all network devices
   *
   * @return unique pointer to the list
   */
  uptr<Array<const char *>> GetNamesOfAllDevices();
  
  /**
   * Check if the specified network device exists or not.
   *
   * @param name interface name.
   * @return if the interface exists.
   */
  bool Exists(const char *name) {
    return GetDeviceInfo(name) != nullptr;
  }

  /** maximum length of interface names */
  static const uint32_t kNetworkInterfaceNameLen = 8;

protected:
  /** maximum number of network devices */
  static const uint32_t kMaxDevNumber = 32;

private:
  /** current device number */
  uint32_t _current_device_number = 0;

  /** table of network devices */
  NetDevInfo _dev_table[kMaxDevNumber];
};

#endif /* __RAPH_KERNEL_NETDEV_H__ */
