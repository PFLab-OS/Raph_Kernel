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

#ifndef __RAPH_KERNEL_E1000_LEM_H__
#define __RAPH_KERNEL_E1000_LEM_H__

#include <stdint.h>
#include <mem/physmem.h>
#include <mem/virtmem.h>
#include <global.h>
#include <dev/pci.h>
#include <buf.h>
#include <dev/eth.h>
#include <freebsd/sys/param.h>
#include <freebsd/sys/types.h>
#include <freebsd/net/if_var-raph.h>

class lE1000 : public BsdDevPci {
public:
 lE1000(uint8_t bus, uint8_t device, uint8_t function) : BsdDevPci(bus, device, function) , _bsd_eth(*this) {}
  static DevPci *InitPci(uint8_t bus, uint8_t device, uint8_t function);

  class lE1000BsdEthernet : public BsdEthernet {
  public:
    lE1000BsdEthernet(lE1000 &master) : _master(master) {
    }
    
    static void PollingHandler(void *arg);
    
    virtual void UpdateLinkStatus() override;
    
    // allocate 6 byte before call
    virtual void GetEthAddr(uint8_t *buffer) override;
    virtual void ChangeHandleMethodToPolling() override;
    virtual void ChangeHandleMethodToInt() override;
    virtual void Transmit(void *) override;
    lE1000 &GetMasterClass() {
      return _master;
    }
  private:
    lE1000BsdEthernet();
    lE1000 &_master;
  };
  
  lE1000BsdEthernet &GetNetInterface() {
    return _bsd_eth;
  };
 private:
  virtual int DevMethodBusProbe() override final;
  virtual int DevMethodBusAttach() override final;
  virtual void DevMethodBusInit() override final;
  lE1000BsdEthernet _bsd_eth;
};

#endif /* __RAPH_KERNEL_E1000_LEM_H__ */
