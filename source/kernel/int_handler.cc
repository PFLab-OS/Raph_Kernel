#include <global.h>
#include <keyboard.h>
#include <apic.h>


void intKeyboard(Regs *reg){
  uint8_t data;
  data=inb(0x60);
  if(data<(1<<7))  keyboard->Write(data);
  //  gtty->Printf("d",(int)data,"s"," ");
  outb(0xa0,0x061);
}
