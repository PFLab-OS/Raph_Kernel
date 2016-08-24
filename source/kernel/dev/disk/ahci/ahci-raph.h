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
 * Author: Liva
 * 
 */

#ifndef __RAPH_KERNEL_DEV_DISK_AHCI_AHCI_RAPH_H__
#define __RAPH_KERNEL_DEV_DISK_AHCI_AHCI_RAPH_H__

#include <sys/types-raph.h>
#include <sys/bus-raph.h>

class AhciCtrl : public BsdDevPci {
public:
  AhciCtrl(uint8_t bus, uint8_t device, uint8_t function) : BsdDevPci(bus, device, function) {
  }
  static DevPci *InitPci(uint8_t bus, uint8_t device, uint8_t function);
private:
  virtual int DevMethodBusProbe() override final;
  virtual int DevMethodBusAttach() override final;
};

class AhciChannel : public BsdDevBus {
public:
  AhciChannel(AhciCtrl *ctrl) : BsdDevBus(ctrl, "ahcich", -1), _ctrl(ctrl) {
  }
  static AhciChannel *Init(AhciCtrl *ctrl);
private:
  AhciChannel();
  virtual int DevMethodBusProbe() override final;
  virtual int DevMethodBusAttach() override final;
  virtual int DevMethodBusSetupIntr(struct resource *r, int flags, driver_filter_t *filter, driver_intr_t *ithread, void *arg, void **cookiep) override final;
  virtual struct resource *DevMethodBusAllocResource(int type, int *rid, rman_res_t start, rman_res_t end, rman_res_t count, u_int flags) override final;
  virtual int DevMethodBusReleaseResource(int type, int rid, struct resource *r) override final;
  AhciCtrl *_ctrl;
};

#endif // __RAPH_KERNEL_DEV_DISK_AHCI_AHCI_RAPH_H__
