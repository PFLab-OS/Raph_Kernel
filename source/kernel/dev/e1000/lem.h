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

#ifndef __RAPH_KERNEL_E1000_LEM_H__
#define __RAPH_KERNEL_E1000_LEM_H__

#include <stdint.h>
#include <mem/physmem.h>
#include <mem/virtmem.h>
#include <polling.h>
#include <global.h>
#include <dev/pci.h>
#include <buf.h>
#include <freebsd/sys/param.h>

struct adapter;

class lE1000;
struct BsdDriver {
  lE1000 *parent;
  struct adapter *adapter;
};

class lE1000 : public DevPCI, Polling {
public:
 lE1000(uint8_t bus, uint8_t device, bool mf) : DevPCI(bus, device, mf) {}
  static bool InitPCI(uint16_t vid, uint16_t did, uint8_t bus, uint8_t device, bool mf);
  // from Polling
  void Handle() override;
  BsdDriver bsd;

  struct Packet {
    size_t len;
    uint8_t buf[MCLBYTES];
  };
  // rxパケットの処理の流れ
  // 0. rx_reservedを初期化、バッファを満タンにしておく
  // 1. Recieveハンドラがパケットを受信すると、rx_reservedから一つ取り出し、
  //    memcpyの上、rx_bufferedに詰む
  // 2. プロトコル・スタックはRecievePacket関数を呼ぶ
  // 3. RecievePacket関数はrx_bufferedからパケットを取得する
  // 4. プロトコル・スタックは取得したパケットを処理した上でReuseRxBufferを呼ぶ
  // 5. ReuseRxBufferはrx_reservedにバッファを返す
  // 6. 1に戻る
  //
  // プロトコル・スタックがReuseRxBufferを呼ばないと
  // そのうちrx_reservedが枯渇して、一切のパケットの受信ができなくなるるよ♪
  RingBuffer<Packet *, 300> _rx_reserved;
  RingBuffer<Packet *, 300> _rx_buffered;

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
  RingBuffer<Packet *, 300> _tx_reserved;
  RingBuffer<Packet *, 300> _tx_buffered;

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
  bool RecievePacket(Packet *&packet) {
    return _rx_buffered.Pop(packet);
  }
  // allocate 6 byte before call
  void GetEthAddr(uint8_t *buffer);
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
 private:
};

#endif /* __RAPH_KERNEL_E1000_LEM_H__ */
