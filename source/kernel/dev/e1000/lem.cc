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
void	lem_start(if_t);
int lem_poll(if_t ifp);

extern lE1000 *eth;
bool lE1000::InitPCI(uint16_t vid, uint16_t did, uint8_t bus, uint8_t device, bool mf) {
  lE1000 *addr = reinterpret_cast<lE1000 *>(virtmem_ctrl->Alloc(sizeof(lE1000)));
  addr = new(addr) lE1000(bus, device, mf);
  addr->bsd.parent = addr;
  addr->bsd.adapter = reinterpret_cast<struct adapter *>(virtmem_ctrl->Alloc(sizeof(adapter)));
  new(&addr->bsd.adapter->timer.callout) Callout;
  new(&addr->bsd.adapter->tx_fifo_timer.callout) Callout;
  new(&addr->bsd.adapter->core_mtx.lock) SpinLock;
  new(&addr->bsd.adapter->tx_mtx.lock) SpinLock;
  new(&addr->bsd.adapter->rx_mtx.lock) SpinLock;
  if (lem_probe(&addr->bsd) == BUS_PROBE_DEFAULT) {
    kassert(lem_attach(&addr->bsd) == 0);
    lem_init(addr->bsd.adapter);
    polling_ctrl->Register(addr);
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
  lem_start(this->bsd.adapter->ifp);
}

void lE1000::GetEthAddr(uint8_t *buffer) {
  memcpy(buffer, bsd.adapter->hw.mac.addr, 6);
}
