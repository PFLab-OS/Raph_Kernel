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

#include "apic.h"
#include "raph_acpi.h"

void AcpiCtrl::Setup(RSDPDescriptor *rsdp) {
  if (strncmp(rsdp->Signature, "RSD PTR ", 8)) {
    return;
  }

  uint8_t sum = 0;
  uint8_t *byte = reinterpret_cast<uint8_t *>(rsdp);
  for (uint32_t i = 0; i < 20; i++, byte++) {
    sum += *byte;
  }
  if (sum != 0){
    return;
  }

  //TODO マッピング範囲内に収まってるかどうか調べた方が良い
  ACPISDTHeader *rsdt = reinterpret_cast<ACPISDTHeader *>(p2v(rsdp->RsdtAddress));
  if (strncmp(rsdt->Signature, "RSDT", 4)) {
    return;
  }

  if (!CheckACPISDTHeader(rsdt)) {
    return;
  }

  for (uint32_t i = 0; i < rsdt->Length; i++) {
    ACPISDTHeader *sdth = reinterpret_cast<ACPISDTHeader *>(p2v(*(reinterpret_cast<uint32_t *>(ptr2virtaddr(rsdt + 1) + i * sizeof(uint32_t)))));
    if (!strncmp(sdth->Signature, "APIC", 4)) {
      apic_ctrl->SetMADT(reinterpret_cast<MADT *>(ptr2virtaddr(sdth)));
    } else if (!strncmp(sdth->Signature, "MCFG", 4)) {
      _mcfg = reinterpret_cast<MCFG *>(ptr2virtaddr(sdth));
    } else if (!strncmp(sdth->Signature, "HPET", 4)) {
      _hpetdt = reinterpret_cast<HPETDT *>(ptr2virtaddr(sdth));
    } else if (!strncmp(sdth->Signature, "FACP", 4)) {
      _fadt = reinterpret_cast<FADT *>(ptr2virtaddr(sdth));
    }
  }
}

