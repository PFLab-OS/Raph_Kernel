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

#ifndef __RAPH_KERNEL_FREEBSD_SYS_TYPES_RAPH_H__
#define __RAPH_KERNEL_FREEBSD_SYS_TYPES_RAPH_H__

#include <sys/types.h>
#include <raph.h>
#include <list.h>
#include <stdlib.h>

class BsdDevBus;
class BsdDevPci;

class BsdDevice {
public:
  BsdDevice(BsdDevice *parent, char *name_, int unit_) {
    _parent = parent;
    name = name_;
    unit = unit_;
    parent->AddChild(this);
  }
  BsdDevice() {
    _parent = nullptr;
    name = nullptr;
    unit = -1;
  }
  ~BsdDevice() {
    if (softc != nullptr) {
      free(softc);
    }
  }
  int ProbeAndAttach() {
    if (DevMethodProbe() == 0 && DevMethodAttach() == 0) {
      return 0;
    } else {
      return -1;
    }
  }
  void ProbeAndAttachChildren() {
    ObjectList<BsdDevice *>::Container *c = _children.GetBegin();
    while((c = c->GetNext()) != nullptr) {
      (*c->GetObject())->ProbeAndAttach();
    }
  }
  template<class T>
  T *GetMasterClass() {
    return reinterpret_cast<T *>(_master);
  }
  void SetBusClass(BsdDevBus *bus) {
    _bus = bus;
  }
  void SetPciClass(BsdDevPci *pci) {
    _pci = pci;
  }
  BsdDevBus *GetBusClass() {
    kassert(_bus != nullptr);
    return _bus;
  }
  BsdDevPci *GetPciClass() {
    kassert(_pci != nullptr);
    return _pci;
  }
  BsdDevice *GetParent() {
    return _parent;
  }
  void *softc = nullptr;
  void *ivar;
  int unit = 0;
  const char *name;
protected:
  virtual int DevMethodProbe() = 0;
  virtual int DevMethodAttach() = 0;
  template<class T>
  void InitBsdDevice(T *master, size_t size_of_softc) {
    SetMasterClass(master);
    softc = calloc(1, size_of_softc);
  }
  template<class T>
  void SetMasterClass(T *master) {
    _master = reinterpret_cast<void *>(master);
  }
private:
  void AddChild(BsdDevice *child_) {
    BsdDevice **child = _children.PushBack()->GetObject();
    *child = child_;
  }
  BsdDevBus *_bus = nullptr;
  BsdDevPci *_pci = nullptr;
  void *_master;
  ObjectList<BsdDevice *> _children;
  BsdDevice *_parent;
};

#endif // __RAPH_KERNEL_FREEBSD_SYS_TYPES_RAPH_H__
