/*-
 * Copyright (c) 1982, 1986, 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)types.h	8.6 (Berkeley) 2/19/95
 * $FreeBSD$
 */

#ifndef _FREEBSD_SYS_TYPES_H_
#define _FREEBSD_SYS_TYPES_H_

#include <stdint.h>
#include <raph.h>
#include <dev/pci.h>
#include <dev/eth.h>
#include <freebsd/net/if_var.h>

typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned int	u_int;
typedef	unsigned long	u_long;

class BsdDevPci : public DevPci {
public:
  BsdDevPci(uint8_t bus, uint8_t device, bool mf) : DevPci(bus, device, mf) {
    for (int i = 0; i < kIntMax; i++) {
      map[i].handler = nullptr;
    }
  }
  virtual ~BsdDevPci() {
  }
  // 返り値は割り当てられたvector または-1(error)
  int SetMsi(int cpuid, ioint_callback handler, void *arg) {
    int i;
    {
      Locker locker(_lock);
      for (i = 0; i < kIntMax; i++) {
        if (map[i].handler == nullptr) {
          break;
        }
      }
      if (i == kIntMax) {
        return -1;
      }
    }
    int vector = DevPci::SetMsi(cpuid, HandleSub, reinterpret_cast<void *>(this));
    if (vector == -1) {
      return -1;
    }
    map[i].cpuid = cpuid;
    map[i].vector = vector;
    map[i].handler = handler;
    map[i].arg = arg;
    return vector;
  }
private:
  static void HandleSub(Regs *rs, void *arg) {
    BsdDevPci *that = reinterpret_cast<BsdDevPci *>(arg);
    Locker locker(that->_lock);
    for (int i = 0; i < kIntMax; i++) {
      if (static_cast<unsigned int>(that->map[i].vector) == rs->n && that->map[i].cpuid == apic_ctrl->GetCpuId()) {
        that->map[i].handler(that->map[i].arg);
        break;
      }
    }
  }
  static const int kIntMax = 10;
  struct IntMap {
    int cpuid;
    int vector;
    ioint_callback handler;
    void *arg;
  } map[kIntMax];
  SpinLock _lock;
};

class BsdDevice {
 public:
  template<class T>
    void SetMasterClass(T *master) {
    _master = reinterpret_cast<void *>(master);
  }
  template<class T>
    T *GetMasterClass() {
    return reinterpret_cast<T *>(_master);
  }
  void SetClass(BsdDevPci *pci) {
    _pci = pci;
  }
  BsdDevPci *GetPciClass() {
    kassert(_pci != nullptr);
    return _pci;
  }
  struct adapter *adapter;
 private:
  BsdDevPci *_pci = nullptr;
  void *_master;
};

class BsdDevEthernet : public DevEthernet {
 public:
  BsdDevEthernet(uint8_t bus, uint8_t device, bool mf) : DevEthernet(_bsd_pci), _bsd_pci(virtmem_ctrl->New<BsdDevPci>(bus, device, mf)) {
  }
  virtual ~BsdDevEthernet() {}
  BsdDevPci *GetBsdDevPci() {
    return _bsd_pci;
  }
  struct ifnet _ifp;
 protected:
  BsdDevPci *_bsd_pci;
  BsdDevice _bsd;
};

typedef struct BsdDevice *device_t;


#endif /* _FREEBSD_SYS_TYPES_H_ */
