/*
 *
 * Copyright (c) 2015 Project Raphine
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

#ifndef __RAPH_KERNEL_ACPI_H__
#define __RAPH_KERNEL_ACPI_H__

#include <stdint.h>
#include <string.h>
#include "global.h"
#include "mem/physmem.h"

// see acpi spec

// RSDP v1
struct RSDPDescriptor {
  char Signature[8];
  uint8_t Checksum;
  char OEMID[6];
  uint8_t Revision;
  uint32_t RsdtAddress;
} __attribute__ ((packed));

// RSDP v2
struct RSDPDescriptor20 {
  RSDPDescriptor firstPart;
  uint32_t Length;
  uint64_t XsdtAddress;
  uint8_t ExtendedChecksum;
  uint8_t reserved[3];
} __attribute__ ((packed));

struct ACPISDTHeader {
  char Signature[4];
  uint32_t Length;
  uint8_t Revision;
  uint8_t Checksum;
  char OEMID[6];
  char OEMTableID[8];
  uint32_t OEMRevision;
  uint32_t CreatorID;
  uint32_t CreatorRevision;
} __attribute__ ((packed));

struct MCFG;
struct HPETDT;

class AcpiCtrl {
public:
  AcpiCtrl() {
  }
  void Setup(RSDPDescriptor *rsdp);
  MCFG *GetMCFG() {
    return _mcfg;
  }
  HPETDT *GetHPETDT() {
    return _hpetdt;
  }
private:
  int CheckACPISDTHeader(ACPISDTHeader *header) {
    uint8_t sum = 0;
    uint8_t *byte = reinterpret_cast<uint8_t *>(header);
    for (uint32_t i = 0; i < header->Length; i++, byte++) {
      sum += *byte;
    }
    return (sum == 0);
  }
  MCFG *_mcfg = nullptr;
  HPETDT *_hpetdt = nullptr;
};

#endif /* __RAPH_KERNEL_ACPI_H__ */
