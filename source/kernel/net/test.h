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

#ifndef __RAPH_KERNEL_NET_TEST_H__
#define __RAPH_KERNEL_NET_TEST_H__

#ifdef __UNIT_TEST__

#include <stdint.h>

// ARP request/reply test (1 turn around)
void ArpReply(uint32_t ipRequest, uint32_t ipReply);
void ArpRequest(uint32_t ipRequest, uint32_t ipReply);

/* 
 * [TEST#1] TCP connection test (client loopback)
 *
 * Enter something to client prompt, then it will be sent to server
 * and server will send back it to client.
 * While these communications, acknowledgement is done correctly.
 * By this test case, the following features can be checked.
 *
 *   * SYN-ACK 3-way handshake (connection establish)
 *   * acknowledgement
 *   * FIN-ACK 4-way handshake (connection closing)
 */
void TcpServer1();
void TcpClient1();

/*
 * [TEST#2] TCP packet segmentation test
 *
 * Client will send large packet (larger than MSS; maximum segment size),
 * so the packet will be segmented before transmission.
 * Through this test, it can be checked that both sender and receiver 
 * can handle segmented packets appropriately.
 */
void TcpServer2();
void TcpClient2();

/*
 * [TEST#3] TCP retransmission timeout test
 *
 * Client performs same as TEST#2. However, this server wait 5 secs,
 * which is larger than RTO (Retransmission TimeOut), before receiving.
 * Therefore client have to regard this as packet loss and retransmit.
 */
void TcpServer3();
void TcpClient3();

#endif // __UNIT_TEST__

#endif // __RAPH_KERNEL_NET_TEST_H__
