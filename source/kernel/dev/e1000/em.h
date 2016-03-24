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

#ifndef __RAPH_KERNEL_E1000_EM_H__
#define __RAPH_KERNEL_E1000_EM_H__

#include <stdint.h>
#include <mem/physmem.h>
#include <mem/virtmem.h>
#include <polling.h>
#include <global.h>
#include <dev/pci.h>
#include "bem.h"

class E1000 : public bE1000, Polling {
public:
 E1000(uint8_t bus, uint8_t device, bool mf) : bE1000(bus, device, mf) {}
  static bool InitPCI(uint16_t vid, uint16_t did, uint8_t bus, uint8_t device, bool mf);
  // from Polling
  virtual void Handle() override;

  BsdDriver bsd;
  virtual void UpdateLinkStatus() override;

  // allocate 6 byte before call
  virtual void GetEthAddr(uint8_t *buffer) override;
 private:
};

#endif /* __RAPH_KERNEL_E1000_EM_H__ */
