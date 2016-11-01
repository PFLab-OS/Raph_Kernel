#include <global.h>
#include <tty.h>
#include "pciid.h"

using namespace PciData;

Table::~Table() {
  while(vendor != nullptr) {
    while(vendor->device != nullptr) {
      while(vendor->device->subsystem != nullptr) {
        Subsystem *p = vendor->device->subsystem;
        vendor->device->subsystem = p->next;
        delete p;
      }
      Device *p = vendor->device;
      vendor->device = p->next;
      delete p;
    }
    Vendor *p = vendor;
    vendor = vendor->next;
    delete p;
  }
}

void Table::Search(int vendorid, int deviceid, int subvendorid, int subdeviceid) {
  Vendor *p = vendor;
  if(p == nullptr) {
    gtty->Cprintf("error: Initialization failed\n");
    return;
  }
  while(p->id != vendorid) {
    if(p->id > vendorid || p->next == nullptr) {
      gtty->Cprintf("[%04x]?:[%04x]?:[%04x]?:[%04x]?\n", vendorid, deviceid, subvendorid, subdeviceid);
      return;
    }
    p = p->next;
  }
  gtty->Cprintf("[%04x]%s", vendorid, p->name);
  Device *d = p->device;
  if(d == nullptr) {
    gtty->Cprintf(":[%04x]?:[%04x]?:[%04x]?\n", deviceid, subvendorid, subdeviceid);
    return;
  }
  while(d->id != deviceid) {
    if(d->id > deviceid || d->next == nullptr) {
      gtty->Cprintf(":[%04x]?:[%04x]?:[%04x]?\n", deviceid, subvendorid, subdeviceid);
      return;
    }
    d = d->next;
  }
  gtty->Cprintf(":[%04x] %s", deviceid, d->name);
  Subsystem *s = d->subsystem;
  if(s == nullptr) {
    gtty->Cprintf(":[%04x]?:[%04x]?\n", subvendorid, subdeviceid);
    return;
  }
  while(s->id != subvendorid) {
    if(s->id > subvendorid || s->next == nullptr) {
      gtty->Cprintf(":[%04x]?:[%04x]?\n", subvendorid, subdeviceid);
      return;
    }
    s = s->next;
  }
  gtty->Cprintf(":[%04x, %04x] %s\n", subvendorid, subdeviceid, s->name);
}
