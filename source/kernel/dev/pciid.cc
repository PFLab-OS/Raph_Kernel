#include <global.h>
#include <tty.h>
#include <dev/pci.h>
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

void Table::Search(uint8_t bus, uint8_t device, uint8_t function, const char *search) {
  uint16_t vendorid = pci_ctrl->ReadReg<uint16_t>(bus, device, function, PciCtrl::kVendorIDReg);
  uint16_t deviceid = pci_ctrl->ReadReg<uint16_t>(bus, device, function, PciCtrl::kDeviceIDReg);
  uint16_t subvendorid = pci_ctrl->ReadReg<uint16_t>(bus, device, function, PciCtrl::kSubsystemVendorIdReg);
  uint16_t subdeviceid = pci_ctrl->ReadReg<uint16_t>(bus, device, function, PciCtrl::kSubsystemIdReg);
  
  Vendor *p = vendor;
  Device *d = nullptr;
  Subsystem *s = nullptr;
  if(p == nullptr) {
    gtty->Cprintf("error: Initialization failed\n");
    return;
  }
  while(p->id != vendorid) {
    if(p->id > vendorid || p->next == nullptr) {
      p = nullptr;
      break;
    }
    p = p->next;
  }
  if (p == nullptr) {
    goto display;
  }
  d = p->device;
  if (d == nullptr) {
    goto display;
  }
  while(d->id != deviceid) {
    if(d->id > deviceid || d->next == nullptr) {
      d = nullptr;
      break;
    }
    d = d->next;
  }
  if (d == nullptr) {
    goto display;
  }
  s = d->subsystem;
  if (s == nullptr) {
    goto display;
  }
  while(s->id != subvendorid) {
    if(s->id > subvendorid || s->next == nullptr) {
      s = nullptr;
      break;
    }
    s = s->next;
  }

 display:
  if (search == nullptr ||
      (p != nullptr && strstr(p->name, search) != nullptr) ||
      (d != nullptr && strstr(d->name, search) != nullptr) ||
      (s != nullptr && strstr(s->name, search) != nullptr)) {
    gtty->Cprintf("%x::%x.%x %s", bus, device, function, p == nullptr ? "???" : p->name);
    gtty->Cprintf(" %s", d == nullptr ? "???" : d->name);
    gtty->Cprintf(" %s", s == nullptr ? "???" : s->name);
    gtty->Cprintf(" [%04x]:[%04x]:[%04x, %04x]\n", vendorid, deviceid, subvendorid, subdeviceid);
  }
}
