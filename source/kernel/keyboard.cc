#include <global.h>
#include <keyboard.h>
#include <apic.h>
#include <idt.h>

void Keyboard::Setup(int lapicid) {
  apic_ctrl->SetupPicInt(kIrqKeyboard);
  apic_ctrl->SetupIoInt(kIrqKeyboard, lapicid, Idt::ReservedIntVector::kKeyboard);
  idt->SetIntCallback(Idt::ReservedIntVector::kKeyboard, Keyboard::Handler);
}

void Keyboard::Write(uint8_t code){
  if (_next_w == _next_r+1) _overflow = true;
  _buf[_next_w] = code;
  _next_w++;
  _count++;
  _next_w %= kbufSize;
}
uint8_t Keyboard::Read() {
  uint8_t data = _buf[_next_r];
  if (_next_r == _next_w) _underflow = true;
  _next_r++;
  _count--;
  _next_r %= kbufSize;
  return data;
}

char Keyboard::GetCh() {
  uint8_t data = Read();
  return kScanCode[data];
}
bool Keyboard::Overflow() {
  return _overflow;
}
bool Keyboard::Underflow() {
  return _underflow;
}

int Keyboard::Count() {
  return _count;
}

void Keyboard::Reset() {
  _overflow = false;
  _underflow = false;
  _count=_next_w = 0;
  _next_r = 0;
}


void Keyboard::Handler(Regs *reg) { //static
  uint8_t data;
  data = inb(kDataPort);
  if(data < (1 << 7))  keyboard->Write(data);
  //  gtty->Printf("d",(int)data,"s"," ");
  apic_ctrl->SendPicEoi();
}

const char Keyboard::kScanCode[256] = {
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
