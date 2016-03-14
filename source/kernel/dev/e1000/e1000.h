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
 * Author: Levelfour
 *
 * 16/01/15: created by Levelfour
 * 16/02/28: add Ich8 support by Liva
 * 
 */

#ifndef __RAPH_KERNEL_E1000_H__
#define __RAPH_KERNEL_E1000_H__

#include <stdint.h>
#include "../../mem/physmem.h"
#include "../../mem/virtmem.h"
#include "../../polling.h"
#include "../../global.h"
#include "../pci.h"

struct adapter;

/*
 * e1000 is the driver for Intel 8254x/8256x/8257x and so on.
 * This implementation mainly takes care of 8257x.
 * These chipsets is loaded on Intel PRO/1000 series NIC,
 * and provide the way to receive/transmit packets.
 *
 * Main reference is PCIe* GbE Controllers Open Source Software Developer’s Manual.
 *   URL: http://www.intel.com/content/www/us/en/embedded/products/networking/pcie-gbe-controllers-open-source-manual.html
 * "pcie-gbe-controllers" in the source codes refers to this reference.
 * (if there's no notations, this means the reference of i8257x, not i8254x)
 */

/*
 * Receiver Descriptor (legacy type)
 *   data structure that contains the receiver data buffer address
 *   and fields for hardware to store packet information
 *   see pcie-gbe-controllers 3.2.4
 */
struct E1000RxDesc {
  // buffer address
  phys_addr bufAddr;
  // buffer length
  uint16_t length;
  // check sum
  uint16_t checkSum;
  // see Table 3-2
  uint8_t  status;
  // see Table 3-3
  uint8_t  errors;
  // additional information for VLAN
  uint16_t vlanTag;
} __attribute__ ((packed));

/*
 * Transmit Descriptor (legacy type)
 *   data structure that contains the transmit data buffer address
 *   see pcie-gbe-controllers 3.4.2
 */
struct E1000TxDesc {
  // buffer address
  phys_addr bufAddr;
  // segment length
  uint16_t length;
  // checksum offset
  uint8_t  cso;
  // command
  uint8_t  cmd;
  // status & reserved
  uint8_t  sta;
  // checksum start
  uint8_t  css;
  // special field
  uint16_t special;
} __attribute__ ((packed));

class E1000;
struct BsdDriver {
  E1000 *parent;
  struct adapter *adapter;
};

class E1000 : public DevPCI, Polling {
public:
 E1000(uint8_t bus, uint8_t device, bool mf) : DevPCI(bus, device, mf) {}
  static void InitPCI(uint16_t vid, uint16_t did, uint8_t bus, uint8_t device, bool mf);
  // from Polling
  void Handle() override;
  BsdDriver bsd;
 protected:
};

#endif /* __RAPH_KERNEL_E1000_H__ */
