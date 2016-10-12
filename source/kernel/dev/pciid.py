import re
import sys

def PrintIDAndName(l):
    sys.stdout.write('0x' + str(l[0]) + ', "')
    for i in l[1:]:
        print i,
    sys.stdout.write('"'),

def Print2IDsAndName(l):
    sys.stdout.write('0x' + str(l[0]) + ', ')
    PrintIDAndName(l[1:])

print '''
#include <global.h>
#include <tty.h>
#include "pciid.h"

PciTable::~PciTable() {
  while(vendor != nullptr) {
    while(vendor->device != nullptr) {
      while(vendor->device->subsystem != nullptr) {
        PciSubsystem *p = vendor->device->subsystem;
        vendor->device->subsystem = p->next;
        delete p;
      }
      PciDevice *p = vendor->device;
      vendor->device = p->next;
      delete p;
    }
    PciVendor *p = vendor;
    vendor = vendor->next;
    delete p;
  }
}

void PciTable::SearchDevice(int vendorid, int deviceid, int subvendorid, int subdeviceid) {
  PciVendor *p = vendor;
  if(p == nullptr) {
    gtty->Cprintf("error: Initialization failed\\n");
    return;
  }
  while(p->id != vendorid) {
    if(p->next == nullptr) {
      gtty->Cprintf("not found: %04x:%04x:%04x:%04x\\n", vendorid, deviceid, subvendorid, subdeviceid);
      gtty->Cprintf("\\n");
      return;
    }
    p = p->next;
  }
  gtty->Cprintf("(%04x) %s", vendorid, p->name);
  PciDevice *d = p->device;
  if(d == nullptr) {
    gtty->Cprintf("\\n");
    return;
  }
  while(d->id != deviceid) {
    if(d->next == nullptr) {
      gtty->Cprintf("\\n");
      return;
    }
    d = d->next;
  }
  gtty->Cprintf(":(%04x) %s", deviceid, d->name);
  PciSubsystem *s = d->subsystem;
  if(s == nullptr) {
    gtty->Cprintf("\\n");
    return;
  }
  while(s->id != subvendorid) {
    if(s->next == nullptr) {
      gtty->Cprintf("\\n");
      return;
    }
    s = s->next;
  }
  gtty->Cprintf(":(%04x, %04x) %s\\n", subvendorid, subdeviceid, s->name);
}

void PciTable::Init() {
  PciVendor *pv = nullptr;
  PciDevice *pd = nullptr;
  PciSubsystem *ps = nullptr;
'''
file = open('pci.ids.formatted', 'r')

isfirstv = True
isfirstd = True
isfirsts = True

for line in file:
    if line.startswith('C'):
        break
    elif line.startswith('#'):
        continue
    nl = re.sub('(\\|")', '\\$1', line)
    s = nl.split()
    if s == []:
        continue
    ind = line.count('\t')
    if ind == 0:
        if isfirstv:
            print '  vendor = new PciVendor(',
            PrintIDAndName(s)
            print ');'
            print '  pv = vendor;'
            isfirstv = False
        else:
            print '  pv->next = new PciVendor(',
            PrintIDAndName(s)
            print ');'
            print '  pv = pv->next;'
        isfirstd = True
    elif ind == 1:
        if isfirstd:
            print '  pv->device = new PciDevice(',
            PrintIDAndName(s)
            print ');'
            print '  pd = pv->device;'
        else:
            print '  pd->next = new PciDevice(',
            PrintIDAndName(s)
            print ');'
            print '  pd = pd->next;'
        isfirsts = True
    elif ind == 2:
        if isfirsts:
            print '  pd->subsystem = new PciSubsystem(',
            Print2IDsAndName(s)
            print ');'
            print '  ps = pd->subsystem;'
            isfirsts = False
        else:
            print '  ps->next = new PciSubsystem(',
            Print2IDsAndName(s)
            print ');'
            print '  ps = ps->next;'

file.close()

print '}'
