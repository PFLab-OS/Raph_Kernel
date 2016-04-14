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
#include <mem/tmpmem.h>

void Queue::Push(void *data) {
  Locker locker(_lock);
  Container *c = reinterpret_cast<Container *>(tmpmem_ctrl->Alloc(sizeof(Container)));
  kassert(_last->next == nullptr);
  _last->next = c;
  _last = c;
  c->data = data;
  c->next = nullptr;
}

bool Queue::Pop(void *&data) {
  Locker locker(_lock);
  if (IsEmpty()) {
    return false;
  }
  Container *c = _first.next;
  kassert(c != nullptr);
  data = c->data;
  _first.next = c->next;
  if (_last == c) {
    _last = &_first;
  }
  return true;
}
