/*
 *
 * Copyright (c) 2017 Raphine Project
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

#include <cpu.h>
#include <dev/netdev.h>
#include <dev/eth.h>
#include <tty.h>
#include <global.h>
#include <list.h>

uint64_t cnt = 0;
int64_t sum = 0;
static const int stime = 7000;
int time = stime, rtime = 0;

extern CpuId network_cpu;

void setup_arp_reply(NetDev *dev) {
  dev->SetReceiveCallback(network_cpu, make_uptr(new Function<NetDev *>([](NetDev *eth){
          NetDev::Packet *rpacket;
          if(!eth->ReceivePacket(rpacket)) {
            return;
          }
          uint32_t my_addr_int;
          assert(eth->GetIpv4Address(my_addr_int));
          uint8_t my_addr[4];
          my_addr[0] = (my_addr_int >> 0) & 0xff;
          my_addr[1] = (my_addr_int >> 8) & 0xff;
          my_addr[2] = (my_addr_int >> 16) & 0xff;
          my_addr[3] = (my_addr_int >> 24) & 0xff;
          // received packet
          if(rpacket->GetBuffer()[12] == 0x08 && rpacket->GetBuffer()[13] == 0x06 && rpacket->GetBuffer()[21] == 0x02) {
            uint64_t l = ((uint64_t)(timer->ReadMainCnt() - cnt) * (uint64_t)timer->GetCntClkPeriod()) / 1000;
            cnt = 0;
            sum += l;
            rtime++;
            // ARP packet
            char buf[40];
            memcpy(buf, rpacket->GetBuffer(),40);
            // gtty->Printf(
            //              "s", "ARP Reply received; ",
            //              "x", rpacket->GetBuffer()[22], "s", ":",
            //              "x", rpacket->GetBuffer()[23], "s", ":",
            //              "x", rpacket->GetBuffer()[24], "s", ":",
            //              "x", rpacket->GetBuffer()[25], "s", ":",
            //              "x", rpacket->GetBuffer()[26], "s", ":",
            //              "x", rpacket->GetBuffer()[27], "s", " is ",
            //              "d", rpacket->GetBuffer()[28], "s", ".",
            //              "d", rpacket->GetBuffer()[29], "s", ".",
            //              "d", rpacket->GetBuffer()[30], "s", ".",
            //              "d", rpacket->GetBuffer()[31], "s", " ",
            //              "s","latency:","d",l,"s","us\n");
            
          }
          if(rpacket->GetBuffer()[12] == 0x08 && rpacket->GetBuffer()[13] == 0x06 && rpacket->GetBuffer()[21] == 0x01 && (memcmp(rpacket->GetBuffer() + 38, my_addr, 4) == 0)) {
            // ARP packet
            uint8_t data[] = {
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Target MAC Address
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source MAC Address
              0x08, 0x06, // Type: ARP
              // ARP Packet
              0x00, 0x01, // HardwareType: Ethernet
              0x08, 0x00, // ProtocolType: IPv4
              0x06, // HardwareLength
              0x04, // ProtocolLength
              0x00, 0x02, // Operation: ARP Reply
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source Hardware Address
              0x00, 0x00, 0x00, 0x00, // Source Protocol Address
              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Target Hardware Address
              0x00, 0x00, 0x00, 0x00, // Target Protocol Address
            };
            memcpy(data, rpacket->GetBuffer() + 6, 6);
            static_cast<DevEthernet *>(eth)->GetEthAddr(data + 6);
            memcpy(data + 22, data + 6, 6);
            memcpy(data + 28, my_addr, 4);
            memcpy(data + 32, rpacket->GetBuffer() + 22, 6);
            memcpy(data + 38, rpacket->GetBuffer() + 28, 4);

            uint32_t len = sizeof(data)/sizeof(uint8_t);
            NetDev::Packet *tpacket;
            kassert(eth->GetTxPacket(tpacket));
            memcpy(tpacket->GetBuffer(), data, len);
            tpacket->len = len;
            eth->TransmitPacket(tpacket);
            // gtty->Printf(
            //              "s", "ARP Request received; ",
            //              "x", rpacket->GetBuffer()[22], "s", ":",
            //              "x", rpacket->GetBuffer()[23], "s", ":",
            //              "x", rpacket->GetBuffer()[24], "s", ":",
            //              "x", rpacket->GetBuffer()[25], "s", ":",
            //              "x", rpacket->GetBuffer()[26], "s", ":",
            //              "x", rpacket->GetBuffer()[27], "s", ",",
            //              "d", rpacket->GetBuffer()[28], "s", ".",
            //              "d", rpacket->GetBuffer()[29], "s", ".",
            //              "d", rpacket->GetBuffer()[30], "s", ".",
            //              "d", rpacket->GetBuffer()[31], "s", " says who's ",
            //              "d", rpacket->GetBuffer()[38], "s", ".",
            //              "d", rpacket->GetBuffer()[39], "s", ".",
            //              "d", rpacket->GetBuffer()[40], "s", ".",
            //              "d", rpacket->GetBuffer()[41], "s", "\n");
            // gtty->Printf("s", "[debug] info: Packet sent (length = ", "d", len, "s", ")\n");
          }
          eth->ReuseRxBuffer(rpacket);
        }, dev)));
}

void send_arp_packet(NetDev *dev, uint8_t *ipaddr) {
  {
    time = stime;
    rtime = 0;
    static uint8_t target_addr[4];
    memcpy(target_addr, ipaddr, 4);
    cnt = 0;
    sum = 0;
    auto callout_ = make_sptr(new Callout);
    callout_->Init(make_uptr(new Function2<wptr<Callout>, NetDev *>([](wptr<Callout> callout, NetDev *eth){
            if (!apic_ctrl->IsBootupAll()) {
              task_ctrl->RegisterCallout(make_sptr(callout), 1000);
              return;
            }
            eth->UpdateLinkStatus();
            if (eth->GetStatus() != NetDev::LinkStatus::kUp) {
              task_ctrl->RegisterCallout(make_sptr(callout), 1000);
              return;
            }
            if (cnt != 0) {
              task_ctrl->RegisterCallout(make_sptr(callout), 1000);
              return;
            }
            for(int k = 0; k < 1; k++) {
              if (time == 0) {
                break;
              }
              uint8_t data[] = {
                0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Target MAC Address
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source MAC Address
                0x08, 0x06, // Type: ARP
                // ARP Packet
                0x00, 0x01, // HardwareType: Ethernet
                0x08, 0x00, // ProtocolType: IPv4
                0x06, // HardwareLength
                0x04, // ProtocolLength
                0x00, 0x01, // Operation: ARP Request
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source Hardware Address
                0x00, 0x00, 0x00, 0x00, // Source Protocol Address
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Target Hardware Address
                // Target Protocol Address
                0x00, 0x00, 0x00, 0x00
              };
              static_cast<DevEthernet *>(eth)->GetEthAddr(data + 6);
              memcpy(data + 22, data + 6, 6);
              uint32_t my_addr;
              assert(eth->GetIpv4Address(my_addr));
              data[28] = (my_addr >> 0) & 0xff;
              data[29] = (my_addr >> 8) & 0xff;
              data[30] = (my_addr >> 16) & 0xff;
              data[31] = (my_addr >> 24) & 0xff;
              memcpy(data + 38, target_addr, 4);
              uint32_t len = sizeof(data)/sizeof(uint8_t);
              NetDev::Packet *tpacket;
              kassert(eth->GetTxPacket(tpacket));
              memcpy(tpacket->GetBuffer(), data, len);
              tpacket->len = len;
              cnt = timer->ReadMainCnt();
              eth->TransmitPacket(tpacket);
              // gtty->Printf("s", "[debug] info: Packet sent (length = ", "d", len, "s", ")\n");
              time--;
            }
            if (time != 0) {
              task_ctrl->RegisterCallout(make_sptr(callout), 1000);
            }
          }, make_wptr(callout_), dev)));
    if (callout_->IsRegistered()) {
      task_ctrl->CancelCallout(callout_);
    }
    task_ctrl->RegisterCallout(callout_, cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority), 1000);
  }

  {
    auto callout_ = make_sptr(new Callout);
    callout_->Init(make_uptr(new Function2<wptr<Callout>, NetDev *>([](wptr<Callout> callout, NetDev *eth){
            if (rtime > 0) {
              gtty->Cprintf("ARP Reply average latency:%dus [%d/%d]\n", sum / rtime, rtime, stime);
            } else {
              if (eth->GetStatus() == NetDev::LinkStatus::kUp) {
                gtty->Cprintf("Link is Up, but no ARP Reply\n");
              } else {
                gtty->Cprintf("Link is Down, please wait...\n");
              }
            }
            if (rtime != stime) {
              task_ctrl->RegisterCallout(make_sptr(callout), 1000*1000*3);
            }
          }, make_wptr(callout_), dev)));
    if (callout_->IsRegistered()) {
      task_ctrl->CancelCallout(callout_);
    }
    task_ctrl->RegisterCallout(callout_, cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority), 2000);
  }
}
