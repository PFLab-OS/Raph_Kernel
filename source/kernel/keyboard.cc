#include <global.h>
#include <keyboard.h>
#include <apic.h>

void Keyboard::Write(uint8_t code){
  if (_next_w==_next_r+1) _overflow=true;
  _buf[_next_w]=code;
  _next_w++;
  _count++;
  _next_w%=bufSize;
}
uint8_t Keyboard::Read(){
  uint8_t data=_buf[_next_r];
  if (_next_r==_next_w) _underflow=true;
  _next_r++;
  _count--;
  _next_r%=bufSize;
  return data;
}

char Keyboard::Getch(){
  uint8_t data=Read();
  return ScanCode[data];
}
bool Keyboard::Overflow(){
  return _overflow;
}
bool Keyboard::Underflow(){
  return _underflow;
}

int Keyboard::Count(){
  return _count;
}

void Keyboard::Reset(){
  _overflow=false;
  _underflow=false;
  _count=_next_w=_next_r=0;
}


void Keyboard::intKeyboard(Regs *reg){ //static
  uint8_t data;
  data=inb(0x60);
  if(data<(1<<7))  keyboard->Write(data);
  //  gtty->Printf("d",(int)data,"s"," ");
  outb(0xa0,0x061);
}

const char Keyboard::ScanCode[256]={
    '!','!','1','2','3','4','5','6',
    '7','8','9','0','-','!','!','\t',
    'q','w','e','r','t','y','u','i',
    'o','p','[',']','\n','!','a','s',
    'd','f','g','h','j','k','l',';',
    '"','^','!','\\','z','x','c','v',
    'b','n','m',',','.','/','!','!',
    '!',' ','!','!','!','!','!','!',
    '!','!','!','!','!','!','!','!',
    '!','!','!','!','!','!','!','!',
    '!','!','!','!','!','!','!','!',
    '!','!','!','!','!','!','!','!',
    '!','!','!','!','!','!','!','!',
    '!','!','!','!','!','!','!','!',
    '!','!','!','!','!','!','!','!',
    '!','!','!','!','!','!','!','!',
  };
