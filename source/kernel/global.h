/*
 *
 * Copyright (c) 2015 Raphine Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Author: Liva
 * 
 */

#ifndef __RAPH_KERNEL_GLOBAL_H__
#define __RAPH_KERNEL_GLOBAL_H__

// TODO: global namespace

class SpinLockCtrl;
class AcpiCtrl;
class MultibootCtrl;
class PagingCtrl;
class PhysmemCtrl;
class VirtmemCtrl;
class TmpmemCtrl;
class TaskCtrl;
class Gdt;
class Idt;

class Tty;
class Timer;
class Keyboard;

class PCICtrl;

class NetDevCtrl;

extern SpinLockCtrl *spinlock_ctrl;
extern AcpiCtrl *acpi_ctrl;

#ifndef __UNIT_TEST__
class ApicCtrl;
extern ApicCtrl *apic_ctrl;
#else
class PthreadCtrl;
extern PthreadCtrl *apic_ctrl;
#endif // __UNIT_TEST__

extern MultibootCtrl *multiboot_ctrl;
extern PagingCtrl *paging_ctrl;
extern PhysmemCtrl *physmem_ctrl;
extern VirtmemCtrl *virtmem_ctrl;
extern TmpmemCtrl *tmpmem_ctrl;
extern TaskCtrl *task_ctrl;
extern Gdt *gdt;
extern Idt *idt;


extern Tty *gtty;
extern Timer *timer;
extern Keyboard *keyboard;

extern PCICtrl *pci_ctrl;

/*
 * Network Controllers
 */

// Network Devices
class DevEthernetCtrl;
extern DevEthernetCtrl *netdev_ctrl;

// ARP Table
class ArpTable;
extern ArpTable *arp_table;

#endif // __RAPH_KERNEL_GLOBAL_H__
