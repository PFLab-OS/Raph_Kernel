/*
 *
 * Copyright (c) 2015 Raphine Project
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
#include <tty.h>
#include <task.h>
#include <global.h>

void AcpiCtrl::Setup() {
  kassert(!ACPI_FAILURE(AcpiInitializeSubsystem()));
  kassert(!ACPI_FAILURE(AcpiInitializeTables(NULL, 16, FALSE)));

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

void AcpiGlobalEventHandler(UINT32 type, ACPI_HANDLE device, UINT32 num, void *context) {
  if (num == ACPI_EVENT_POWER_BUTTON) {
    task_ctrl->Register(cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority), reinterpret_cast<Task *>(context));
  }
}

void AcpiCtrl::SetupAcpica() {
  kassert(!ACPI_FAILURE(AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION)));
  kassert(!ACPI_FAILURE(AcpiLoadTables()));
  kassert(!ACPI_FAILURE(AcpiInitializeObjects(ACPI_FULL_INITIALIZATION)));

  ClassFunction<AcpiCtrl> func;
  func.Init(this, &AcpiCtrl::GlobalEventHandler, nullptr);
  _global_event_task.SetFunc(func);
   AcpiInstallGlobalEventHandler(AcpiGlobalEventHandler, reinterpret_cast<void *>(&_global_event_task));
} 
  
void AcpiCtrl::Shutdown() {
  AcpiEnterSleepStatePrep(5);
  asm volatile("cli;");
  AcpiEnterSleepState(5);
  kernel_panic("acpi", "could not halt system");
}

void AcpiCtrl::GlobalEventHandler(void *) {
  gtty->Cprintf("shutting down the system...\n");
  Shutdown();
}

void AcpiCtrl::Reset() {
  AcpiReset();

  // send CPU reset to 8042 keyboad controller
  outb(0x64, 0xfe);
}

static ACPI_STATUS GetIntNum(ACPI_RESOURCE *resource, void *context) {
  int *irq = reinterpret_cast<int *>(context);
  switch(resource->Type) {
  case ACPI_RESOURCE_TYPE_START_DEPENDENT:
  case ACPI_RESOURCE_TYPE_END_TAG: {
    return_ACPI_STATUS (AE_OK);
  }
  case ACPI_RESOURCE_TYPE_IRQ: {
    ACPI_RESOURCE_IRQ *p = &resource->Data.Irq;
    if (p == nullptr || p->InterruptCount == 0) {
      return_ACPI_STATUS (AE_OK);
    }
    *irq = p->Interrupts[0];
    break;
  }
  case ACPI_RESOURCE_TYPE_EXTENDED_IRQ: {
    ACPI_RESOURCE_EXTENDED_IRQ *p = &resource->Data.ExtendedIrq;
    if (p == nullptr || p->InterruptCount == 0) {
      return_ACPI_STATUS (AE_OK);
    }
    *irq = p->Interrupts[0];
    break;
  }
  default: {
    kassert(false);
  }
  }
  
  return_ACPI_STATUS (AE_CTRL_TERMINATE);
}

struct ArgContainer1 {
  DevPci *device;
  PciCtrl::IntPin intpin;
  bool source_not_found;
  bool source_available_flag;
  int source_index;
  char *linkname;
};

static ACPI_STATUS GetRouteTable(ACPI_HANDLE obj_handle, UINT32 level, void *context, void **ReturnValue) {
  ArgContainer1 *container = reinterpret_cast<ArgContainer1 *>(context);
  DevPci *device = container->device;
  ACPI_STATUS status;
  ACPI_DEVICE_INFO *info;

  ACPI_BUFFER buf;
  ACPI_OBJECT param;

  buf.Pointer = &param;
  buf.Length = sizeof(param);
  
  status = AcpiGetObjectInfo(obj_handle, &info);
  if (ACPI_FAILURE(status)) {
    return_ACPI_STATUS (AE_OK);
  }

  kassert(info->Type == ACPI_TYPE_DEVICE);
  if (info->Flags & ACPI_PCI_ROOT_BRIDGE) {
    status = AcpiEvaluateObjectTyped(obj_handle, METHOD_NAME__BBN, nullptr, &buf, ACPI_TYPE_INTEGER);
    uint8_t bus = 0;
    if (ACPI_SUCCESS(status)) {
      kassert(param.Type == ACPI_TYPE_INTEGER);
      bus = param.Integer.Value;
    }

    if (bus == device->GetBus()) {
      buf.Pointer = nullptr;
      buf.Length = ACPI_ALLOCATE_BUFFER;
      status = AcpiGetIrqRoutingTable(obj_handle, &buf);

      if (ACPI_SUCCESS(status)) {
        ACPI_PCI_ROUTING_TABLE *entry = reinterpret_cast<ACPI_PCI_ROUTING_TABLE *>(buf.Pointer);
        while((entry != nullptr) && (entry->Length > 0)) {
          if ((((entry->Address >> 16) & 0xFFFF) == device->GetDevice()) &&
              (entry->Pin + 1 == static_cast<unsigned int>(container->intpin))) {

            if (entry->Source[0] == '\0') {
              container->source_not_found = false;
              container->source_available_flag = false;
              container->source_index = entry->SourceIndex;
              return_ACPI_STATUS (AE_CTRL_TERMINATE);
            }

            status = AcpiGetHandle(ACPI_ROOT_OBJECT, entry->Source, &obj_handle);

            if (ACPI_SUCCESS(status)) {
              status = AcpiGetObjectInfo(obj_handle, &info);
              if (ACPI_SUCCESS(status)) {
                container->source_not_found = false;
                container->source_available_flag = true;
                container->linkname = reinterpret_cast<char *>(&info->Name);
          
                return_ACPI_STATUS (AE_CTRL_TERMINATE);
              }
            }
          }
          entry = reinterpret_cast<ACPI_PCI_ROUTING_TABLE *>(reinterpret_cast<virt_addr>(entry) + entry->Length);
        }
      }
    }
  }

  return_ACPI_STATUS (AE_OK);
}

struct ArgContainer2 {
  char *linkname;
  int irq;
};

static ACPI_STATUS GetIntLink(ACPI_HANDLE obj_handle, UINT32 level, void *context, void **ReturnValue) {
  ArgContainer2 *container = reinterpret_cast<ArgContainer2 *>(context);
  ACPI_STATUS status;
  ACPI_DEVICE_INFO *info;

  status = AcpiGetObjectInfo(obj_handle, &info);
  if (ACPI_FAILURE(status)) {
    return_ACPI_STATUS (AE_OK);
  }

  kassert(info->Type == ACPI_TYPE_DEVICE);
  if (info->Valid & ACPI_VALID_HID &&
      !strncmp(info->HardwareId.String, "PNP0C0F", info->HardwareId.Length)) {

    if (!strncmp(reinterpret_cast<char *>(&info->Name), container->linkname, 4)) {
      int irq = -1;
      AcpiWalkResources(obj_handle, METHOD_NAME__CRS, GetIntNum, &irq);
      container->irq = irq;
    }
  }

  return_ACPI_STATUS (AE_OK);
}


int AcpiCtrl::GetPciIntNum(DevPci *device) {
  // TODO : is MaxDepth valid?
  PciCtrl::IntPin intpin = device->GetLegacyIntPin();
  if (intpin == PciCtrl::IntPin::kInvalid) {
    return -1;
  }
  ArgContainer1 container1;
  container1.device = device;
  container1.intpin = intpin;
  container1.source_not_found = true;
  AcpiWalkNamespace(ACPI_TYPE_DEVICE, ACPI_ROOT_OBJECT, 100, GetRouteTable, nullptr, reinterpret_cast<void *>(&container1), nullptr);
  if (container1.source_not_found) {
    return -1;
  }
  if (!container1.source_available_flag) {
    return container1.source_index;
  }

  ArgContainer2 container2;
  container2.irq = -1;
  container2.linkname = container1.linkname;
  AcpiWalkNamespace(ACPI_TYPE_DEVICE, ACPI_ROOT_OBJECT, 100, GetIntLink, nullptr, reinterpret_cast<void *>(&container2), nullptr);

  return container2.irq;
}
