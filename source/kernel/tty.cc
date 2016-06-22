/*
 *
 * Copyright (c) 2016 Project Raphine
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

#include <tty.h>
#include <mem/virtmem.h>

void Tty::PrintString(String *str) {
  for(int i = 0; i < String::length; i++) {
    if (str->str[i] == '\0') {
      return;
    }
    Write(str->str[i]);
  }
  if (str->next != nullptr) {
    PrintString(str->next);
  }
}

Tty::String *Tty::String::New() {
  String *str = reinterpret_cast<String *>(virtmem_ctrl->Alloc(sizeof(String)));
  new(str) String;
  str->Init();
  return str;
}

void Tty::String::Delete() {
  if (next != nullptr) {
    next->Delete();
  }
  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(this));
}
