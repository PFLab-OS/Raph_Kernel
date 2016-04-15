#include "acpi.h"
#include <stdio.h>
int main() {
  ACPI_STATUS status;
  puts("InitSubSystem");
  AcpiInitializeSubsystem();
  puts("init tables");
  AcpiInitializeTables(NULL, 16, FALSE);
  puts("load tables");
  AcpiLoadTables();
  puts("enable subsystem");
  AcpiEnableSubsystem(0);
  puts("init objs");
  AcpiInitializeObjects(0);
  puts("terminate");
  AcpiTerminate();
  puts("terminated");
  while(1){
    __asm__("hlt");
  }
  return 0;
}
