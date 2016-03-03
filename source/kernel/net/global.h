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

#ifndef __RAPH_KERNEL_NET_GLOBAL_H__
#define __RAPH_KERNEL_NET_GLOBAL_H__

// hide network controllers
// be careful to define __NETCTRL__ macro
// (intend to be included only in NetDev and its child class)
#ifdef __NETCTRL__

// Network Devices
class NetDevCtrl;
extern NetDevCtrl *netdev_ctrl;

// ARP Table
class ARPTable;
extern ARPTable *arp_table;

// L2Ctrl
class EthCtrl;
extern EthCtrl *eth_ctrl;

class ARPCtrl;
extern ARPCtrl *arp_ctrl;

// L3Ctrl
class IPCtrl;
extern IPCtrl *ip_ctrl;

// L4Ctrl
class UDPCtrl;
class TCPCtrl;
extern UDPCtrl *udp_ctrl;
extern TCPCtrl *tcp_ctrl;

#else
#error "cannot include <net/global.h> unless __NETCTRL__ macro is defined"
#endif

#endif // __RAPH_KERNEL_NET_GLOBAL_H__
