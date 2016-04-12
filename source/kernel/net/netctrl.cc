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

#include <raph.h>
#include <global.h>
#include <mem/physmem.h>
#include <mem/virtmem.h>
#include <dev/netdev.h>
#include <net/eth.h>
#include <net/arp.h>
#include <net/ip.h>
#include <net/udp.h>
#include <net/tcp.h>

DevEthernetCtrl *netdev_ctrl;
ArpTable *arp_table;

EthCtrl *eth_ctrl;
ArpCtrl *arp_ctrl;
IpCtrl *ip_ctrl;
UdpCtrl *udp_ctrl;
TcpCtrl *tcp_ctrl;

void InitNetCtrl() {
  netdev_ctrl = new(reinterpret_cast<DevEthernetCtrl*>(virtmem_ctrl->Alloc(sizeof(DevEthernetCtrl)))) DevEthernetCtrl();

  arp_table = new(reinterpret_cast<ArpTable*>(virtmem_ctrl->Alloc(sizeof(ArpTable)))) ArpTable();

  eth_ctrl = new(reinterpret_cast<EthCtrl*>(virtmem_ctrl->Alloc(sizeof(EthCtrl)))) EthCtrl();

  arp_ctrl = new(reinterpret_cast<ArpCtrl*>(virtmem_ctrl->Alloc(sizeof(ArpCtrl)))) ArpCtrl();

  ip_ctrl = new(reinterpret_cast<IpCtrl*>(virtmem_ctrl->Alloc(sizeof(IpCtrl)))) IpCtrl();

  udp_ctrl = new(reinterpret_cast<UdpCtrl*>(virtmem_ctrl->Alloc(sizeof(UdpCtrl)))) UdpCtrl();

  tcp_ctrl = new(reinterpret_cast<TcpCtrl*>(virtmem_ctrl->Alloc(sizeof(TcpCtrl)))) TcpCtrl();
}

void DismissNetCtrl() {
  netdev_ctrl->~NetDevCtrl();
  arp_table->~ArpTable();
  eth_ctrl->~EthCtrl();
  arp_ctrl->~ArpCtrl();
  ip_ctrl->~IpCtrl();
  udp_ctrl->~UdpCtrl();
  tcp_ctrl->~TcpCtrl();
  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(netdev_ctrl));
  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(arp_table));
  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(eth_ctrl));
  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(arp_ctrl));
  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(ip_ctrl));
  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(udp_ctrl));
  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(tcp_ctrl));
}
