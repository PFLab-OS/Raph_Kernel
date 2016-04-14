#include <acpica.h>
#include <acpica/source/include/acpi.h> 
#include <acpica/source/include/accommon.h> 
#include <acpica/source/include/acapps.h> 
#include <acpica/source/tools/acpiexec/aecommon.h> 


bool Acpica::Shutdown() {
  AcpiEnterSleepStatePrep(5);
  //  cli(); // disable interrupts
  AcpiEnterSleepState(5);
  //  panic("power off"); // in case it didn't work!
  return true;
}
