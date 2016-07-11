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

#include <sys/types.h>
#include <raph.h>
#include <list.h>

class BsdDevPci;

class BsdDevice {
 public:
  template<class T>
    void SetMasterClass(T *master) {
    _master = reinterpret_cast<void *>(master);
  }
  template<class T>
    T *GetMasterClass() {
    return reinterpret_cast<T *>(_master);
  }
  void SetClass(BsdDevPci *pci) {
    _pci = pci;
  }
  BsdDevPci *GetPciClass() {
    kassert(_pci != nullptr);
    return _pci;
  }
  void AddChild(const char *name, int unit) {
    BsdDevice *child = _children.PushBack()->GetObject();
    child->name = name;
    child->unit = unit;
  }
  struct adapter *adapter;
  void *ivar;
  int unit = 0;
  const char *name;
 private:
  BsdDevPci *_pci = nullptr;
  void *_master;
  ObjectList<BsdDevice> _children;
};
