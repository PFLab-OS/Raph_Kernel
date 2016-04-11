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
#include <spinlock.h>
#include <freebsd/sys/param.h>

class NetDev {
public:
  struct Packet {
    size_t len;
    uint8_t buf[MCLBYTES];
  };
  enum class LinkStatus {
    Up,
    Down
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

  void ReuseRxBuffer(Packet *packet) {
    kassert(_rx_reserved.Push(packet));
  }
  void ReuseTxBuffer(Packet *packet) {
    kassert(_tx_reserved.Push(packet));
  }
  // 戻り値がfalseの時はバッファが枯渇しているので、要リトライ
  bool GetTxPacket(Packet *&packet) {
    if (_tx_reserved.Pop(packet)) {
      packet->len = 0;
      return true;
    } else {
      return false;
    }
  }
  bool TransmitPacket(Packet *packet) {
    return _tx_buffered.Push(packet);
  }
  bool ReceivePacket(Packet *&packet) {
    return _rx_buffered.Pop(packet);
  }
  void SetReceiveCallback(int apicid, const Function &func) {
    _rx_buffered.SetFunction(apicid, func);
  }

  void InitTxPacketBuffer() {
    while(!_tx_reserved.IsFull()) {
      Packet *packet = reinterpret_cast<Packet *>(virtmem_ctrl->Alloc(sizeof(Packet)));
      kassert(_tx_reserved.Push(packet));
    }
  }
  void InitRxPacketBuffer() {
    while(!_rx_reserved.IsFull()) {
      Packet *packet = reinterpret_cast<Packet *>(virtmem_ctrl->Alloc(sizeof(Packet)));
      kassert(_rx_reserved.Push(packet));
    }
  }

  virtual void UpdateLinkStatus() = 0;
  void SetStatus(LinkStatus status) {
    _status = status;
  }
  volatile LinkStatus GetStatus() {
    return _status;
  }

  void SetName(const char *name) {
    strncpy(_name, name, kNetworkInterfaceNameLen);
  }
  const char *GetName() { return _name; }
 protected:
  NetDev() {}
  SpinLock _lock;
  volatile LinkStatus _status = LinkStatus::Down;

  // IP address
  uint32_t _ipAddr = 0;
 private:
  static const uint32_t kNetworkInterfaceNameLen = 8;
  // network interface name
  char _name[kNetworkInterfaceNameLen];
};

class NetDevCtrl {
public:
  NetDevCtrl() {}

  bool RegisterDevice(NetDev *dev, const char *name = kDefaultNetworkInterfaceName);
  NetDev *GetDevice(const char *name = kDefaultNetworkInterfaceName);

protected:
  static const uint32_t kMaxDevNumber = 32;

private:
  static const uint32_t kNetworkInterfaceNameLen = 8;
  static const char *kDefaultNetworkInterfaceName;
  uint32_t _curDevNumber = 0;
  NetDev *_devTable[kMaxDevNumber] = {nullptr};
};

#endif /* __RAPH_KERNEL_NETDEV_H__ */
