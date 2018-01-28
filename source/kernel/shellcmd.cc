#include <apic.h>
#include <cpu.h>
#include <gdt.h>
#include <global.h>
#include <idt.h>
#include <multiboot.h>
#include <mem/physmem.h>
#include <mem/virtmem.h>
#include <raph_acpi.h>
#include <thread.h>
#include <timer.h>
#include <tty.h>
#include <shell.h>
#include <measure.h>
#include <mem/kstack.h>
#include <elf.h>
#include <syscall.h>

#include <dev/hpet.h>
#include <dev/pci.h>
#include <dev/usb/usb.h>
#include <dev/framebuffer.h>
#include <dev/pciid.h>
#include <dev/8042.h>
#include <dev/storage/storage.h>
#include <dev/storage/ramdisk.h>

#include <fs.h>
#include <v6fs.h>

#include <dev/netdev.h>
#include <dev/eth.h>
#include <net/arp.h>
#include <net/udp.h>
#include <net/dhcp.h>

//
// misc func
//

extern CpuId network_cpu;

void setup_arp_reply(NetDev *dev);
void send_arp_packet(NetDev *dev, uint8_t *ipaddr);

void register_membench2_callout();

static bool parse_ipaddr(const char *c, uint8_t *addr) {
  int i = 0;
  addr[0] = 0x00;
  addr[1] = 0x00;
  addr[2] = 0x00;
  addr[3] = 0x00;
  while (true) {
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
static void PrintEthernetAddr(uint8_t *eth_addr) {
  gtty->Printf("%02X:%02X:%02X:%02X:%02X:%02X", eth_addr[0], eth_addr[1],
               eth_addr[2], eth_addr[3], eth_addr[4], eth_addr[5]);
}

//
// shell commands
//

static void halt(int argc, const char *argv[]) { acpi_ctrl->Shutdown(); }

static void reset(int argc, const char *argv[]) { acpi_ctrl->Reset(); }

static void lspci(int argc, const char *argv[]) {
  MCFG *mcfg = acpi_ctrl->GetMCFG();
  if (mcfg == nullptr) {
    gtty->Printf("[Pci] error: could not find MCFG table.\n");
    return;
  }

  const char *search = nullptr;
  if (argc > 2) {
    gtty->Printf("[lspci] error: invalid argument\n");
    return;
  } else if (argc == 2) {
    search = argv[1];
  }

  PciData::Table table;
  table.Init();

  for (int i = 0;
       i * sizeof(MCFGSt) < mcfg->header.Length - sizeof(ACPISDTHeader); i++) {
    if (i == 1) {
      gtty->Printf("[Pci] info: multiple MCFG tables.\n");
      break;
    }
    for (int j = mcfg->list[i].pci_bus_start; j <= mcfg->list[i].pci_bus_end;
         j++) {
      for (int k = 0; k < 32; k++) {
        int maxf =
            ((pci_ctrl->ReadPciReg<uint16_t>(j, k, 0, PciCtrl::kHeaderTypeReg) &
              PciCtrl::kHeaderTypeRegFlagMultiFunction) != 0)
                ? 7
                : 0;
        for (int l = 0; l <= maxf; l++) {
          if (pci_ctrl->ReadPciReg<uint16_t>(j, k, l, PciCtrl::kVendorIDReg) ==
              0xffff) {
            continue;
          }
          table.Search(j, k, l, search);
        }
      }
    }
  }
}

static void setip(int argc, const char *argv[]) {
  if (argc < 3) {
    gtty->Printf("invalid argument\n");
    return;
  }
  NetDevCtrl::NetDevInfo *info = netdev_ctrl->GetDeviceInfo(argv[1]);
  if (info == nullptr) {
    gtty->Printf("no ethernet interface(%s).\n", argv[1]);
    return;
  }
  NetDev *dev = info->device;

  uint8_t addr[4] = {0, 0, 0, 0};
  if (!strcmp(argv[2], "static")) {
    if (argc != 4) {
      gtty->Printf("invalid argument\n");
      return;
    }
    if (!parse_ipaddr(argv[3], addr)) {
      gtty->Printf("invalid ip v4 addr.\n");
      return;
    }
    dev->AssignIpv4Address(*(reinterpret_cast<uint32_t *>(addr)));
  } else if (!strcmp(argv[2], "dhcp")) {
    if (argc != 3) {
      gtty->Printf("invalid argument\n");
      return;
    }

    DhcpCtrl::GetCtrl().AssignAddr(dev);
  } else {
    gtty->Printf("invalid argument\n");
    return;
  }
}

static void ifconfig(int argc, const char *argv[]) {
  uptr<Array<const char *>> list = netdev_ctrl->GetNamesOfAllDevices();
  gtty->Printf("\n");
  for (size_t l = 0; l < list->GetLen(); l++) {
    gtty->Printf("%s\n", (*list)[l]);
    NetDev *dev = netdev_ctrl->GetDeviceInfo((*list)[l])->device;
    dev->UpdateLinkStatus();
    uint8_t eth_addr[6];
    static_cast<DevEthernet *>(dev)->GetEthAddr(eth_addr);
    gtty->Printf("  ether: %02X:%02X:%02X:%02X:%02X:%02X\n", eth_addr[0],
                 eth_addr[1], eth_addr[2], eth_addr[3], eth_addr[4],
                 eth_addr[5]);
    gtty->Printf("  link: %s\n", dev->IsLinkUp() ? "up" : "down");
  }
}

static void bench(int argc, const char *argv[]) {
  if (argc == 1) {
    gtty->Printf("invalid argument.\n");
    return;
  }
  if (strcmp(argv[1], "snd") == 0) {
    if (argc == 2) {
      gtty->Printf("specify ethernet interface.\n");
      return;
    } else if (argc == 3) {
      gtty->Printf("specify ip v4 addr.\n");
      return;
    } else if (argc != 4) {
      gtty->Printf("invalid arguments\n");
      return;
    }
    NetDevCtrl::NetDevInfo *info = netdev_ctrl->GetDeviceInfo(argv[2]);
    if (info == nullptr) {
      gtty->Printf("no ethernet interface(%s).\n", argv[2]);
      return;
    }
    NetDev *dev = info->device;

    uint8_t addr[4] = {0, 0, 0, 0};
    if (!parse_ipaddr(argv[3], addr)) {
      gtty->Printf("invalid ip v4 addr.\n");
      return;
    }

    send_arp_packet(dev, addr);
  } else if (strcmp(argv[1], "set_reply") == 0) {
    if (argc == 2) {
      gtty->Printf("specify ethernet interface.\n");
      return;
    }
    NetDevCtrl::NetDevInfo *info = netdev_ctrl->GetDeviceInfo(argv[2]);
    if (info == nullptr) {
      gtty->Printf("no ethernet interface(%s).\n", argv[2]);
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
      gtty->Printf("no ethernet interface(%s).\n", rdevice);
      return;
    }
    } */
  else {
    gtty->Printf("invalid argument.\n");
    return;
  }
}

static void setflag(int argc, const char *argv[]) {
  if (argc == 1) {
    gtty->Printf("invalid argument.\n");
    return;
  }
  if (strcmp(argv[1], "spinlock_timeout") == 0) {
    if (argc == 2) {
      gtty->Printf("invalid argument.\n");
      return;
    }
    if (strcmp(argv[2], "true") == 0) {
      SpinLock::_spinlock_timeout = true;
    } else if (strcmp(argv[2], "false") == 0) {
      SpinLock::_spinlock_timeout = false;
    } else {
      gtty->Printf("invalid argument.\n");
      return;
    }
  } else {
    gtty->Printf("invalid argument.\n");
    return;
  }
}

static void udp_setup(int argc, const char *argv[]) {
  UdpCtrl::GetCtrl().SetupServer();
}

static void arp_scan_on_device(const char *dev_name, uint32_t base_ipv4_addr,
                               int range) {
  auto dev = netdev_ctrl->GetDeviceInfo(dev_name)->device;
  if (dev->GetStatus() != NetDev::LinkStatus::kUp) {
    gtty->Printf("skip %s (Link Down)\n", dev_name);
    return;
  }
  union {
    uint8_t bytes[4];
    uint32_t uint32;
  } dev_ipv4_addr;
  if (!dev->GetIpv4Address(dev_ipv4_addr.uint32) ||
      dev_ipv4_addr.uint32 == 0xFFFFFFFF) {
    gtty->Printf("skip %s (no IP)\n", dev_name);
    return;
  }

  dev->SetReceiveCallback(
      network_cpu,
      make_uptr(new Function<NetDev *>(
          [](NetDev *eth) {
            NetDev::Packet *rpacket;
            if (!eth->ReceivePacket(rpacket)) {
              return;
            }
            uint32_t my_addr_int_;
            assert(eth->GetIpv4Address(my_addr_int_));
            uint8_t my_addr[4];
            *(reinterpret_cast<uint32_t *>(my_addr)) = my_addr_int_;
            // received packet
            gtty->Printf("Some reply got\n");
            if (rpacket->GetBuffer()[12] == 0x08 &&
                rpacket->GetBuffer()[13] == 0x06 &&
                rpacket->GetBuffer()[21] == 0x02) {
              // ARP Reply
              union {
                uint8_t bytes[4];
                uint32_t uint32;
              } responder_ipv4_addr;
              memcpy(responder_ipv4_addr.bytes, &rpacket->GetBuffer()[28], 4);
              gtty->Printf(
                  "ARP reply from %d.%d.%d.%d\n", responder_ipv4_addr.bytes[0],
                  responder_ipv4_addr.bytes[1], responder_ipv4_addr.bytes[2],
                  responder_ipv4_addr.bytes[3]);
              arp_table->Set(responder_ipv4_addr.uint32,
                             rpacket->GetBuffer() + 22, eth);
            }
            if (rpacket->GetBuffer()[12] == 0x08 &&
                rpacket->GetBuffer()[13] == 0x06 &&
                rpacket->GetBuffer()[21] == 0x01 &&
                (memcmp(rpacket->GetBuffer() + 38, my_addr, 4) == 0)) {
              // ARP Request
              uint8_t data[] = {
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Target MAC Address
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Source MAC Address
                  0x08, 0x06,                          // Type: ARP
                  // ARP Packet
                  0x00, 0x01,  // HardwareType: Ethernet
                  0x08, 0x00,  // ProtocolType: IPv4
                  0x06,        // HardwareLength
                  0x04,        // ProtocolLength
                  0x00, 0x02,  // Operation: ARP Reply
                  0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00,                    // Source Hardware Address
                  0x00, 0x00, 0x00, 0x00,  // Source Protocol Address
                  0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00,                    // Target Hardware Address
                  0x00, 0x00, 0x00, 0x00,  // Target Protocol Address
              };
              memcpy(data, rpacket->GetBuffer() + 6, 6);
              static_cast<DevEthernet *>(eth)->GetEthAddr(data + 6);
              memcpy(data + 22, data + 6, 6);
              memcpy(data + 28, my_addr, 4);
              memcpy(data + 32, rpacket->GetBuffer() + 22, 6);
              memcpy(data + 38, rpacket->GetBuffer() + 28, 4);

              uint32_t len = sizeof(data) / sizeof(uint8_t);
              NetDev::Packet *tpacket;
              kassert(eth->GetTxPacket(tpacket));
              memcpy(tpacket->GetBuffer(), data, len);
              tpacket->len = len;
              eth->TransmitPacket(tpacket);
            }
            eth->ReuseRxBuffer(rpacket);
          },
          dev)));

  gtty->Printf("ARP scan with %s(%d.%d.%d.%d)\n", dev_name,
               dev_ipv4_addr.bytes[0], dev_ipv4_addr.bytes[1],
               dev_ipv4_addr.bytes[2], dev_ipv4_addr.bytes[3]);
  for (int i = 0; i < (1 << range); i++) {
    uptr<Thread> thread = ThreadCtrl::GetCtrl(cpu_ctrl->RetainCpuIdForPurpose(
                                                  CpuPurpose::kLowPriority))
                              .AllocNewThread(Thread::StackState::kShared);
    do {
      auto t_op = thread->CreateOperator();
      struct Container {
        NetDev *eth;
        union {
          uint8_t bytes[4];
          uint32_t uint32;
        } target_ipv4_addr;
      };
      auto container_ = make_uptr(new Container);
      container_->eth = dev;
      container_->target_ipv4_addr.uint32 = base_ipv4_addr;
      //(base_ipv4_addr & ~((1 << range) - 1)) + i;
      gtty->Printf("Target ip: %d.%d.%d.%d\n",
                   container_->target_ipv4_addr.bytes[0],
                   container_->target_ipv4_addr.bytes[1],
                   container_->target_ipv4_addr.bytes[2],
                   container_->target_ipv4_addr.bytes[3]);

      t_op.SetFunc(make_uptr(new Function<uptr<Container>>(
          [](uptr<Container> container) {
            uint8_t data[] = {0xff, 0xff, 0xff, 0xff, 0xff,
                              0xff,  // +0x00: Target MAC Address
                              0x00, 0x00, 0x00, 0x00, 0x00,
                              0x00,        // +0x06: Source MAC Address
                              0x08, 0x06,  // +0x0c: Type: ARP
                              // ARP Packet
                              0x00, 0x01,  // +0x0e: HardwareType: Ethernet
                              0x08, 0x00,  // *0x10: ProtocolType: IPv4
                              0x06,        // +0x12: HardwareLength
                              0x04,        // +0x13: ProtocolLength
                              0x00, 0x01,  // +0x14: Operation: ARP Request
                              0x00, 0x00, 0x00, 0x00, 0x00,  // +0x16
                              0x00,  // Source Hardware Address
                              0x00, 0x00, 0x00,
                              0x00,  // +0x1c: Source Protocol Address
                              0x00, 0x00, 0x00, 0x00, 0x00,
                              0x00,  // Target Hardware Address
                              // Target Protocol Address
                              0x00, 0x00, 0x00, 0x00};
            static_cast<DevEthernet *>(container->eth)->GetEthAddr(data + 6);
            memcpy(data + 22, data + 6, 6);
            uint32_t my_addr;
            assert(container->eth->GetIpv4Address(my_addr));
            *(reinterpret_cast<uint32_t *>(data + 0x1c)) = my_addr;
            memcpy(data + 38, container->target_ipv4_addr.bytes, 4);
            uint32_t len = sizeof(data) / sizeof(uint8_t);
            NetDev::Packet *tpacket;
            kassert(container->eth->GetTxPacket(tpacket));
            memcpy(tpacket->GetBuffer(), data, len);
            tpacket->len = len;
            container->eth->TransmitPacket(tpacket);
            gtty->Printf("send ARP Req to (%d.%d.%d.%d)\n",
                         container->target_ipv4_addr.bytes[0],
                         container->target_ipv4_addr.bytes[1],
                         container->target_ipv4_addr.bytes[2],
                         container->target_ipv4_addr.bytes[3]);
            gtty->Printf("target eth: ");
            PrintEthernetAddr(data + 0);
            gtty->Printf("\n");

          },
          container_)));
      t_op.Schedule();
    } while (0);
    thread->Join();
  }
}

static void arp_scan(int argc, const char *argv[]) {
  union {
    uint8_t bytes[4];
    uint32_t uint32;
  } target_addr;
  if (argc < 2 || !parse_ipaddr(argv[1], target_addr.bytes)) {
    gtty->Printf("invalid ip v4 addr.\n");
    return;
  }
  gtty->Printf("specified ip: %X %d %d %d %d\n", target_addr.uint32,
               target_addr.bytes[0], target_addr.bytes[1], target_addr.bytes[2],
               target_addr.bytes[3]);
  auto devices = netdev_ctrl->GetNamesOfAllDevices();
  for (size_t i = 0; i < devices->GetLen(); i++) {
    arp_scan_on_device((*devices)[i], target_addr.uint32, 0);
  }
  uptr<Thread> thread = ThreadCtrl::GetCtrl(cpu_ctrl->RetainCpuIdForPurpose(
                                                CpuPurpose::kLowPriority))
                            .AllocNewThread(Thread::StackState::kShared);
  do {
    auto t_op = thread->CreateOperator();
    t_op.SetFunc(make_uptr(new Function<void *>(
        [](void *) {
          auto devices_ = netdev_ctrl->GetNamesOfAllDevices();
          for (size_t i = 0; i < devices_->GetLen(); i++) {
            auto dev = netdev_ctrl->GetDeviceInfo((*devices_)[i])->device;
            dev->SetReceiveCallback(network_cpu,
                                    make_uptr(new GenericFunction<>()));
          }
        },
        nullptr)));
    t_op.Schedule(3 * 1000 * 1000);
  } while (0);
  thread->Join();
}

void udpsend(int argc, const char *argv[]) {
  if (argc != 4) {
    gtty->Printf("invalid argument.\n");
    return;
  }

  uint8_t target_addr[4] = {0, 0, 0, 0};
  if (!parse_ipaddr(argv[1], target_addr)) {
    gtty->Printf("invalid ip v4 addr.\n");
    return;
  }
  UdpCtrl::GetCtrl().SendStr(&target_addr, atoi(argv[2]), argv[3]);
}

static void show(int argc, const char *argv[]) {
  if (argc == 1) {
    gtty->Printf("invalid argument.\n");
    return;
  }
  if (strcmp(argv[1], "module") == 0) {
    if (argc != 2) {
      gtty->Printf("invalid argument.\n");
      return;
    }
    multiboot_ctrl->ShowModuleInfo();
  } else if (strcmp(argv[1], "info") == 0) {
    gtty->Printf("[kernel] info: Hardware Information\n");
    gtty->Printf("available cpu thread num: %d\n", cpu_ctrl->GetHowManyCpus());
    gtty->Printf("\n");
    gtty->Printf("[kernel] info: Build Information\n");
    multiboot_ctrl->ShowBuildTimeStamp();
  } else {
    gtty->Printf("invalid argument.\n");
    return;
  }
}

static void wait_until_linkup(int argc, const char *argv[]) {
  if (argc != 2) {
    gtty->Printf("invalid argument.\n");
    return;
  }
  NetDevCtrl::NetDevInfo *info = netdev_ctrl->GetDeviceInfo(argv[1]);
  if (info == nullptr) {
    gtty->Printf("no such device.\n");
    return;
  }
  NetDev *dev_ = info->device;
  uptr<Thread> thread = ThreadCtrl::GetCtrl(cpu_ctrl->RetainCpuIdForPurpose(
                                                CpuPurpose::kLowPriority))
                            .AllocNewThread(Thread::StackState::kShared);
  do {
    Thread::Operator t_op = thread->CreateOperator();
    t_op.SetFunc(make_uptr(new Function<NetDev *>(
        [](NetDev *dev) {
          if (!dev->IsLinkUp()) {
            ThreadCtrl::GetCurrentThreadOperator().Schedule(1000 * 1000);
          }
        },
        dev_)));
    t_op.Schedule();
  } while (0);
  thread->Join();
}

void load_script(uptr<Array<uint8_t>> data) {
  // TODO: Should it move to Shell?
  size_t old_index = 0;
  size_t index = 0;
  while (index <= data->GetLen()) {
    if (index == data->GetLen() || (*data)[index] == '\r' ||
        (*data)[index] == '\n' || (*data)[index] == '\0') {
      if (old_index != index) {
        char buffer[index - old_index + 1];
        memcpy(buffer, reinterpret_cast<char *>(data->GetRawPtr()) + old_index,
               index - old_index);
        buffer[index - old_index] = '\0';
        auto ec = make_uptr(new Shell::ExecContainer(shell));
        ec = shell->Tokenize(ec, buffer);
        int timeout = 0;
        if (ec->argc != 0) {
          gtty->Printf("> %s\n", buffer);
          if (strcmp(ec->argv[0], "wait") == 0) {
            if (ec->argc == 2) {
              int t = 0;
              for (size_t l = 0; l < strlen(ec->argv[1]); l++) {
                if ('0' > ec->argv[1][l] || ec->argv[1][l] > '9') {
                  gtty->Printf("invalid argument.\n");
                  t = 0;
                  break;
                }
                t = t * 10 + ec->argv[1][l] - '0';
              }
              timeout = t * 1000 * 1000;
            } else {
              gtty->Printf("invalid argument.\n");
            }
          } else if (strcmp(ec->argv[0], "wait_until_linkup") == 0) {
            wait_until_linkup(ec->argc, ec->argv);
          } else {
            shell->Execute(ec);
          }
        }
        if (timeout != 0) {
          ThreadCtrl::GetCurrentThreadOperator().Schedule(timeout);
          ThreadCtrl::WaitCurrentThread();
        }
      }
      if (index == data->GetLen() - 1 || index == data->GetLen() ||
          (*data)[index] == '\0') {
        break;
      }
      old_index = index + 1;
    }
    index++;
  }
}

static void load(int argc, const char *argv[]) {
  if (argc != 2) {
    gtty->Printf("invalid argument.\n");
    return;
  }
  if (strcmp(argv[1], "script.sh") == 0) {
    load_script(multiboot_ctrl->LoadFile(argv[1]));
  } else if (strcmp(argv[1], "test.elf") == 0) {
    auto buf_ = multiboot_ctrl->LoadFile(argv[1]);
    uptr<Thread> thread = ThreadCtrl::GetCtrl(cpu_ctrl->RetainCpuIdForPurpose(
                                                  CpuPurpose::kLowPriority))
                              .AllocNewThread(Thread::StackState::kIndependent);
    do {
      auto t_op = thread->CreateOperator();
      t_op.SetFunc(make_uptr(new Function<uptr<Array<uint8_t>>>(
          [](uptr<Array<uint8_t>> buf) {
            Loader loader;
            ElfObject obj(loader, buf->GetRawPtr());
            if (obj.Init() != BinObjectInterface::ErrorState::kOk) {
              gtty->Printf("error while loading app\n");
            } else {
              obj.Execute();
            }
          },
          buf_)));
      t_op.Schedule();
    } while (0);
    thread->Join();
  } else if (strcmp(argv[1], "rump.bin") == 0) {
    auto buf_ = multiboot_ctrl->LoadFile(argv[1]);
    uptr<Thread> thread = ThreadCtrl::GetCtrl(cpu_ctrl->RetainCpuIdForPurpose(
                                                  CpuPurpose::kLowPriority))
                              .AllocNewThread(Thread::StackState::kIndependent);
    do {
      auto t_op = thread->CreateOperator();
      t_op.SetFunc(make_uptr(new Function<uptr<Array<uint8_t>>>(
          [](uptr<Array<uint8_t>> buf) {
            Ring0Loader loader;
            RaphineRing0AppObject obj(loader, buf->GetRawPtr());
            if (obj.Init() != BinObjectInterface::ErrorState::kOk) {
              gtty->Printf("error while loading app\n");
            } else {
              obj.Execute();
            }
          },
          buf_)));
      t_op.Schedule();
    } while (0);
    thread->Join();
  } else {
    gtty->Printf("invalid argument.\n");
    return;
  }
}

static void remote_load(int argc, const char *argv[]) {
  if (argc != 2) {
    gtty->Printf("invalid argument.\n");
    return;
  }
  if (strcmp(argv[1], "test.elf") == 0) {
    SystemCallCtrl::GetCtrl().SetMode(SystemCallCtrl::Mode::kRemote);
    auto buf_ = multiboot_ctrl->LoadFile(argv[1]);
    uptr<Thread> thread = ThreadCtrl::GetCtrl(cpu_ctrl->RetainCpuIdForPurpose(
                                                  CpuPurpose::kLowPriority))
                              .AllocNewThread(Thread::StackState::kShared);
    do {
      auto t_op = thread->CreateOperator();
      t_op.SetFunc(make_uptr(new Function<uptr<Array<uint8_t>>>(
          [](uptr<Array<uint8_t>> buf) {
            Loader loader;
            ElfObject obj(loader, buf->GetRawPtr());
            if (obj.Init() != BinObjectInterface::ErrorState::kOk) {
              gtty->Printf("error while loading app\n");
            } else {
              obj.Execute();
            }
          },
          buf_)));
      t_op.Schedule();
    } while (0);
    thread->Join();
  } else {
    gtty->Printf("invalid argument.\n");
    return;
  }
}

void beep(int argc, const char *argv[]) {
  uptr<Thread> thread = ThreadCtrl::GetCtrl(cpu_ctrl->RetainCpuIdForPurpose(
                                                CpuPurpose::kLowPriority))
                            .AllocNewThread(Thread::StackState::kShared);
  do {
    auto t_op = thread->CreateOperator();
    t_op.SetFunc(make_uptr(new Function<void *>(
        [](void *) {
          static int i = 0;
          if (i < 6) {
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
            ThreadCtrl::GetCurrentThreadOperator().Schedule(110000);
          } else {
            uint8_t off = inb(0x61);
            outb(0x61, off & 0xd);
          }
        },
        nullptr)));
    t_op.Schedule();
  } while (0);
  thread->Join();
}

static void membench(int argc, const char *argv[]) {
  register_membench2_callout();
}

bool cat_sub(VirtualFileSystem *vfs, const char *path) {
  InodeContainer inode;
  if (vfs->LookupInodeFromPath(inode, path, false) != IoReturnState::kOk)
    return false;
  VirtualFileSystem::Stat st;
  if (inode.GetStatOfInode(st) != IoReturnState::kOk) return false;
  size_t s = st.size;
  uint8_t buf[s];
  if (inode.ReadData(buf, 0, s) != IoReturnState::kOk) return false;
  for (size_t i = 0; i < s; i++) {
    gtty->Printf("%c", buf[i]);
  }
  gtty->Printf("\n");
  return true;
}

void cat(int argc, const char *argv[]) {
  if (argc < 2) {
    gtty->Printf("usage: cat <filename>\n");
    return;
  }
  Storage *storage;
  if (StorageCtrl::GetCtrl().GetDevice("ram0", storage) != IoReturnState::kOk) {
    kernel_panic("storage", "not found");
  }
  V6FileSystem *v6fs = new V6FileSystem(*storage);
  VirtualFileSystem *vfs = new VirtualFileSystem(v6fs);
  vfs->Init();
  if (!cat_sub(vfs, argv[1])) {
    gtty->Printf("cat: %s: No such file or directory\n", argv[1]);
  }
}

void RegisterDefaultShellCommands() {
  shell->Register("halt", halt);
  shell->Register("reset", reset);
  shell->Register("lspci", lspci);
  shell->Register("setip", setip);
  shell->Register("ifconfig", ifconfig);
  shell->Register("bench", bench);
  shell->Register("setflag", setflag);
  shell->Register("udp_setup", udp_setup);
  shell->Register("arp_scan", arp_scan);
  shell->Register("udpsend", udpsend);
  shell->Register("show", show);
  shell->Register("load", load);
  shell->Register("remote_load", remote_load);
  shell->Register("beep", beep);
  shell->Register("membench", membench);
  shell->Register("cat", cat);
}
