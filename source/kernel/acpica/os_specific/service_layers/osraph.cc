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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * Author: Liva
 *
 */

#include <raph.h>
#include <tty.h>
#include <spinlock.h>
#include <semaphore.h>
#include <timer.h>
#include <idt.h>
#include <mem/physmem.h>
#include <cpu.h>
#include <dev/pci.h>
#include <multiboot.h>

extern "C" {

#include "acpi.h"
#include "accommon.h"
#include "amlcode.h"
#include "acparser.h"
#include "acdebug.h"

struct AcpiIntHandler {
  ACPI_OSD_HANDLER func;
  int vector;
};
static AcpiIntHandler handler[10];
ACPI_STATUS AcpiOsInitialize() {
  for (int i = 0; i < 10; i++) {
    handler[i].func = nullptr;
  }
  return (AE_OK);
}

ACPI_STATUS AcpiOsTerminate() { return (AE_OK); }

ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer() {
  phys_addr ptr = multiboot_ctrl->GetAcpiRoot();
  if (ptr == 0) {
    kernel_panic("ACPICA", "no RSDP");
  }
  return ptr;
}

ACPI_STATUS AcpiOsPredefinedOverride(
    const ACPI_PREDEFINED_NAMES *PredefinedObject, ACPI_STRING *NewValue) {
  *NewValue = NULL;
  return (AE_OK);
}

ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *ExistingTable,
                                ACPI_TABLE_HEADER **NewTable) {
  *NewTable = NULL;
  return (AE_NO_ACPI_TABLES);
}

ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER *ExistingTable,
                                        ACPI_PHYSICAL_ADDRESS *NewAddress,
                                        UINT32 *NewTableLength) {
  return (AE_SUPPORT);
}

void *AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress, ACPI_SIZE Length) {
  return reinterpret_cast<void *>(p2v(PhysicalAddress));
}

void AcpiOsUnmapMemory(void *where, ACPI_SIZE length) {}

ACPI_STATUS AcpiOsGetPhysicalAddress(void *LogicalAddress,
                                     ACPI_PHYSICAL_ADDRESS *PhysicalAddress) {
  *PhysicalAddress = v2p(reinterpret_cast<virt_addr>(LogicalAddress));
  return (AE_OK);
}

void *AcpiOsAllocate(ACPI_SIZE Size) {
  return reinterpret_cast<void *>(system_memory_space->kvc.Alloc(Size));
}

void AcpiOsFree(void *Memory) {
  system_memory_space->kvc.Free(reinterpret_cast<virt_addr>(Memory));
}

BOOLEAN AcpiOsReadable(void *Memory, ACPI_SIZE Length) {
  kassert(false);
  return false;
}

BOOLEAN AcpiOsWritable(void *Memory, ACPI_SIZE Length) {
  kassert(false);
  return false;
}

ACPI_THREAD_ID AcpiOsGetThreadId() { return 1; }

ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type,
                          ACPI_OSD_EXEC_CALLBACK Function, void *Context) {
  kassert(false);
  return (AE_ERROR);
}

ACPI_STATUS AcpiOsSignal(UINT32 Function, void *Info) {
  kassert(false);
  return (AE_ERROR);
}

void AcpiOsWaitEventsComplete() { kassert(false); }

UINT64 AcpiOsGetTimer() {
  return timer->ReadTime().GetRaw() * 10;
}

void AcpiOsSleep(UINT64 Milliseconds) { timer->BusyUwait(Milliseconds * 1000); }

void AcpiOsStall(UINT32 Microseconds) { timer->BusyUwait(Microseconds); }

ACPI_STATUS AcpiOsCreateMutex(ACPI_MUTEX *OutHandle) {
  SpinLock *lock = new SpinLock();
  *OutHandle = reinterpret_cast<ACPI_MUTEX>(lock);
  return (AE_OK);
}

void AcpiOsDeleteMutex(ACPI_MUTEX Handle) {
  SpinLock *lock = reinterpret_cast<SpinLock *>(Handle);
  delete lock;
}

ACPI_STATUS AcpiOsAcquireMutex(ACPI_MUTEX Handle, UINT16 Timeout) {
  SpinLock *lock = reinterpret_cast<SpinLock *>(Handle);
  if (Timeout == 0) {
    return (lock->Trylock() == ReturnState::kOk) ? (AE_OK) : (AE_ERROR);
  } else if (Timeout > 0) {
    Time limit = timer->ReadTime() + Timeout * 1000;
    while (timer->ReadTime() < limit) {
      if (lock->Trylock() == ReturnState::kOk) {
        return (AE_OK);
      }
    }
    return (AE_ERROR);
  } else {
    lock->Lock();
    return (AE_OK);
  }
}

void AcpiOsReleaseMutex(ACPI_MUTEX Handle) {
  SpinLock *lock = reinterpret_cast<SpinLock *>(Handle);
  lock->Unlock();
}

ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits, UINT32 InitialUnits,
                                  ACPI_SEMAPHORE *OutHandle) {
  Semaphore *semaphore = new Semaphore(MaxUnits, InitialUnits);
  *OutHandle = reinterpret_cast<ACPI_SEMAPHORE>(semaphore);
  return (AE_OK);
}

ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle) {
  Semaphore *semaphore = reinterpret_cast<Semaphore *>(Handle);
  delete semaphore;
  return (AE_OK);
}

ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units,
                                UINT16 Timeout) {
  Semaphore *semaphore = reinterpret_cast<Semaphore *>(Handle);
  if (Timeout == 0) {
    return (semaphore->Tryacquire() == 0) ? (AE_OK) : (AE_ERROR);
  } else if (Timeout > 0) {
    Time limit = timer->ReadTime() + Timeout * 1000;
    while (timer->ReadTime() < limit) {
      if (semaphore->Tryacquire() == 0) {
        return (AE_OK);
      }
    }
    return (AE_ERROR);
  } else {
    semaphore->Acquire();
    return (AE_OK);
  }
}

ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units) {
  Semaphore *semaphore = reinterpret_cast<Semaphore *>(Handle);
  for (; Units > 0; Units--) {
    semaphore->Release();
  }
  return (AE_OK);
}

ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *OutHandle) {
  SpinLock *lock = new SpinLock();
  *OutHandle = reinterpret_cast<ACPI_SPINLOCK>(lock);
  return (AE_OK);
}

void AcpiOsDeleteLock(ACPI_HANDLE Handle) {
  SpinLock *lock = reinterpret_cast<SpinLock *>(Handle);
  delete lock;
}

ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle) {
  SpinLock *lock = reinterpret_cast<SpinLock *>(Handle);
  lock->Lock();
  return 0;
}

void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags) {
  SpinLock *lock = reinterpret_cast<SpinLock *>(Handle);
  lock->Unlock();
}

static void AcpiHandlerSub(Regs *rs, void *arg) {
  for (int i = 0; i < 10; i++) {
    if (static_cast<int>(rs->n) == handler[i].vector) {
      handler[i].func(arg);
      return;
    }
  }
}

ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 InterruptLevel,
                                          ACPI_OSD_HANDLER Handler,
                                          void *Context) {
  kassert(apic_ctrl != nullptr);
  kassert(idt != nullptr);
  CpuId cpuid = cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority);
  int vector =
      idt->SetIntCallback(cpuid, AcpiHandlerSub, Context, Idt::EoiType::kLapic);
  for (int i = 0; i < 10; i++) {
    if (handler[i].func == nullptr) {
      handler[i].func = Handler;
      handler[i].vector = vector;
    }
  }
  apic_ctrl->SetupIoInt(InterruptLevel, apic_ctrl->GetApicIdFromCpuId(cpuid),
                        vector, true, true);
  return (AE_OK);
}

ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber,
                                         ACPI_OSD_HANDLER Handler) {
  // TODO fix this
  return (AE_OK);
}

void ACPI_INTERNAL_VAR_XFACE AcpiOsPrintf(const char *Fmt, ...) {
  va_list Args;
  va_start(Args, Fmt);
  AcpiOsVprintf(Fmt, Args);
  va_end(Args);
}

void AcpiOsVprintf(const char *Fmt, va_list Args) {
  kassert(gtty != nullptr);
  gtty->Vprintf(Fmt, Args);
}

ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 *Value,
                             UINT32 Width) {
  switch (Width) {
    case 8: {
      *Value = *(reinterpret_cast<uint8_t *>(p2v(Address)));
      break;
    }
    case 16: {
      *Value = *(reinterpret_cast<uint16_t *>(p2v(Address)));
      break;
    }
    case 32: {
      *Value = *(reinterpret_cast<uint32_t *>(p2v(Address)));
      break;
    }
    case 64: {
      *Value = *(reinterpret_cast<uint64_t *>(p2v(Address)));
      break;
    }
    default: {
      kassert(false);
      return (AE_ERROR);
    }
  }
  return (AE_OK);
}

ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 Value,
                              UINT32 Width) {
  switch (Width) {
    case 8: {
      *(reinterpret_cast<uint8_t *>(p2v(Address))) = Value;
      break;
    }
    case 16: {
      *(reinterpret_cast<uint16_t *>(p2v(Address))) = Value;
      break;
    }
    case 32: {
      *(reinterpret_cast<uint32_t *>(p2v(Address))) = Value;
      break;
    }
    case 64: {
      *(reinterpret_cast<uint64_t *>(p2v(Address))) = Value;
      break;
    }
    default: {
      kassert(false);
      return (AE_ERROR);
    }
  }
  return (AE_OK);
}

ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32 *Value,
                           UINT32 Width) {
  switch (Width) {
    case 8: {
      *Value = inb(Address);
      break;
    }
    case 16: {
      *Value = inw(Address);
      break;
    }
    case 32: {
      *Value = inl(Address);
      break;
    }
    default: {
      kassert(false);
      return (AE_ERROR);
    }
  }
  return (AE_OK);
}
ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value,
                            UINT32 Width) {
  switch (Width) {
    case 8: {
      outb(Address, Value);
      break;
    }
    case 16: {
      outw(Address, Value);
      break;
    }
    case 32: {
      outl(Address, Value);
      break;
    }
    default: {
      kassert(false);
      return (AE_ERROR);
    }
  }
  return (AE_OK);
}

ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register,
                                       UINT64 *Value, UINT32 Width) {
  kassert(pci_ctrl != nullptr);
  switch (Width) {
    case 8: {
      *(reinterpret_cast<uint8_t *>(Value)) = pci_ctrl->ReadPciReg<uint8_t>(
          PciId->Bus, PciId->Device, PciId->Function, Register);
      break;
    }
    case 16: {
      *(reinterpret_cast<uint16_t *>(Value)) = pci_ctrl->ReadPciReg<uint16_t>(
          PciId->Bus, PciId->Device, PciId->Function, Register);
      break;
    }
    case 32: {
      *(reinterpret_cast<uint32_t *>(Value)) = pci_ctrl->ReadPciReg<uint32_t>(
          PciId->Bus, PciId->Device, PciId->Function, Register);
      break;
    }
    default: {
      kassert(false);
      return (AE_ERROR);
    }
  }
  return (AE_OK);
}

ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register,
                                        ACPI_INTEGER Value, UINT32 Width) {
  kassert(pci_ctrl != nullptr);
  switch (Width) {
    case 8: {
      pci_ctrl->WritePciReg<uint8_t>(PciId->Bus, PciId->Device, PciId->Function,
                                     Register, Value);
      break;
    }
    case 16: {
      pci_ctrl->WritePciReg<uint16_t>(PciId->Bus, PciId->Device,
                                      PciId->Function, Register, Value);
      break;
    }
    case 32: {
      pci_ctrl->WritePciReg<uint32_t>(PciId->Bus, PciId->Device,
                                      PciId->Function, Register, Value);
      break;
    }
    default: {
      kassert(false);
      return (AE_ERROR);
    }
  }
  return (AE_OK);
}
}

