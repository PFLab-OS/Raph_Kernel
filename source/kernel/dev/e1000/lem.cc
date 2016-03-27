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

#include "lem.h"
#include "e1000_raph.h"
#include "e1000_hw.h"
#include "if_lem.h"

int	lem_probe(device_t);
int	lem_attach(device_t);
void	lem_init(struct adapter *);
int lem_poll(if_t ifp);
void lem_update_link_status(struct adapter *adapter);

extern bE1000 *eth;
bool lE1000::InitPCI(uint16_t vid, uint16_t did, uint8_t bus, uint8_t device, bool mf) {
  lE1000 *addr = reinterpret_cast<lE1000 *>(virtmem_ctrl->Alloc(sizeof(lE1000)));
  addr = new(addr) lE1000(bus, device, mf);
  addr->bsd.parent = addr;
  addr->bsd.adapter = reinterpret_cast<struct adapter *>(virtmem_ctrl->AllocZ(sizeof(adapter)));

  if (lem_probe(&addr->bsd) == BUS_PROBE_DEFAULT) {
    kassert(lem_attach(&addr->bsd) == 0);
    lem_init(addr->bsd.adapter);
    addr->RegisterPolling();
    eth = addr;
    return true;
  } else {
    virtmem_ctrl->Free(ptr2virtaddr(addr->bsd.adapter));
    virtmem_ctrl->Free(ptr2virtaddr(addr));
    return false;
  }  
}

void lE1000::Handle() {
  lem_poll(this->bsd.adapter->ifp);
}

void lE1000::GetEthAddr(uint8_t *buffer) {
  memcpy(buffer, bsd.adapter->hw.mac.addr, 6);
}

void lE1000::UpdateLinkStatus() {
  this->bsd.adapter->hw.mac.get_link_status = 1;
  lem_update_link_status(this->bsd.adapter);
}
