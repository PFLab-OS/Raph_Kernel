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
#include <acparser.h>
#include <amlcode.h>
#include <acnamesp.h>
#include <acdebug.h>
#include <actables.h>
#include <acinterp.h>
#include <amlresrc.h>
}

#include <limits.h>
#include <apic.h>
#include <raph_acpi.h>
#include <dev/pci.h>

#define ACPI_MAX_INIT_TABLES    16
static ACPI_TABLE_DESC      TableArray[ACPI_MAX_INIT_TABLES];

void AcpiCtrl::Setup() {
  AcpiInitializeTables (TableArray, ACPI_MAX_INIT_TABLES, TRUE);

  ACPI_TABLE_HEADER *table;
  kassert(!ACPI_FAILURE(AcpiGetTable("APIC", 1, &table)));
  apic_ctrl->SetMADT(reinterpret_cast<MADT *>(table));
}

MCFG *AcpiCtrl::GetMCFG() {
  ACPI_TABLE_HEADER *table;
  kassert(!ACPI_FAILURE(AcpiGetTable("MCFG", 1, &table)));
  return reinterpret_cast<MCFG *>(table);
}

HPETDT *AcpiCtrl::GetHPETDT() {
  ACPI_TABLE_HEADER *table;
  kassert(!ACPI_FAILURE(AcpiGetTable("HPET", 1, &table)));
  return reinterpret_cast<HPETDT *>(table);
}

FADT *AcpiCtrl::GetFADT() {
  ACPI_TABLE_HEADER *table;
  kassert(!ACPI_FAILURE(AcpiGetTable("FACP", 1, &table)));
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
// TODO remove this
#include <tty.h>


static ACPI_STATUS DisplayOneDevice2(ACPI_HANDLE obj_handle, UINT32 level, void *context, void **ReturnValue) {
  ACPI_STATUS status;
  ACPI_DEVICE_INFO *info;
  ACPI_BUFFER  path;
  char buffer[256];
  uint8_t *bus = reinterpret_cast<uint8_t *>(context);

  path.Length = sizeof(buffer);
  path.Pointer = buffer;
  
  status = AcpiGetName(obj_handle, ACPI_FULL_PATHNAME, &path);
  if (!ACPI_SUCCESS(status)) {
    return AE_OK;
  }

  status = AcpiGetObjectInfo(obj_handle, &info);
  if (!ACPI_SUCCESS(status)) {
    return AE_OK;
  }

  kassert(info->Type == ACPI_TYPE_DEVICE);
  ACPI_BUFFER buf;
  ACPI_OBJECT param;
  buf.Pointer = &param;
  buf.Length = sizeof(param);
  
  status = AcpiEvaluateObjectTyped(obj_handle, "_ADR", nullptr, &buf, ACPI_TYPE_INTEGER);
  if (ACPI_FAILURE(status)) {
    return AE_OK;
  }

  kassert(param.Type == ACPI_TYPE_INTEGER);
  gtty->Cprintf("%s %x %x\n", path.Pointer, (int)param.Integer.Value, info->Address);
    
  // pci_ctrl->InitPciDevices(*bus, param.Integer.Value >> 16, param.Integer.Value & 0xffff);
  //  AcpiWalkNamespace(ACPI_TYPE_METHOD, obj_handle, 100, DisplayOneDevice3, nullptr, nullptr, nullptr);
  return AE_OK;
}

static ACPI_STATUS DisplayOneDevice(ACPI_HANDLE obj_handle, UINT32 level, void *context, void **ReturnValue) {
  ACPI_STATUS status;
  ACPI_DEVICE_INFO *info;
  ACPI_BUFFER  path;
  char buffer[256];

  path.Length = sizeof(buffer);
  path.Pointer = buffer;
  
  status = AcpiGetName(obj_handle, ACPI_FULL_PATHNAME, &path);
  if (!ACPI_SUCCESS(status)) {
    return AE_OK;
  }

  status = AcpiGetObjectInfo(obj_handle, &info);
  if (!ACPI_SUCCESS(status)) {
    return AE_OK;
  }

  kassert(info->Type == ACPI_TYPE_DEVICE);
  if (info->Flags & ACPI_PCI_ROOT_BRIDGE) {
    ACPI_BUFFER buf;
    ACPI_OBJECT param;
    buf.Pointer = &param;
    buf.Length = sizeof(param);

    status = AcpiEvaluateObjectTyped(obj_handle, "_ADR", nullptr, &buf, ACPI_TYPE_INTEGER);
    if (ACPI_SUCCESS(status)) {
      kassert(param.Type == ACPI_TYPE_INTEGER);
      gtty->Cprintf("root> %s %x\n", path.Pointer, (int)param.Integer.Value);
      status = AcpiEvaluateObjectTyped(obj_handle, "_BBN", nullptr, &buf, ACPI_TYPE_INTEGER);
      if (ACPI_SUCCESS(status)) {
        kassert(param .Type == ACPI_TYPE_INTEGER);
        uint8_t bus = param.Integer.Value;
        AcpiWalkNamespace(ACPI_TYPE_DEVICE, obj_handle, 100, DisplayOneDevice2, nullptr, reinterpret_cast<void *>(&bus), nullptr);
      } else {
        kassert(false);
      }
    } else {
      kassert(false);
    }
  }

  return AE_OK;
}

void AcpiCtrl::TraversePciNameSpace() {
  // TODO : is MaxDepth valid?
  AcpiWalkNamespace(ACPI_TYPE_DEVICE, ACPI_ROOT_OBJECT, 100, DisplayOneDevice, nullptr, nullptr, nullptr);
}
