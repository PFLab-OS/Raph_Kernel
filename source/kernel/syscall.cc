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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * Author: hikalium
 *
 */

#include <tty.h>
#include <mem/physmem.h>
#include <string.h>
#include <stdlib.h>
#include <x86.h>
#include <syscall.h>
#include <gdt.h>
#include <global.h>
#include <net/udp.h>
#include <dev/eth.h>
#include <net/usersocket.h>

extern CpuId network_cpu;
/*
void CapturePort() { UdpCtrl::GetCtrl().RegisterSocket(5621, &_dhcp_ctrl); }
*/
SystemCallCtrl SystemCallCtrl::_ctrl;
UserSocket *_socket;

extern "C" int64_t syscall_handler();
extern size_t syscall_handler_stack;

extern "C" int64_t syscall_handler_sub(SystemCallCtrl::Args *args, int index) {
  return SystemCallCtrl::Handler(args, index);
}

void SystemCallCtrl::Init() {
  new (&_ctrl) SystemCallCtrl();
  _socket = nullptr;

  // IA32_EFER.SCE = 1
  const uint64_t bit_efer_SCE = 0x01;
  uint64_t efer = rdmsr(kIA32EFER);
  efer |= bit_efer_SCE;
  wrmsr(kIA32EFER, efer);
  // set vLSTAR
  wrmsr(kIA32STAR, (static_cast<uint64_t>(KERNEL_CS) << 32) |
                       ((static_cast<uint64_t>(USER_DS) - 8) << 48));
  wrmsr(kIA32LSTAR, reinterpret_cast<uint64_t>(syscall_handler));
  wrmsr(kIA32KernelGsBase, KERNEL_DS);
  syscall_handler_stack =
      KernelStackCtrl::GetCtrl().AllocThreadStack(cpu_ctrl->GetCpuId());
}

struct in_addr {
  uint8_t s_addr[4];  // big endian
};

struct sockaddr_in {
  uint8_t sin_len;
  uint8_t sin_family;
  uint8_t sin_port[2];  // big endian
  in_addr sin_addr;
  uint8_t sin_zero[8];
};

int64_t SystemCallCtrl::Handler(Args *args, int index) {
  if (GetCtrl()._mode == Mode::kLocal) {
    switch (index) {
      case 2: {
        // // open
        // gtty->Printf("<%s>", args->arg1);
        // return 3;
        break;
      }
      case 16: {
        // ioctl
        if (args->arg1 == 1) {
          // stdout
          switch (args->arg2) {
            case TCGETS: {
              return 0;
            }
            case TIOCGWINSZ: {
              winsize *ws = reinterpret_cast<winsize *>(args->arg3);
              ws->ws_row = gtty->GetRow();
              ws->ws_col = gtty->GetColumn();
              ws->ws_xpixel = 0;
              ws->ws_ypixel = 0;
              return 0;
            }
            default: {
              gtty->DisablePrint();
              gtty->ErrPrintf("%x\n", args->arg2);
              kernel_panic("Sysctrl", "unknown argument(ioctrl)");
            }
          };
        } else {
          kernel_panic("Sysctrl", "unknown fd(ioctrl)");
        }
        break;
      }
      case 20:
        // writev
        {
          if (args->arg1 == 1 || args->arg1 == 2) {
            // stdout, stderr
            iovec *iv_array = reinterpret_cast<iovec *>(args->arg2);
            int rval = 0;
            for (int i = 0; i < args->arg3; i++) {
              for (unsigned int j = 0; j < iv_array[i].iov_len; j++) {
                gtty->Printf("%c",
                             reinterpret_cast<char *>(iv_array[i].iov_base)[j]);
                rval++;
              }
            }
            gtty->Flush();
            return rval;
          } else {
            gtty->DisablePrint();
            gtty->ErrPrintf("<%llx>", args->arg1);
            kernel_panic("Sysctrl", "unknown fd(writev)");
          }
          break;
        }
      case 41:
        // socket
        {
          const int PF_INET = 2;
          const int SOCK_DGRAM = 2;
          const int IPPROTO_UDP = 17;
          if (args->arg1 == PF_INET && args->arg2 == SOCK_DGRAM &&
              args->arg3 == IPPROTO_UDP) {
            if (_socket != nullptr) {
              // no more sockets.
              return -1;
            }
            _socket = new UserSocket();
            return 1;
          } else {
            gtty->DisablePrint();
            gtty->ErrPrintf("%d %d %d", args->arg1, args->arg2, args->arg3);
            kernel_panic("socket", "not impl");
          }
        }
        break;
      case 44:
        // sendto
        {
          int fd = args->arg1;
          const uint8_t *ubuf = const_cast<const uint8_t *>(
              reinterpret_cast<uint8_t *>(args->arg2));
          size_t size = args->arg3;
          unsigned flags = args->arg4;
          sockaddr_in *addr = reinterpret_cast<sockaddr_in *>(args->arg5);
          int *addr_len = reinterpret_cast<int *>(args->arg6);
          if (fd == 1 && _socket) {
            uint16_t port = (addr->sin_port[0] << 8) | addr->sin_port[1];
            UdpCtrl::GetCtrl().Send(&addr->sin_addr.s_addr, port, ubuf, size,
                                    _socket->GetBoundPort());
            gtty->Printf("sent!!!\n");
            return size;
          } else {
            gtty->DisablePrint();
            gtty->ErrPrintf("%d %d %d", args->arg1, args->arg2, args->arg3);
            kernel_panic("sendto", "not impl");
          }
        }
        break;
      case 45:
        // recvfrom
        {
          int fd = args->arg1;
          void *ubuf = reinterpret_cast<void *>(args->arg2);
          size_t size = args->arg3;
          unsigned flags = args->arg4;
          sockaddr_in *addr = reinterpret_cast<sockaddr_in *>(args->arg5);
          int *addr_len = reinterpret_cast<int *>(args->arg6);
          const int SOCK_DGRAM = 2;
          const int IPPROTO_UDP = 17;
          if (fd == 1) {
            uint8_t dst_ip_addr[4], src_ip_addr[4];
            uint16_t dst_port, src_port;
            for (int i = 0; i < 10; i++) {
              asm volatile("sti");
              int recv_size = _socket->ReceiveSync(
                  reinterpret_cast<uint8_t *>(ubuf), size, dst_ip_addr,
                  src_ip_addr, &dst_port, &src_port);
              asm volatile("cli");
              // int recv_size = 10;
              //
              gtty->Printf("Receive in syscall!!!\n");
              if (recv_size < 0) {
                gtty->Printf("Error...\n");
                return -1;
              } else {
                addr->sin_port[0] = ((src_port >> 8) & 0xff);
                addr->sin_port[1] = ((src_port >> 0) & 0xff);
                memcpy(addr->sin_addr.s_addr, src_ip_addr, 4);
                return recv_size;
              }
            }

            return -1;
          } else {
            gtty->DisablePrint();
            gtty->ErrPrintf("%d %d %d", args->arg1, args->arg2, args->arg3);
            kernel_panic("socket", "not impl");
          }
        }
        break;
      case 49:
        // bind
        {
          int fd = args->arg1;
          sockaddr_in *addr = reinterpret_cast<sockaddr_in *>(args->arg2);
          // arg3
          if (fd == 1) {
            /*
            gtty->DisablePrint();
            gtty->ErrPrintf("port: %d, ip: %d.%d.%d.%d",
                            (addr->sin_port[0] << 8) | addr->sin_port[1],
                            addr->sin_addr.s_addr[0], addr->sin_addr.s_addr[1],
                            addr->sin_addr.s_addr[2], addr->sin_addr.s_addr[3]);
            kernel_panic("bind", "not impl");
            */
            if (_socket == nullptr) {
              // socket not created.
              return -1;
            }
            uint16_t port = (addr->sin_port[0] << 8) | addr->sin_port[1];
            if (_socket->Bind(port)) {
              return -1;
            }
            return 0;  // 0: success
          } else {
            gtty->DisablePrint();
            gtty->ErrPrintf("%d %d %d", args->arg1, args->arg2, args->arg3);
            kernel_panic("bind", "not impl");
          }
        }
      case 63:
        // uname
        {
          static const int kNewUTSLen = 64;
          struct new_utsname {
            char sysname[kNewUTSLen + 1];
            char nodename[kNewUTSLen + 1];
            char release[kNewUTSLen + 1];
            char version[kNewUTSLen + 1];
            char machine[kNewUTSLen + 1];
            char domainname[kNewUTSLen + 1];
          };
          new_utsname tmp;
          strcpy(tmp.sysname, "");
          strcpy(tmp.nodename, "");
          strcpy(tmp.release, "");
          strcpy(tmp.version, "");
          strcpy(tmp.machine, "");
          strcpy(tmp.domainname, "");
          memcpy(reinterpret_cast<void *>(args->arg1), &tmp,
                 sizeof(new_utsname));
          return 0;
        }
      case 158: {
        gtty->Printf("sys_arch_prctl code=%llx addr=%llx\n", args->arg1,
                     args->arg2);
        switch (args->arg1) {
          case kArchSetGs:
            wrmsr(kIA32GsBase, args->arg2);
            break;
          case kArchSetFs:
            wrmsr(kIA32FsBase, args->arg2);
            break;
          case kArchGetFs:
            *reinterpret_cast<uint64_t *>(args->arg2) = rdmsr(kIA32FsBase);
            break;
          case kArchGetGs:
            *reinterpret_cast<uint64_t *>(args->arg2) = rdmsr(kIA32GsBase);
            break;
          default:
            gtty->DisablePrint();
            gtty->ErrPrintf("Invalid args\n", index);
            kassert(false);
        }
        return 0;
      }
      case 231: {
        // exit group
        gtty->Printf("user program called exit\n");
        gtty->Flush();
        while (true) {
          asm volatile("hlt;");
        }
      }
    }
  } else if (GetCtrl()._mode == Mode::kRemote) {
    switch (index) {
      case 16: {
        // ioctl
        if (args->arg1 == 1) {
          // stdout
          switch (args->arg2) {
            case TCGETS: {
              return 0;
            }
            case TIOCGWINSZ: {
              winsize *ws = reinterpret_cast<winsize *>(args->arg3);
              ws->ws_row = gtty->GetRow();
              ws->ws_col = gtty->GetColumn();
              ws->ws_xpixel = 0;
              ws->ws_ypixel = 0;
              return 0;
            }
            default: {
              gtty->DisablePrint();
              gtty->ErrPrintf("%x\n", args->arg2);
              kernel_panic("Sysctrl", "unknown argument(ioctrl)");
            }
          };
        } else {
          kernel_panic("Sysctrl", "unknown fd(ioctrl)");
        }
        break;
      }
      case 20:
        // writev
        {
          if (args->arg1 == 1) {
            // stdout
            iovec *iv_array = reinterpret_cast<iovec *>(args->arg2);
            int rval = 0;
            for (int i = 0; i < args->arg3; i++) {
              uint8_t target_addr[4] = {192, 168, 12, 17};
              UdpCtrl::GetCtrl().Send(
                  &target_addr, 12345,
                  const_cast<const uint8_t *>(
                      reinterpret_cast<uint8_t *>(iv_array[i].iov_base)),
                  iv_array[i].iov_len, 12345);  // TODO: Specify PORT
              rval += iv_array[i].iov_len;
            }
            return rval;
          } else {
            gtty->DisablePrint();
            gtty->ErrPrintf("<%llx>", args->arg1);
            kernel_panic("Sysctrl", "unknown fd(writev)");
          }
          break;
        }
      case 63:
        // uname
        {
          static const int kNewUTSLen = 64;
          struct new_utsname {
            char sysname[kNewUTSLen + 1];
            char nodename[kNewUTSLen + 1];
            char release[kNewUTSLen + 1];
            char version[kNewUTSLen + 1];
            char machine[kNewUTSLen + 1];
            char domainname[kNewUTSLen + 1];
          };
          new_utsname tmp;
          strcpy(tmp.sysname, "");
          strcpy(tmp.nodename, "");
          strcpy(tmp.release, "");
          strcpy(tmp.version, "");
          strcpy(tmp.machine, "");
          strcpy(tmp.domainname, "");
          memcpy(reinterpret_cast<void *>(args->arg1), &tmp,
                 sizeof(new_utsname));
          return 0;
        }
      case 158: {
        gtty->Printf("sys_arch_prctl code=%llx addr=%llx\n", args->arg1,
                     args->arg2);
        switch (args->arg1) {
          case kArchSetGs:
            wrmsr(kIA32GsBase, args->arg2);
            break;
          case kArchSetFs:
            wrmsr(kIA32FsBase, args->arg2);
            break;
          case kArchGetFs:
            *reinterpret_cast<uint64_t *>(args->arg2) = rdmsr(kIA32FsBase);
            break;
          case kArchGetGs:
            *reinterpret_cast<uint64_t *>(args->arg2) = rdmsr(kIA32GsBase);
            break;
          default:
            gtty->DisablePrint();
            gtty->ErrPrintf("Invalid args\n", index);
            kassert(false);
        }
        return 0;
      }
      case 231: {
        // exit group
        gtty->Printf("user program called exit\n");
        gtty->Flush();
        while (true) {
          asm volatile("hlt;");
        }
      }
    }
  }
  gtty->DisablePrint();
  gtty->ErrPrintf("syscall(%d) (return address %llx)\n", index, args->raddr);
  kernel_panic("Syscall", "unimplemented systemcall");
}
