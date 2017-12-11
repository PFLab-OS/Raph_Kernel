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

#ifndef __RAPH_KERNEL_MP_H__
#define __RAPH_KERNEL_MP_H__

#include <mem/virtmem.h>
#include <global.h>
#include <apic.h>

class MultiProcCtrl {
public:
  MultiProcCtrl() {
  }
  void Init() {
    _containers = kernel_virtmem_ctrl->Alloc(sizeof(class Container) * apic_ctrl->GetHowManyCpus());
    for(int i = 0; i < apic_ctrl->GetHowManyCpus(); i++) {
      new(&_containers[i]) Container;
    }
  }
private:
  class Container {
  public:
    Container() {
    }
  };
  Container *_containers;
};

#endif // __RAPH_KERNEL_MP_H__
