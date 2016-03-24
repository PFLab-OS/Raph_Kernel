/*
 *
 * Copyright (c) 2016 Project Raphine
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
 * Author: Levelfour
 * 
 */

#include "../raph.h"
#include "../mem/physmem.h"
#include "../mem/virtmem.h"
#include "../dev/netdev.h"
#include "eth.h"
#include "arp.h"
#include "ip.h"
#include "udp.h"
#include "tcp.h"

#define __NETCTRL__
#include "global.h"

DevEthernetCtrl *netdev_ctrl;
ARPTable *arp_table;

EthCtrl *eth_ctrl;
ARPCtrl *arp_ctrl;
IPCtrl *ip_ctrl;
UDPCtrl *udp_ctrl;
TCPCtrl *tcp_ctrl;

void InitNetCtrl() {
  netdev_ctrl = new(reinterpret_cast<DevEthernetCtrl*>(virtmem_ctrl->Alloc(sizeof(DevEthernetCtrl)))) DevEthernetCtrl();

  arp_table = new(reinterpret_cast<ARPTable*>(virtmem_ctrl->Alloc(sizeof(ARPTable)))) ARPTable();

  eth_ctrl = new(reinterpret_cast<EthCtrl*>(virtmem_ctrl->Alloc(sizeof(EthCtrl)))) EthCtrl();

  arp_ctrl = new(reinterpret_cast<ARPCtrl*>(virtmem_ctrl->Alloc(sizeof(ARPCtrl)))) ARPCtrl();

  ip_ctrl = new(reinterpret_cast<IPCtrl*>(virtmem_ctrl->Alloc(sizeof(IPCtrl)))) IPCtrl();

  udp_ctrl = new(reinterpret_cast<UDPCtrl*>(virtmem_ctrl->Alloc(sizeof(UDPCtrl)))) UDPCtrl();

  tcp_ctrl = new(reinterpret_cast<TCPCtrl*>(virtmem_ctrl->Alloc(sizeof(TCPCtrl)))) TCPCtrl();
}

void DismissNetCtrl() {
  netdev_ctrl->~NetDevCtrl();
  arp_table->~ARPTable();
  eth_ctrl->~EthCtrl();
  arp_ctrl->~ARPCtrl();
  ip_ctrl->~IPCtrl();
  udp_ctrl->~UDPCtrl();
  tcp_ctrl->~TCPCtrl();
  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(netdev_ctrl));
  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(arp_table));
  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(eth_ctrl));
  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(arp_ctrl));
  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(ip_ctrl));
  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(udp_ctrl));
  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(tcp_ctrl));
}
