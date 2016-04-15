
extern "C"{
#include "include/acpi.h"
}
#include "acpica.h"


#define __ACPICA_DEBUG__

#ifdef __ACPICA_DEBUG__
#include <stdio.h>
#define DEBUG(str)   (puts(str))
#else
#define DEBUG(str) 
#endif


void Acpica::Init() {
  ACPI_STATUS status;
  DEBUG("InitializeSubsytem segufo here.");
  AcpiInitializeSubsystem();
  DEBUG("Initialize tables");
  AcpiInitializeTables(NULL, 16, FALSE);
  DEBUG("load tables.");
  AcpiLoadTables();
  DEBUG("Enable Subsystem");
  AcpiEnableSubsystem(0);
  DEBUG("Init Obj");
  AcpiInitializeObjects(0);
}

void Acpica::Terminate() {
  AcpiTerminate();
}
