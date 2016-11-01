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
 * Author: levelfour
 * 
 */


#include <net/eth.h>
#include <net/ip.h>
#include <net/udp.h>
#include <dev/eth.h>


int UdpSocket::Open() {
  NetDevCtrl::NetDevInfo *devinfo = netdev_ctrl->GetDeviceInfo(_ifname);
  DevEthernet *device = static_cast<DevEthernet *>(devinfo->device);
  ProtocolStack *pstack = devinfo->ptcl_stack;

  uint8_t eth_addr[6];
  device->GetEthAddr(eth_addr);

  // stack construction
  // BaseLayer > EthernetLayer > Ipv4Layer > UdpLayer > UdpSocket
  ProtocolStackBaseLayer *base_layer_addr = reinterpret_cast<ProtocolStackBaseLayer *>(virtmem_ctrl->Alloc(sizeof(ProtocolStackBaseLayer)));
  ProtocolStackBaseLayer *base_layer = new(base_layer_addr) ProtocolStackBaseLayer();
  base_layer->Setup(nullptr);
  pstack->SetBaseLayer(base_layer);

  EthernetLayer *eth_layer_addr = reinterpret_cast<EthernetLayer *>(virtmem_ctrl->Alloc(sizeof(EthernetLayer)));
  EthernetLayer *eth_layer = new(eth_layer_addr) EthernetLayer();
  eth_layer->Setup(base_layer);
  eth_layer->SetAddress(eth_addr);
  eth_layer->SetUpperProtocolType(EthernetLayer::kProtocolIpv4);

  Ipv4Layer *ip_layer_addr = reinterpret_cast<Ipv4Layer *>(virtmem_ctrl->Alloc(sizeof(Ipv4Layer)));
  Ipv4Layer *ip_layer = new(ip_layer_addr) Ipv4Layer();
  ip_layer->Setup(eth_layer);
  ip_layer->SetAddress(_ipv4_addr);
  ip_layer->SetPeerAddress(_peer_addr);
  ip_layer->SetProtocol(Ipv4Layer::kProtocolUdp);

  UdpLayer *udp_layer_addr = reinterpret_cast<UdpLayer *>(virtmem_ctrl->Alloc(sizeof(UdpLayer)));
  UdpLayer *udp_layer = new(udp_layer_addr) UdpLayer();
  udp_layer->Setup(ip_layer);
  udp_layer->SetPort(_port);
  udp_layer->SetPeerPort(_peer_port);

  return this->Setup(udp_layer) ? 0 : -1;
}


bool UdpLayer::FilterPacket(NetDev::Packet *packet) {
  UdpLayer::Header *header = reinterpret_cast<UdpLayer::Header *>(packet->buf);

  if (_peer_port != UdpSocket::kPortAny && ntohs(header->sport) != _peer_port) {
    return false;
  }

  if (ntohl(header->dport) != _port) {
    return false;
  }

  return true;
}


bool UdpLayer::PreparePacket(NetDev::Packet *packet) {
  if (_peer_port == UdpSocket::kPortAny) {
    return false;
  }

  UdpLayer::Header *header = reinterpret_cast<UdpLayer::Header *>(packet->buf);

  header->sport = htons(_port);
  header->dport = htons(_peer_port);
  header->len = htons(packet->len);
  header->checksum = 0;  // checksum can be set to 0 (do not calculate)

  return true;
}
