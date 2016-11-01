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


#include <net/ip.h>
#include <net/arp.h>


bool Ipv4Layer::FilterPacket(NetDev::Packet *packet) {
  Ipv4Layer::Header *header = reinterpret_cast<Ipv4Layer::Header *>(packet->buf);

  if (header->pid != _upper_protocol) {
    return false;
  }

  if (ntohl(header->daddr) != _ipv4_addr) {
    return false;
  }

  if (_peer_addr != kAddressNotSet && ntohl(header->saddr) != _peer_addr) {
    return false;
  } else {
    // reset for next filtering, preventing from repeatedly using
    // the same peer address inadvertently
    ResetPeerAddress();
  }

  return true;
}


bool Ipv4Layer::PreparePacket(NetDev::Packet *packet) {
  Ipv4Layer::Header *header = reinterpret_cast<Ipv4Layer::Header *>(packet->buf);

  header->hlen = sizeof(Ipv4Layer::Header) >> 2;
  header->ver = kIpVersion;
  header->type = 0;
  header->total_len = htons(packet->len);
  header->id = _id++;
  header->frag_offset = htons(kFlagNoFragment);
  header->ttl = _ttl;
  header->pid = _upper_protocol;
  header->checksum = 0;
  header->saddr = htonl(_ipv4_addr);
  header->daddr = htonl(_peer_addr);

  header->checksum = Ipv4Layer::CheckSum(header, sizeof(Ipv4Layer::Header));

  return true;
}

bool Ipv4Layer::GetHardwareDestinationAddress(NetDev::Packet *packet, uint8_t *addr) {
  Ipv4Layer::Header *header = reinterpret_cast<Ipv4Layer::Header *>(packet->buf);
  return arp_table->Find(ntohl(header->daddr), addr);
}
