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
#include "pciid.h"

using namespace PciData;

void Table::Init() {
  Vendor *pv = nullptr;
  Device *pd = nullptr;
  Subsystem *ps = nullptr;
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
            print '  vendor = new Vendor(',
            PrintIDAndName(s)
            print ');'
            print '  pv = vendor;'
            isfirstv = False
        else:
            print '  pv->next = new Vendor(',
            PrintIDAndName(s)
            print ');'
            print '  pv = pv->next;'
        isfirstd = True
    elif ind == 1:
        if isfirstd:
            print '  pv->device = new Device(',
            PrintIDAndName(s)
            print ');'
            print '  pd = pv->device;'
            isfirstd = False
        else:
            print '  pd->next = new Device(',
            PrintIDAndName(s)
            print ');'
            print '  pd = pd->next;'
        isfirsts = True
    elif ind == 2:
        if isfirsts:
            print '  pd->subsystem = new Subsystem(',
            Print2IDsAndName(s)
            print ');'
            print '  ps = pd->subsystem;'
            isfirsts = False
        else:
            print '  ps->next = new Subsystem(',
            Print2IDsAndName(s)
            print ');'
            print '  ps = ps->next;'

file.close()

print '}'
