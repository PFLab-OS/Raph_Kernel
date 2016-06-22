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

#include <queue.h>
#include <global.h>
#include <mem/virtmem.h>
#include <raph.h>

void Queue::Push(void *data) {
  Container *c = reinterpret_cast<Container *>(virtmem_ctrl->Alloc(sizeof(Container)));
  c->data = data;
  c->next = nullptr;
  Locker locker(_lock);
  kassert(_last->next == nullptr);
  _last->next = c;
  _last = c;
}

bool Queue::Pop(void *&data) {
  Container *c;
  {
    Locker locker(_lock);
    if (IsEmpty()) {
      return false;
    }
    c = _first.next;
    kassert(c != nullptr);
    _first.next = c->next;
    if (_last == c) {
      _last = &_first;
    }
  }
  data = c->data;
  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(c));
  return true;
}
