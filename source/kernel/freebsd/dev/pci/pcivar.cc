/*
 *
 * Copyright (c) 2016 Raphine Project
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

#include <dev/pci/pcivar.h>
#include <sys/types-raph.h>
#include <sys/bus-raph.h>

extern "C" {
  int pci_alloc_msi(device_t dev, int *count) {
    int dcount = dev->GetPciClass()->GetMsiCount();
    if (dcount == 0) {
      return -1;
    }
    if (dcount < *count) {
      *count = dcount;
    }
    dev->GetPciClass()->SetupMsi();
    return 0;
  }

  int pci_alloc_msix(device_t dev, int *count) {
    return -1;
  }
}
