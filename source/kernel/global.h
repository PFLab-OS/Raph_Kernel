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

class SpinLockCtrl;
class AcpiCtrl;
class ApicCtrl;
class MultibootCtrl;
class PagingCtrl;
class PhysmemCtrl;
class VirtmemCtrl;

extern SpinLockCtrl *spinlock_ctrl;
extern AcpiCtrl *acpi_ctrl;
extern ApicCtrl *apic_ctrl;
extern MultibootCtrl *multiboot_ctrl;
extern PagingCtrl *paging_ctrl;
extern VirtmemCtrl *virtmem_ctrl;
extern PhysmemCtrl *physmem_ctrl;


#endif // __RAPH_KERNEL_GLOBAL_H__
