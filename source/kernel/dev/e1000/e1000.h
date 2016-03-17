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

#ifndef __RAPH_KERNEL_E1000_H__
#define __RAPH_KERNEL_E1000_H__

#include <stdint.h>
#include <mem/physmem.h>
#include <mem/virtmem.h>
#include <polling.h>
#include <global.h>
#include <dev/pci.h>
#include <buf.h>
#include <freebsd/sys/param.h>

struct adapter;

class E1000;
struct BsdDriver {
  E1000 *parent;
  struct adapter *adapter;
};

class E1000 : public DevPCI, Polling {
public:
 E1000(uint8_t bus, uint8_t device, bool mf) : DevPCI(bus, device, mf) {}
  static void InitPCI(uint16_t vid, uint16_t did, uint8_t bus, uint8_t device, bool mf);
  // from Polling
  void Handle() override;
  BsdDriver bsd;

  struct Packet {
    size_t len;
    uint8_t buf[MCLBYTES];
  };
  // rx$B%Q%1%C%H$N=hM}$NN.$l(B
  // 0. rx_reserved$B$r=i4|2=!"%P%C%U%!$rK~%?%s$K$7$F$*$/(B
  // 1. Recieve$B%O%s%I%i$,%Q%1%C%H$r<u?.$9$k$H!"(Brx_reserved$B$+$i0l$D<h$j=P$7!"(B
  //    memcpy$B$N>e!"(Brx_buffered$B$K5M$`(B
  // 2. $B%W%m%H%3%k!&%9%?%C%/$O(BRecievePacket$B4X?t$r8F$V(B
  // 3. RecievePacket$B4X?t$O(Brx_buffered$B$+$i%Q%1%C%H$r<hF@$9$k(B
  // 4. $B%W%m%H%3%k!&%9%?%C%/$O<hF@$7$?%Q%1%C%H$r=hM}$7$?>e$G(BReuseRxBuffer$B$r8F$V(B
  // 5. ReuseRxBuffer$B$O(Brx_reserved$B$K%P%C%U%!$rJV$9(B
  // 6. 1$B$KLa$k(B
  //
  // $B%W%m%H%3%k!&%9%?%C%/$,(BReuseRxBuffer$B$r8F$P$J$$$H(B
  // $B$=$N$&$A(Brx_reserved$B$,8O3i$7$F!"0l@Z$N%Q%1%C%H$N<u?.$,$G$-$J$/$J$k$k$h"v(B
  RingBuffer<Packet *, 30> _rx_reserved;
  RingBuffer<Packet *, 30> _rx_buffered;

  // tx$B%Q%1%C%H$N=hM}$NN.$l(B
  // 0. tx_reserved$B$r=i4|2=!"%P%C%U%!$rK~%?%s$K$7$F$*$/(B
  // 1. $B%W%m%H%3%k!&%9%?%C%/$O(BGetTxBuffer$B$r8F$S=P$9(B
  // 2. GetTxBuffer$B$O(Btx_reserved$B$+$i%P%C%U%!$r<hF@$9$k(B
  // 3. $B%W%m%H%3%k!&%9%?%C%/$O%P%C%U%!$K(Bmemcpy$B$7$F!"(BTransmitPacket$B4X?t$r8F$V(B
  // 4. TransmitPacket$B4X?t$O(Btx_buffered$B$K%Q%1%C%H$r5M$`(B
  // 5. Transmit$B%O%s%I%i$,%Q%1%C%H$r=hM}$7$?>e$G(Btx_reserved$B$KJV$9(B
  // 6. 1$B$KLa$k(B
  //
  // $B%W%m%H%3%k!&%9%?%C%/$O(BGetTxBuffer$B$G3NJ]$7$?%P%C%U%!$rI,$:(BTransmitPacket$B$9$k$+(B
  // ReuseTxBuffer$B$G3+J|$7$J$1$l$P$J$i$J$$!#%5%\$k$H$=$N$&$A(Btx_reserved$B$,8O3i(B
  // $B$7$F!"0l@Z$N%Q%1%C%H$NAw?.$,$G$-$J$/$J$k$h"v(B
  RingBuffer<Packet *, 30> _tx_reserved;
  RingBuffer<Packet *, 30> _tx_buffered;

  void ReuseRxBuffer(Packet *packet) {
    kassert(_rx_reserved.Push(packet));
  }
  void ReuseTxBuffer(Packet *packet) {
    kassert(_tx_reserved.Push(packet));
  }
  // $BLa$jCM$,(Bfalse$B$N;~$O%P%C%U%!$,8O3i$7$F$$$k$N$G!"MW%j%H%i%$(B
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
 private:
};

#endif /* __RAPH_KERNEL_E1000_H__ */
