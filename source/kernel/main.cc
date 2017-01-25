/*
 *
 * Copyright (c) 2015 Raphine Project
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

#include <apic.h>
#include <cpu.h>
#include <gdt.h>
#include <global.h>
#include <idt.h>
#include <multiboot.h>
#include <mem/physmem.h>
#include <mem/paging.h>
#include <mem/virtmem.h>
#include <raph_acpi.h>
#include <task.h>
#include <timer.h>
#include <tty.h>
#include <shell.h>
#include <measure.h>
#include <mem/kstack.h>

#include <dev/hpet.h>
#include <dev/pci.h>
#include <dev/usb/usb.h>
#include <dev/vga.h>
#include <dev/pciid.h>
#include <dev/8042.h>

#include <dev/netdev.h>
#include <dev/eth.h>
// #include <net/arp.h>
#include <arpa/inet.h>

AcpiCtrl *acpi_ctrl = nullptr;
ApicCtrl *apic_ctrl = nullptr;
MultibootCtrl *multiboot_ctrl = nullptr;
PagingCtrl *paging_ctrl = nullptr;
PhysmemCtrl *physmem_ctrl = nullptr;
VirtmemCtrl *virtmem_ctrl = nullptr;
Gdt *gdt = nullptr;
Idt *idt = nullptr;
Shell *shell = nullptr;
PciCtrl *pci_ctrl = nullptr;
NetDevCtrl *netdev_ctrl = nullptr;
// ArpTable *arp_table = nullptr;

MultibootCtrl _multiboot_ctrl;
AcpiCtrl _acpi_ctrl;
ApicCtrl _apic_ctrl;
CpuCtrl _cpu_ctrl;
Gdt _gdt;
Idt _idt;
VirtmemCtrl _virtmem_ctrl;
PhysmemCtrl _physmem_ctrl;
PagingCtrl _paging_ctrl;
TaskCtrl _task_ctrl;
Hpet _htimer;
Vga _vga;
Shell _shell;
AcpicaPciCtrl _acpica_pci_ctrl;
NetDevCtrl _netdev_ctrl;
// ArpTable _arp_table;

CpuId network_cpu;
CpuId pstack_cpu;

static uint32_t rnd_next = 1;

#include <dev/disk/ahci/ahci-raph.h>
AhciChannel *g_channel = nullptr;
#include <dev/fs/fat/fat.h>
FatFs *fatfs;

#ifdef FLAG_MEMBENCH
static const bool do_membench = true;
#else
static const bool do_membench = false;
#endif

void register_membench2_callout();

void halt(int argc, const char* argv[]) {
  acpi_ctrl->Shutdown();
}

void reset(int argc, const char* argv[]) {
  acpi_ctrl->Reset();
}

void lspci(int argc, const char* argv[]) {
  MCFG *mcfg = acpi_ctrl->GetMCFG();
  if (mcfg == nullptr) {
    gtty->Cprintf("[Pci] error: could not find MCFG table.\n");
    return;
  }

  const char *search = nullptr;
  if (argc > 2) {
    gtty->Cprintf("[lspci] error: invalid argument\n");
    return;
  } else if (argc == 2) {
    search = argv[1];
  }

  PciData::Table table;
  table.Init();

  for (int i = 0; i * sizeof(MCFGSt) < mcfg->header.Length - sizeof(ACPISDTHeader); i++) {
    if (i == 1) {
      gtty->Cprintf("[Pci] info: multiple MCFG tables.\n");
      break;
    }
    for (int j = mcfg->list[i].pci_bus_start; j <= mcfg->list[i].pci_bus_end; j++) {
      for (int k = 0; k < 32; k++) {
        int maxf = ((pci_ctrl->ReadReg<uint16_t>(j, k, 0, PciCtrl::kHeaderTypeReg) & PciCtrl::kHeaderTypeRegFlagMultiFunction) != 0) ? 7 : 0;
        for (int l = 0; l <= maxf; l++) {
          if (pci_ctrl->ReadReg<uint16_t>(j, k, l, PciCtrl::kVendorIDReg) == 0xffff) {
            continue;
          }
          table.Search(j, k, l, search);
        }
      }
    }
  }
}

static bool parse_ipaddr(const char *c, uint8_t *addr) {
  int i = 0;
  while(true) {
    if (i != 3 && *c == '.') {
      i++;
    } else if (i == 3 && *c == '\0') {
      return true;
    } else if (*c >= '0' && *c <= '9') {
      addr[i] *= 10;
      addr[i] += *c - '0';
    } else {
      return false;
    }
    c++;
  }
}

static void setip(int argc, const char* argv[]) {
  if (argc != 3) {
    gtty->Cprintf("invalid argument\n");
    return;
  }
  NetDevCtrl::NetDevInfo *info = netdev_ctrl->GetDeviceInfo(argv[1]);
  if (info == nullptr) {
    gtty->Cprintf("no ethernet interface(%s).\n", argv[1]);
    return;
  }
  NetDev *dev = info->device;

  uint8_t addr[4] = {0, 0, 0, 0};
  if (!parse_ipaddr(argv[2], addr)) {
    gtty->Cprintf("invalid ip v4 addr.\n");
    return;
  }

  dev->AssignIpv4Address((addr[3] << 24) | (addr[2] << 16) | (addr[1] << 8) | addr[0]);
}

void ifconfig(int argc, const char* argv[]){
  uptr<Array<const char *>> list = netdev_ctrl->GetNamesOfAllDevices();
  gtty->CprintfRaw("\n");
  for (size_t l = 0; l < list->GetLen(); l++) {
    gtty->CprintfRaw("%s", (*list)[l]);
    NetDev *dev = netdev_ctrl->GetDeviceInfo((*list)[l])->device;
    dev->UpdateLinkStatus();
    gtty->CprintfRaw("  link: %s\n", dev->IsLinkUp() ? "up" : "down");
  }
}

void setup_arp_reply(NetDev *dev);
void send_arp_packet(NetDev *dev, uint8_t *ipaddr);

void bench(int argc, const char* argv[]) {

  if (argc == 1) {
    gtty->Cprintf("invalid argument.\n");
    return;
  }
  if (strcmp(argv[1], "snd") == 0) {
    if (argc == 2) {
      gtty->Cprintf("specify ethernet interface.\n");
      return;
    } else if (argc == 3) {
      gtty->Cprintf("specify ip v4 addr.\n");
      return;
    } else if (argc != 4) {
      gtty->Cprintf("invalid arguments\n");
      return;
    }
    NetDevCtrl::NetDevInfo *info = netdev_ctrl->GetDeviceInfo(argv[2]);
    if (info == nullptr) {
      gtty->Cprintf("no ethernet interface(%s).\n", argv[2]);
      return;
    }
    NetDev *dev = info->device;

    uint8_t addr[4] = {0, 0, 0, 0};
    if (!parse_ipaddr(argv[3], addr)) {
      gtty->Cprintf("invalid ip v4 addr.\n");
      return;
    }

    send_arp_packet(dev, addr);
  } else if (strcmp(argv[1], "set_reply") == 0) {
    if (argc == 2) {
      gtty->Cprintf("specify ethernet interface.\n");
      return;
    }
    NetDevCtrl::NetDevInfo *info = netdev_ctrl->GetDeviceInfo(argv[2]);
    if (info == nullptr) {
      gtty->Cprintf("no ethernet interface(%s).\n", argv[2]);
      return;
    }
    setup_arp_reply(info->device);
  } /* else if (strcmp(argv[1], "qemu") == 0) {
    uint8_t ip1_[] = {10, 0, 2, 9};
    uint8_t ip2_[] = {10, 0, 2, 15};
    memcpy(ip1, ip1_, 4);
    memcpy(ip2, ip2_, 4);
    sdevice = (argc == 2) ? "eth0" : argv[2];
    rdevice = (argc == 2) ? "eth0" : argv[2];
    if (!netdev_ctrl->Exists(rdevice)) {
      gtty->Cprintf("no ethernet interface(%s).\n", rdevice);
      return;
    }
    } */ else {
    gtty->Cprintf("invalid argument.\n");
    return;
  }
}

static void setflag(int argc, const char *argv[]) {
  if (argc == 1) {
    gtty->Cprintf("invalid argument.\n");
    return;
  }
  if (strcmp(argv[1], "spinlock_timeout") == 0) {
    if (argc == 2) {
      gtty->Cprintf("invalid argument.\n");
      return;
    }
    if (strcmp(argv[2], "true") == 0) {
      SpinLock::_spinlock_timeout = true;
    } else if (strcmp(argv[2], "false") == 0) {
      SpinLock::_spinlock_timeout = false;
    } else {
      gtty->Cprintf("invalid argument.\n");
      return;
    }
  } else {
    gtty->Cprintf("invalid argument.\n");
    return;
  }
}

class ArpTable {
public:
  void Set(uint32_t ip_addr, uint8_t *hw_addr, NetDev *dev) {
    auto iter = arp_table.GetBegin();
    while(!iter.IsNull()) {
      if ((*iter)->ip_addr == ip_addr) {
        memcpy((*iter)->hw_addr, hw_addr, 6);
        (*iter)->dev = dev;
        return;
      }
      iter = iter->GetNext();
    }
    arp_table.PushBack(ip_addr, hw_addr, dev);
  }
  NetDev *Search(uint32_t ip_addr, uint8_t *hw_addr) {
    auto iter = arp_table.GetBegin();
    while(!iter.IsNull()) {
      if ((*iter)->ip_addr == ip_addr) {
        memcpy(hw_addr, (*iter)->hw_addr, 6);
        return (*iter)->dev;
      }
      iter = iter->GetNext();
    }
    return nullptr;
  }
private:
  struct ArpEntry {
    ArpEntry(uint32_t ip_addr_, uint8_t *hw_addr_, NetDev *dev_) {
      ip_addr = ip_addr_;
      memcpy(hw_addr, hw_addr_, 6);
      dev = dev_;
    }
    uint32_t ip_addr;
    uint8_t hw_addr[6];
    NetDev *dev;
  };
  List<ArpEntry> arp_table;
} *arp_table;

static void arp_scan(int argc, const char *argv[]) {
  auto devices = netdev_ctrl->GetNamesOfAllDevices();
  for (size_t i = 0; i < devices->GetLen(); i++) {
    auto dev = netdev_ctrl->GetDeviceInfo((*devices)[i])->device;
    if (dev->GetStatus() != NetDev::LinkStatus::kUp) {
      gtty->Cprintf("skip %s (Link Down)\n", (*devices)[i]);
      continue;
    }
    uint32_t my_addr_int_;
    if (!dev->GetIpv4Address(my_addr_int_) || my_addr_int_ == 0) {
      gtty->Cprintf("skip %s (no IP)\n", (*devices)[i]);
      continue;
    }
    gtty->Cprintf("ARP scan with %s\n", (*devices)[i]);
    dev->SetReceiveCallback(network_cpu, make_uptr(new Function<NetDev *>([](NetDev *eth) {
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
              // ARP Reply
              uint32_t target_addr_int = (rpacket->GetBuffer()[31] << 24) | (rpacket->GetBuffer()[30] << 16) | (rpacket->GetBuffer()[29] << 8) | rpacket->GetBuffer()[28];
              arp_table->Set(target_addr_int, rpacket->GetBuffer() + 22, eth);
            }
            if(rpacket->GetBuffer()[12] == 0x08 && rpacket->GetBuffer()[13] == 0x06 && rpacket->GetBuffer()[21] == 0x01 && (memcmp(rpacket->GetBuffer() + 38, my_addr, 4) == 0)) {
              // ARP Request
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
            }
            eth->ReuseRxBuffer(rpacket);
          }, dev)));
    for (int j = 1; j < 255; j++) {
      uint32_t my_addr_int;
      assert(dev->GetIpv4Address(my_addr_int));
      auto callout_ = make_sptr(new Callout);
      struct Container {
        wptr<Callout> callout;
        NetDev *eth;
        uint8_t target_addr[4];
      };
      auto container_ = make_uptr(new Container);
      container_->callout = make_wptr(callout_);
      container_->eth = dev;
      container_->target_addr[0] = (my_addr_int >> 0) & 0xff;
      container_->target_addr[1] = (my_addr_int >> 8) & 0xff;
      container_->target_addr[2] = (my_addr_int >> 16) & 0xff;
      container_->target_addr[3] = j;
      callout_->Init(make_uptr(new Function<uptr<Container>>([](uptr<Container> container){
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
              static_cast<DevEthernet *>(container->eth)->GetEthAddr(data + 6);
              memcpy(data + 22, data + 6, 6);
              uint32_t my_addr;
              assert(container->eth->GetIpv4Address(my_addr));
              data[28] = (my_addr >> 0) & 0xff;
              data[29] = (my_addr >> 8) & 0xff;
              data[30] = (my_addr >> 16) & 0xff;
              data[31] = (my_addr >> 24) & 0xff;
              memcpy(data + 38, container->target_addr, 4);
              uint32_t len = sizeof(data)/sizeof(uint8_t);
              NetDev::Packet *tpacket;
              kassert(container->eth->GetTxPacket(tpacket));
              memcpy(tpacket->GetBuffer(), data, len);
              tpacket->len = len;
              container->eth->TransmitPacket(tpacket);
            }, container_)));
      if (callout_->IsRegistered()) {
        task_ctrl->CancelCallout(callout_);
      }
      task_ctrl->RegisterCallout(callout_, cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority), 1000);
    }
  }
  auto callout_ = make_sptr(new Callout);
  callout_->Init(make_uptr(new Function<void *>([](void *){
          auto devices_ = netdev_ctrl->GetNamesOfAllDevices();
          for (size_t i = 0; i < devices_->GetLen(); i++) {
            auto dev = netdev_ctrl->GetDeviceInfo((*devices_)[i])->device;
            dev->SetReceiveCallback(network_cpu, make_uptr(new GenericFunction<>()));
          }
        }, nullptr)));
  task_ctrl->RegisterCallout(callout_, cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority), 3*1000*1000);
}

static void udpsend(int argc, const char *argv[]) {
  if (argc != 4) {
    gtty->Cprintf("invalid argument.\n");
    return;
  }

  uint8_t target_addr[4] = {0, 0, 0, 0};
  if (!parse_ipaddr(argv[1], target_addr)) {
    gtty->Cprintf("invalid ip v4 addr.\n");
    return;
  }
  uint32_t target_addr_int = (target_addr[3] << 24) | (target_addr[2] << 16) | (target_addr[1] << 8) | target_addr[0];
  
  uint8_t target_mac[6];
  NetDev *dev = arp_table->Search(target_addr_int, target_mac);
  if (dev == nullptr) {
    gtty->Cprintf("cannot solve mac address from ARP Table.\n");
    return;
  }

  uint32_t my_addr_int;
  assert(dev->GetIpv4Address(my_addr_int));
  uint8_t my_addr[4];
  my_addr[0] = (my_addr_int >> 0) & 0xff;
  my_addr[1] = (my_addr_int >> 8) & 0xff;
  my_addr[2] = (my_addr_int >> 16) & 0xff;
  my_addr[3] = (my_addr_int >> 24) & 0xff;
  
  
  uint8_t buf[1518];
  int offset = 0;

  //
  // ethernet
  //

  // target MAC address
  memcpy(buf + offset, target_mac, 6);
  offset += 6;

  // source MAC address
  static_cast<DevEthernet *>(dev)->GetEthAddr(buf + offset);
  offset += 6;

  // type: IPv4
  uint8_t type[2] = {0x08, 0x00};
  memcpy(buf + offset, type, 2);
  offset += 2;

  //
  // IPv4
  //

  int ipv4_header_start = offset;

  // version & header length
  buf[offset] = (0x4 << 4) | 0x5;
  offset += 1;

  // service type
  buf[offset] = 0;
  offset += 1;

  // skip
  int datagram_length_offset = offset;
  offset += 2;

  // ID field
  buf[offset] = rand() & 0xff;
  buf[offset + 1] = rand() & 0xff;
  offset += 2;

  // flag & flagment offset
  uint16_t foffset = 0 | (1 << 15);
  buf[offset] = foffset >> 8;
  buf[offset + 1] = foffset;
  offset += 2;

  // ttl
  buf[offset] = 0xff;
  offset += 1;

  // procol number;
  buf[offset] = 17;
  offset += 1;

  // skip
  int checksum_offset = offset;
  buf[offset] = 0;
  buf[offset + 1] = 0;
  offset += 2;

  // source address
  memcpy(buf + offset, my_addr, 4);
  offset += 4;

  // target address
  memcpy(buf + offset, target_addr, 4);
  offset += 4;

  int ipv4_header_end = offset;

  //
  // udp
  //

  int udp_header_start = offset;

  // source port
  uint8_t source_port[] = {0x4, 0xD2}; // 1234
  memcpy(buf + offset, source_port, 2);
  offset += 2;

  // target port
  // TODO analyze from argument
  uint8_t target_port[] = {0x4, 0xD2}; // 1234
  memcpy(buf + offset, target_port, 2);
  offset += 2;

  // skip
  int udp_length_offset = offset;
  offset += 2;

  // skip
  int udp_checksum_offset = offset;
  buf[offset] = 0;
  buf[offset + 1] = 0;
  offset += 2;

  // data
  memcpy(buf + offset, argv[3], strlen(argv[3]) + 1);
  offset += strlen(argv[3]) + 1;
  
  // length
  size_t udp_length = offset - udp_header_start;
  buf[udp_length_offset] = udp_length >> 8;
  buf[udp_length_offset + 1] = udp_length;

  // checksum
  {
    uint32_t checksum = 0;
    uint8_t pseudo_header[] = {
      0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0x00, 17, 0x00, 0x00,
    };
    memcpy(pseudo_header + 0, my_addr, 4);
    memcpy(pseudo_header + 4, target_addr, 4);
    pseudo_header[10] = udp_length >> 8;
    pseudo_header[11] = udp_length;
    for (int i = 0; i < 12; i += 2) {
      checksum += (pseudo_header[i] << 8) + pseudo_header[i + 1];
    }

    if ((offset - udp_header_start) % 2 == 1) {
      buf[offset] = 0x0;
    }
    for (int i = udp_header_start; i < offset; i += 2) {
      checksum += (buf[i] << 8) + buf[i + 1];
    }

    while(checksum > 0xffff) {
      checksum = (checksum >> 16) + (checksum & 0xffff);
    }
    checksum = ~checksum;
  
    buf[udp_checksum_offset] = checksum >> 8;
    buf[udp_checksum_offset + 1] = checksum;
  }

  // datagram length(IPv4)
  size_t len = offset - ipv4_header_start;
  buf[datagram_length_offset] = len >> 8;
  buf[datagram_length_offset + 1] = len;

  // checksum
  {
    uint32_t checksum = 0;
    for (int i = ipv4_header_start; i < ipv4_header_end; i += 2) {
      checksum += (buf[i] << 8) + buf[i + 1];
    }

    while(checksum > 0xffff) {
      checksum = (checksum >> 16) + (checksum & 0xffff);
    }
    checksum = ~checksum;

    buf[checksum_offset] = checksum >> 8;
    buf[checksum_offset + 1] = checksum;
  }
  
  assert(offset < 1518);

  NetDev::Packet *tpacket;
  kassert(dev->GetTxPacket(tpacket));
  memcpy(tpacket->GetBuffer(), buf, offset);
  tpacket->len = offset;
  
  dev->TransmitPacket(tpacket);
}
  
static void show(int argc, const char *argv[]) {
  if (argc == 1) {
    gtty->Cprintf("invalid argument.\n");
    return;
  }
  if (strcmp(argv[1], "module") == 0) {
    if (argc != 2) {
      gtty->Cprintf("invalid argument.\n");
      return;
    }
    multiboot_ctrl->ShowModuleInfo();
  } else if (strcmp(argv[1], "info") == 0) {
    gtty->Cprintf("[kernel] info: Hardware Information\n");
    gtty->Cprintf("available cpu thread num: %d\n", cpu_ctrl->GetHowManyCpus());
    gtty->Cprintf("\n");
    gtty->Cprintf("[kernel] info: Build Information\n");
    multiboot_ctrl->ShowBuildTimeStamp();
  } else {
    gtty->Cprintf("invalid argument.\n");
    return;
  }
}

struct LoadContainer {
  LoadContainer() = delete;
  LoadContainer(uptr<Array<uint8_t>> data_) : data(data_) {
  }
  uptr<Array<uint8_t>> data;
  int i = 0;
};

static void load_script(sptr<LoadContainer> container_) {
  auto callout_ = make_sptr(new Callout);
  callout_->Init(make_uptr(new Function2<wptr<Callout>, sptr<LoadContainer>>([](wptr<Callout> callout, sptr<LoadContainer> container){
          size_t i = container->i;
          while(container->i < container->data->GetLen()) {
            if ((*container->data)[container->i] == '\n') {
              (*container->data)[container->i] = '\0';
              auto ec = make_uptr(new Shell::ExecContainer(shell));
              ec = shell->Tokenize(ec, reinterpret_cast<char *>(container->data->GetRawPtr()) + i);
              container->i++;
              if (strcmp(ec->argv[0], "wait") != 0) {
                shell->Execute(ec);
              } else {
                gtty->Cprintf("> wait %s\n", ec->argv[1]);
                if (ec->argc == 2) {
                  int t = 0;
                  for(size_t l = 0; l < strlen(ec->argv[1]); l++) {
                    if ('0' > ec->argv[1][l] || ec->argv[1][l] > '9') {
                      gtty->Cprintf("invalid argument.\n");
                      t = 0;
                      break;
                    }
                    t = t * 10 + ec->argv[1][l] - '0';
                  }
                  task_ctrl->RegisterCallout(make_sptr(callout), t * 1000 * 1000);
                  return;
                } else {
                  gtty->Cprintf("invalid argument.\n");
                }
              }
              task_ctrl->RegisterCallout(make_sptr(callout), 10);
              return;
            }
            container->i++;
          }
        }, make_wptr(callout_), container_)));
  task_ctrl->RegisterCallout(callout_, 10);
}

static void load(int argc, const char *argv[]) {
  if (argc != 2) {
    gtty->Cprintf("invalid argument.\n");
    return;
  }
  if (strcmp(argv[1], "script.sh") == 0) {
    load_script(make_sptr(new LoadContainer(multiboot_ctrl->LoadFile(argv[1]))));
  } else {
    gtty->Cprintf("invalid argument.\n");
    return;
  }
}

static void beep(int argc, const char *argv[]) {
  CpuId beep_cpuid = cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority);
  auto callout_ = make_sptr(new Callout);
  callout_->Init(make_uptr(new Function<wptr<Callout>>([](wptr<Callout> callout) {
          static int i = 0;
          if(i < 6) {
            uint16_t sound[6] = {905, 761, 452, 570, 508, 380};
            uint8_t a = 0xb6;
            outb(0x43, a);
            uint8_t l = sound[i] & 0x00FF;
            outb(0x42, l);
            uint8_t h = (sound[i] >> 8) & 0x00FF;
            outb(0x42, h);
            uint8_t on = inb(0x61);
            outb(0x61, (on | 0x03) & 0x0f);
            i++;
            task_ctrl->RegisterCallout(make_sptr(callout), 110000);
          } else {
            uint8_t off = inb(0x61);
            outb(0x61, off & 0xd);
          }
        }, make_wptr(callout_))));
  task_ctrl->RegisterCallout(callout_, beep_cpuid, 1);
}

void freebsd_main();

extern "C" int main() {

  multiboot_ctrl = new (&_multiboot_ctrl) MultibootCtrl;

  acpi_ctrl = new (&_acpi_ctrl) AcpiCtrl;

  apic_ctrl = new (&_apic_ctrl) ApicCtrl;

  cpu_ctrl = new (&_cpu_ctrl) CpuCtrl;

  gdt = new (&_gdt) Gdt;

  idt = new (&_idt) Idt;

  virtmem_ctrl = new (&_virtmem_ctrl) VirtmemCtrl;

  physmem_ctrl = new (&_physmem_ctrl) PhysmemCtrl;

  paging_ctrl = new (&_paging_ctrl) PagingCtrl;

  task_ctrl = new (&_task_ctrl) TaskCtrl;

  timer = new (&_htimer) Hpet;

  gtty = new (&_vga) Vga;

  shell = new (&_shell) Shell;

  netdev_ctrl = new (&_netdev_ctrl) NetDevCtrl();

  // arp_table = new (&_arp_table) ArpTable();

  physmem_ctrl->Init();

  multiboot_ctrl->Setup();

  paging_ctrl->MapAllPhysMemory();

  KernelStackCtrl::Init();

  apic_ctrl->Init();

  acpi_ctrl->Setup();

  if (timer->Setup()) {
    gtty->Cprintf("[timer] info: HPET supported.\n");
  } else {
    kernel_panic("timer", "HPET not supported.\n");
  }

  apic_ctrl->Setup();

  cpu_ctrl->Init();
  
  network_cpu = cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kHighPerformance);

  pstack_cpu = cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kHighPerformance);

  rnd_next = timer->ReadMainCnt();

  task_ctrl->Setup();

  idt->SetupGeneric();

  apic_ctrl->BootBSP();

  gdt->SetupProc();

  idt->SetupProc();
  
  pci_ctrl = new (&_acpica_pci_ctrl) AcpicaPciCtrl;

  acpi_ctrl->SetupAcpica();

  UsbCtrl::Init();

  freebsd_main();

  InitDevices<PciCtrl, LegacyKeyboard, Device>();

  // 各コアは最低限の初期化ののち、TaskCtrlに制御が移さなければならない
  // 特定のコアで専用の処理をさせたい場合は、TaskCtrlに登録したジョブとして
  // 実行する事

  apic_ctrl->StartAPs();

  gtty->Init();
  
  // arp_table->Setup();

  while (!apic_ctrl->IsBootupAll()) {
  }
  gtty->Cprintf("\n\n[kernel] info: initialization completed\n");

  arp_table = new ArpTable;
  
  shell->Setup();
  shell->Register("halt", halt);
  shell->Register("reset", reset);
  shell->Register("bench", bench);
  shell->Register("beep", beep);
  shell->Register("lspci", lspci);
  shell->Register("ifconfig", ifconfig);
  shell->Register("setip", setip);
  shell->Register("show", show);
  shell->Register("load", load);
  shell->Register("setflag", setflag);
  shell->Register("udpsend", udpsend);
  shell->Register("arp_scan", arp_scan);

  if (do_membench) {
    register_membench2_callout();
  }

  load_script(make_sptr(new LoadContainer(multiboot_ctrl->LoadFile("init.sh"))));

  task_ctrl->Run();

  return 0;
}

extern "C" int main_of_others() {
  // according to mp spec B.3, system should switch over to Symmetric I/O mode
  
  apic_ctrl->BootAP();

  gdt->SetupProc();
  idt->SetupProc();

  // ループ性能測定用
  //#define LOOP_BENCHMARK
#ifdef LOOP_BENCHMARK
#define LOOP_BENCHMARK_CPU  4
  PollingFunc p;
  if (cpu_ctrl->GetCpuId().GetRawId() == LOOP_BENCHMARK_CPU) {
    static int hoge = 0;
    p.Init(make_uptr(new Function<void *>([](void *){
            int hoge2 = timer->GetUsecFromCnt(timer->ReadMainCnt()) - hoge;
            gtty->Cprintf("%d ", hoge2);
            hoge = timer->GetUsecFromCnt(timer->ReadMainCnt());
          }, nullptr)));
    p.Register();
  }
#endif
  
// ワンショット性能測定用
// #define ONE_SHOT_BENCHMARK
#ifdef ONE_SHOT_BENCHMARK
#define ONE_SHOT_BENCHMARK_CPU  5
  if (cpu_ctrl->GetCpuId().GetRawId() == ONE_SHOT_BENCHMARK_CPU) {
    auto callout_ = make_sptr(new Callout);
    callout_->Init(make_uptr(new Function<wptr<Callout>>([](wptr<Callout> callout){
            if (!apic_ctrl->IsBootupAll()) {
              task_ctrl->RegisterCallout(make_sptr(callout), 1000);
              return;
            }
            // kassert(g_channel != nullptr);
            // FatFs *fatfs = new FatFs();
            // kassert(fatfs->Mount());
            //        g_channel->Read(0, 1);
          }, make_wptr(callout_))));
    task_ctrl->RegisterCallout(callout_, 10);
  }
#endif
  if (do_membench) {
    register_membench2_callout();
  }

  task_ctrl->Run();
  return 0;
}

static int error_output_flag = 0;

void show_backtrace(size_t *rbp) {
    for (int i = 0; i < 3; i++) {
      gtty->CprintfRaw("backtrace(%d): rip:%llx,\n", i, rbp[1]);
      rbp = reinterpret_cast<size_t *>(rbp[0]);
    }
}

extern "C" void _kernel_panic(const char *class_name, const char *err_str) {
  if (gtty != nullptr) {
    while(!__sync_bool_compare_and_swap(&error_output_flag, 0, 1)) {
    }
    gtty->CprintfRaw("\n!!!! Kernel Panic !!!!\n");
    gtty->CprintfRaw("[%s] error: %s\n",class_name, err_str);
    gtty->CprintfRaw("\n"); 
    gtty->CprintfRaw(">> debugging information >>\n");
    gtty->CprintfRaw("cpuid: %d\n", cpu_ctrl->GetCpuId().GetRawId());
    size_t *rbp;
    asm volatile("movq %%rbp, %0":"=r"(rbp));
    show_backtrace(rbp);
    __sync_bool_compare_and_swap(&error_output_flag, 1, 0);
  }
  while(true) {
    asm volatile("cli;hlt;");
  }
}

extern "C" void checkpoint(int id, const char *str) {
  if (id < 0 || cpu_ctrl->GetCpuId().GetRawId() == id) {
    gtty->CprintfRaw(str);
  }
}

extern "C" void _checkpoint(const char *func, const int line) {
  gtty->CprintfRaw("[%s:%d]", func, line);
}

extern "C" void abort() {
  if (gtty != nullptr) {
    while(!__sync_bool_compare_and_swap(&error_output_flag, 0, 1)) {
    }
    gtty->CprintfRaw("system stopped by unexpected error.\n");
    size_t *rbp;
    asm volatile("movq %%rbp, %0":"=r"(rbp));
    show_backtrace(rbp);
    __sync_bool_compare_and_swap(&error_output_flag, 1, 0);
  }
  while(true){
    asm volatile("cli;hlt");
  }
}

extern "C" void _kassert(const char *file, int line, const char *func) {
  if (gtty != nullptr) {
    while(!__sync_bool_compare_and_swap(&error_output_flag, 0, 1)) {
    }
    gtty->CprintfRaw("assertion failed at %s l.%d (%s) cpuid: %d\n",
                     file, line, func, cpu_ctrl->GetCpuId().GetRawId());
    size_t *rbp;
    asm volatile("movq %%rbp, %0":"=r"(rbp));
    show_backtrace(rbp);
    __sync_bool_compare_and_swap(&error_output_flag, 1, 0);
  }
  while(true){
    asm volatile("cli;hlt");
  }
}

extern "C" void __cxa_pure_virtual() {
  kernel_panic("", "");
}

extern "C" void __stack_chk_fail() {
  kernel_panic("", "");
}

#define RAND_MAX 0x7fff

extern "C" uint32_t rand() {
  rnd_next = rnd_next * 1103515245 + 12345;
  /* return (unsigned int)(rnd_next / 65536) % 32768;*/
  return (uint32_t)(rnd_next >> 16) & RAND_MAX;
}
