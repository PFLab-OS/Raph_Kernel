//#include <global.h>
//#include <gtty.h>


#include "acpi.h"
#include "accommon.h"
#include "amlcode.h"
#include "acparser.h"




#define _COMPONENT          ACPI_OS_SERVICES
ACPI_MODULE_NAME    ("osraphinexf")


/*Environmental and ACPI Tables*/
ACPI_STATUS AcpiOsInitialize() {
  return AE_OK;
}

ACPI_STATUS AcpiOsTerminate() {
  return AE_OK;
}

ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer() {
  return 0; //fix later but it will work 
}

ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *PredefinedObject, ACPI_STRING *NewValue) {
  *NewValue = NULL;
}

ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_TABLE_HEADER **NewTable) {
  *NewTable = NULL;
}

/*Memory Management*/
void *AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress, ACPI_SIZE Length){
  return NULL; //this doesn't work i guess
}
void AcpiOsUnmapMemory(void *where, ACPI_SIZE length){

}
ACPI_STATUS AcpiOsGetPhysicalAddress(void *LogicalAddress, ACPI_PHYSICAL_ADDRESS *PhysicalAddress){
  *PhysicalAddress = *((ACPI_PHYSICAL_ADDRESS *)LogicalAddress);  //doubtful
  return AE_OK;
}
void *AcpiOsAllocate(ACPI_SIZE Size){
  //  return malloc(Size);
}
void AcpiOsFree(void *Memory){
  //  return free(Memory);
}
BOOLEAN AcpiOsReadable(void *Memory, ACPI_SIZE Length){
  return TRUE;
}
BOOLEAN AcpiOsWritable(void *Memory, ACPI_SIZE Length){
  return TRUE;
}

//#define ACPI_CACHE_T                ACPI_MEMORY_LIST
#define ACPI_USE_LOCAL_CACHE        1



/* Multithreading and Scheduling Services*/

ACPI_THREAD_ID AcpiOsGetThreadId(){
  return 0; //no threads yet
}

ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type, ACPI_OSD_EXEC_CALLBACK Function, void *Context){
  return AE_NOT_IMPLEMENTED;
}

void AcpiOsSleep(UINT64 Milliseconds){
  //not implemented
}

void AcpiOsStall(UINT32 Microseconds){
  //not implemented
}

/*Mutual Exclusion and Synchronization */
#if (ACPI_MUTEX_TYPE != ACPI_BINARY_SEMAPHORE)

ACPI_STATUS AcpiOsCreateMutex(ACPI_MUTEX *OutHandle){
  return AE_NOT_IMPLEMENTED;
}
void AcpiOsDeleteMutex(ACPI_MUTEX Handle){
  //NOT_IMPLEMENTED
}
ACPI_STATUS AcpiOsAcquireMutex(ACPI_MUTEX Handle, UINT16 Timeout){
  return AE_NOT_IMPLEMENTED;
}
void AcpiOsReleaseMutex(ACPI_MUTEX Handle){
  //NOT_IMPLEMENTED
}

#endif 

ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits, UINT32 InitialUnits, ACPI_SEMAPHORE *OutHandle){
  return AE_NOT_IMPLEMENTED;
}
ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle){
  return AE_NOT_IMPLEMENTED;
}
ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units, UINT16 Timeout){
  return AE_NOT_IMPLEMENTED;
}
ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units){
  return AE_NOT_IMPLEMENTED;
}
ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *OutHandle){
  return AE_NOT_IMPLEMENTED;
}
void AcpiOsDeleteLock(ACPI_HANDLE Handle){
  //NOT_IMPLEMENTED
}
ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle){
  return AE_NOT_IMPLEMENTED;
}
void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags){
  //NOT_IMPLEMENTED
}

/*Interrupt Handling */

ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 InterruptLevel, ACPI_OSD_HANDLER Handler, void *Context){
  //NOT_IMPLEMENTED
  return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER Handler){
  return AE_NOT_IMPLEMENTED;
}

///////////////////////////////////////////////////////////////

ACPI_STATUS AcpiOsReadPciConfiguration (
    ACPI_PCI_ID             *PciId,
    UINT32                  PciRegister,
    UINT64                  *Value,
    UINT32                  Width)
{

    *Value = 0;
    return (AE_OK);
}


ACPI_STATUS
AcpiOsWritePciConfiguration (
    ACPI_PCI_ID             *PciId,
    UINT32                  PciRegister,
    UINT64                  Value,
    UINT32                  Width)
{

    return (AE_OK);
}




ACPI_STATUS
AcpiOsReadMemory (
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT64                  *Value,
    UINT32                  Width)
{

    switch (Width)
    {
    case 8:
    case 16:
    case 32:
    case 64:

        *Value = 0;
        break;

    default:

        return (AE_BAD_PARAMETER);
    }
    return (AE_OK);
}


ACPI_STATUS
AcpiOsWriteMemory (
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT64                  Value,
    UINT32                  Width)
{

    return (AE_OK);
}


ACPI_STATUS
AcpiOsReadPort (
    ACPI_IO_ADDRESS         Address,
    UINT32                  *Value,
    UINT32                  Width)
{

    switch (Width)
    {
    case 8:

        *Value = 0xFF;
        break;

    case 16:

        *Value = 0xFFFF;
        break;

    case 32:

        *Value = 0xFFFFFFFF;
        break;

    default:

        return (AE_BAD_PARAMETER);
    }

    return (AE_OK);
}

ACPI_STATUS
AcpiOsWritePort (
    ACPI_IO_ADDRESS         Address,
    UINT32                  Value,
    UINT32                  Width)
{

    return (AE_OK);
}

void
AcpiOsWaitEventsComplete (
    void)
{
    return;
}

void ACPI_INTERNAL_VAR_XFACE
AcpiOsPrintf (
    const char              *Fmt,
    ...)
{
    /* va_list                 Args; */
    /* UINT8                   Flags; */


    /* Flags = AcpiGbl_DbOutputFlags; */
    /* if (Flags & ACPI_DB_REDIRECTABLE_OUTPUT) */
    /* { */
    /*     /\* Output is directable to either a file (if open) or the console *\/ */

    /*     if (AcpiGbl_DebugFile) */
    /*     { */
    /*         /\* Output file is open, send the output there *\/ */

    /*         va_start (Args, Fmt); */
    /*         vfprintf (AcpiGbl_DebugFile, Fmt, Args); */
    /*         va_end (Args); */
    /*     } */
    /*     else */
    /*     { */
    /*         /\* No redirection, send output to console (once only!) *\/ */

    /*         Flags |= ACPI_DB_CONSOLE_OUTPUT; */
    /*     } */
    /* } */

    /* if (Flags & ACPI_DB_CONSOLE_OUTPUT) */
    /* { */
    /*     va_start (Args, Fmt); */
    /*     vfprintf (AcpiGbl_OutputFile, Fmt, Args); */
    /*     va_end (Args); */
    /* } */
}

void
AcpiOsVprintf (
    const char              *Fmt,
    va_list                 Args)
{
    /* UINT8                   Flags; */
    /* char                    Buffer[ACPI_VPRINTF_BUFFER_SIZE]; */


    /*
     * We build the output string in a local buffer because we may be
     * outputting the buffer twice. Using vfprintf is problematic because
     * some implementations modify the args pointer/structure during
     * execution. Thus, we use the local buffer for portability.
     *
     * Note: Since this module is intended for use by the various ACPICA
     * utilities/applications, we can safely declare the buffer on the stack.
     * Also, This function is used for relatively small error messages only.
     */
    /* vsnprintf (Buffer, ACPI_VPRINTF_BUFFER_SIZE, Fmt, Args); */

    /* Flags = AcpiGbl_DbOutputFlags; */
    /* if (Flags & ACPI_DB_REDIRECTABLE_OUTPUT) */
    /* { */
    /*     /\* Output is directable to either a file (if open) or the console *\/ */

    /*     if (AcpiGbl_DebugFile) */
    /*     { */
    /*         /\* Output file is open, send the output there *\/ */

    /*         fputs (Buffer, AcpiGbl_DebugFile); */
    /*     } */
    /*     else */
    /*     { */
    /*         /\* No redirection, send output to console (once only!) *\/ */

    /*         Flags |= ACPI_DB_CONSOLE_OUTPUT; */
    /*     } */
    /* } */

    /* if (Flags & ACPI_DB_CONSOLE_OUTPUT) */
    /* { */
    /*     fputs (Buffer, AcpiGbl_OutputFile); */
    /* } */
}


ACPI_STATUS
AcpiOsSignal (
    UINT32                  Function,
    void                    *Info)
{

    switch (Function)
    {
    case ACPI_SIGNAL_FATAL:

        break;

    case ACPI_SIGNAL_BREAKPOINT:

        break;

    default:

        break;
    }

    return (AE_OK);
}




UINT64
AcpiOsGetTimer (
    void)
{
    /* struct timeval          time; */


    /* /\* This timer has sufficient resolution for user-space application code *\/ */

    /* //    gettimeofday (&time, NULL); */

    /* /\* (Seconds * 10^7 = 100ns(10^-7)) + (Microseconds(10^-6) * 10^1 = 100ns) *\/ */

    /* return (((UINT64) time.tv_sec * ACPI_100NSEC_PER_SEC) + */
    /*         ((UINT64) time.tv_usec * ACPI_100NSEC_PER_USEC)); */

  return 0;
}


ACPI_STATUS
AcpiOsPhysicalTableOverride (
    ACPI_TABLE_HEADER       *ExistingTable,
    ACPI_PHYSICAL_ADDRESS   *NewAddress,
    UINT32                  *NewTableLength)
{

    return (AE_SUPPORT);
}
