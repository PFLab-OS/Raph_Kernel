
extern "C"{
#include "include/acpi.h"
}
#include "acpica.h"

void Acpica::Init() {
  ACPI_STATUS status;
  AcpiInitializeSubsystem();
  AcpiTerminate();

  AcpiInitializeTables(NULL, 16, FALSE);
  AcpiLoadTables();
  AcpiEnableSubsystem(0);
  AcpiInitializeObjects(0);
}

void Acpica::Terminate() {
  AcpiTerminate();
}
