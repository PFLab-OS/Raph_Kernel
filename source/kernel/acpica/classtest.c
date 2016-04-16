#include "acpica.h"
#include <stdio.h>
Acpica *acpica;
int main() {
  Acpica _Acpica;
  acpica = &_Acpica;
  puts("init");
  acpica->Init();
  puts("initted");
  acpica->Terminate();
  while(1){
    puts("p");
  }
  return 0;
}
