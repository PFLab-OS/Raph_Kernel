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

extern "C" {
#include <acpi.h>
#include <accommon.h>
}

#include "apic.h"
#include "raph_acpi.h"

#define ACPI_MAX_INIT_TABLES    16
static ACPI_TABLE_DESC      TableArray[ACPI_MAX_INIT_TABLES];

void AcpiCtrl::Setup() {
  AcpiInitializeTables (TableArray, ACPI_MAX_INIT_TABLES, TRUE);

  ACPI_TABLE_HEADER *table;
  AcpiGetTable("APIC", 1, &table);
  apic_ctrl->SetMADT(reinterpret_cast<MADT *>(table));
}

MCFG *AcpiCtrl::GetMCFG() {
  ACPI_TABLE_HEADER *table;
  AcpiGetTable("MCFG", 1, &table);
  return reinterpret_cast<MCFG *>(table);
}

HPETDT *AcpiCtrl::GetHPETDT() {
  ACPI_TABLE_HEADER *table;
  AcpiGetTable("HPET", 1, &table);
  return reinterpret_cast<HPETDT *>(table);
}

FADT *AcpiCtrl::GetFADT() {
  ACPI_TABLE_HEADER *table;
  AcpiGetTable("FACP", 1, &table);
  return reinterpret_cast<FADT *>(table);
}

void AcpiCtrl::SetupAcpica() {
  kassert(!ACPI_FAILURE(AcpiInitializeSubsystem()));
  kassert(!ACPI_FAILURE(AcpiReallocateRootTable()));
  kassert(!ACPI_FAILURE(AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION)));
  kassert(!ACPI_FAILURE(AcpiLoadTables()));
  kassert(!ACPI_FAILURE(AcpiInitializeObjects(ACPI_FULL_INITIALIZATION)));
} 
  
void AcpiCtrl::Shutdown() {
  AcpiEnterSleepStatePrep(5);
  asm volatile("cli;");
  AcpiEnterSleepState(5);
  kernel_panic("acpi", "could not halt system");
}
