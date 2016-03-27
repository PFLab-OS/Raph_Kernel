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

#include "em.h"
#include "e1000_raph.h"
#include "e1000_hw.h"
#include "if_em.h"

int	em_probe(device_t);
int	em_attach(device_t);
void	em_init(struct adapter *);
int em_poll(if_t ifp);
void em_update_link_status(struct adapter *adapter);

extern bE1000 *eth;
bool E1000::InitPCI(uint16_t vid, uint16_t did, uint8_t bus, uint8_t device, bool mf) {
  E1000 *addr = reinterpret_cast<E1000 *>(virtmem_ctrl->Alloc(sizeof(E1000)));
  addr = new(addr) E1000(bus, device, mf);
  addr->bsd.parent = addr;
  addr->bsd.adapter = reinterpret_cast<struct adapter *>(virtmem_ctrl->AllocZ(sizeof(adapter)));
  if (em_probe(&addr->bsd) == BUS_PROBE_DEFAULT) {
    kassert(em_attach(&addr->bsd) == 0);
    em_init(addr->bsd.adapter);
    addr->RegisterPolling();
    eth = addr;
    return true;
  } else {
    virtmem_ctrl->Free(ptr2virtaddr(addr->bsd.adapter));
    virtmem_ctrl->Free(ptr2virtaddr(addr));
    return false;
  }  
}

void E1000::Handle() {
  em_poll(this->bsd.adapter->ifp);
}

void E1000::GetEthAddr(uint8_t *buffer) {
  memcpy(buffer, bsd.adapter->hw.mac.addr, 6);
}

void E1000::UpdateLinkStatus() {
  this->bsd.adapter->hw.mac.get_link_status = 1;
  em_update_link_status(this->bsd.adapter);
}
