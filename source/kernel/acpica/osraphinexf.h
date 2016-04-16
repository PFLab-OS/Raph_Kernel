//#include <global.h>
//#include <gtty.h>


#include "acpi.h"
#include "accommon.h"
#include "amlcode.h"
#include "acparser.h"



/*Environmental and ACPI Tables*/
ACPI_STATUS AcpiOsInitialize();

ACPI_STATUS AcpiOsTerminate();

ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer();

ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *PredefinedObject, ACPI_STRING *NewValue);

ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_TABLE_HEADER **NewTable);

/*Memory Management*/
void *AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress, ACPI_SIZE Length);
void AcpiOsUnmapMemory(void *where, ACPI_SIZE length);
ACPI_STATUS AcpiOsGetPhysicalAddress(void *LogicalAddress, ACPI_PHYSICAL_ADDRESS *PhysicalAddress);
void *AcpiOsAllocate(ACPI_SIZE Size);
void AcpiOsFree(void *Memory);
BOOLEAN AcpiOsReadable(void *Memory, ACPI_SIZE Length);
BOOLEAN AcpiOsWritable(void *Memory, ACPI_SIZE Length);

//#define ACPI_CACHE_T                ACPI_MEMORY_LIST
#define ACPI_USE_LOCAL_CACHE        1
/* Multithreading and Scheduling Services*/

ACPI_THREAD_ID AcpiOsGetThreadId();

ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type, ACPI_OSD_EXEC_CALLBACK Function, void *Context);

void AcpiOsSleep(UINT64 Milliseconds);

void AcpiOsStall(UINT32 Microseconds);
/*Mutual Exclusion and Synchronization */
#if (ACPI_MUTEX_TYPE != ACPI_BINARY_SEMAPHORE)

ACPI_STATUS AcpiOsCreateMutex(ACPI_MUTEX *OutHandle);
void AcpiOsDeleteMutex(ACPI_MUTEX Handle);
ACPI_STATUS AcpiOsAcquireMutex(ACPI_MUTEX Handle, UINT16 Timeout);
void AcpiOsReleaseMutex(ACPI_MUTEX Handle);

#endif 

ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits, UINT32 InitialUnits, ACPI_SEMAPHORE *OutHandle);
ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle);
ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units, UINT16 Timeout);
ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units);
ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *OutHandle);
void AcpiOsDeleteLock(ACPI_HANDLE Handle);
ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle);
void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags);

/*Interrupt Handling */

ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 InterruptLevel, ACPI_OSD_HANDLER Handler, void *Context);

ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER Handler);

///////////////////////////////////////////////////////////////

ACPI_STATUS AcpiOsReadPciConfiguration (
    ACPI_PCI_ID             *PciId,
    UINT32                  PciRegister,
    UINT64                  *Value,
    UINT32                  Width);


ACPI_STATUS
AcpiOsWritePciConfiguration (
    ACPI_PCI_ID             *PciId,
    UINT32                  PciRegister,
    UINT64                  Value,
    UINT32                  Width);



ACPI_STATUS
AcpiOsReadMemory (
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT64                  *Value,
    UINT32                  Width);

ACPI_STATUS
AcpiOsWriteMemory (
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT64                  Value,
    UINT32                  Width);

ACPI_STATUS
AcpiOsReadPort (
    ACPI_IO_ADDRESS         Address,
    UINT32                  *Value,
    UINT32                  Width);

ACPI_STATUS
AcpiOsWritePort (
    ACPI_IO_ADDRESS         Address,
    UINT32                  Value,
    UINT32                  Width);
void
AcpiOsWaitEventsComplete (
			  void);
void ACPI_INTERNAL_VAR_XFACE
AcpiOsPrintf (
    const char              *Fmt,
    ...);

void
AcpiOsVprintf (
    const char              *Fmt,
    va_list                 Args);

void AcpiRaphPrintf2( const char *Fmt,const char *Str);
   

ACPI_STATUS
AcpiOsSignal (
    UINT32                  Function,
    void                    *Info);

UINT64
AcpiOsGetTimer (
		void);


ACPI_STATUS
AcpiOsPhysicalTableOverride (
    ACPI_TABLE_HEADER       *ExistingTable,
    ACPI_PHYSICAL_ADDRESS   *NewAddress,
    UINT32                  *NewTableLength);
