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

#ifndef _FREEBSD_NET_IF_VAR_RAPH_H_
#define _FREEBSD_NET_IF_VAR_RAPH_H_

#include <dev/eth.h>
#include <net/if_var.h>
#include <sys/types-raph.h>
#include <sys/bus-raph.h>

// should be defined at ethernet.h
class BsdDevPciEthernet : public DevEthernet {
public:
  BsdDevPciEthernet(uint8_t bus, uint8_t device, uint8_t function) : DevEthernet(), _bsd_pci(bus, device, function) {
    _bsd.SetClass(&_bsd_pci);
  }
  virtual ~BsdDevPciEthernet() {}
  struct ifnet _ifp;
protected:
  BsdDevice _bsd;
private:
  BsdDevPci _bsd_pci;
};


#endif /* _FREEBSD_NET_IF_VAR_RAPH_H_ */
