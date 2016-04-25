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
 * Author: Yuchiki
 * 
 */

#ifndef __RAPH_KERNEL_SHELL_H__
#define __RAPH_KERNEL_SHELL_H__

class Shell {
 public:
  void Setup();
  void Register(const char *name, void (*func)(void));

  void Exec(const char *name);

 private:
  static const int kBufSize = 10;
  static const int kNameSize = 10;
  int _next_buf = 0;
  struct NameFuncMapping {
    char name[kNameSize];
    void (*func)(void);
  } _name_func_mapping[kBufSize];
};
#endif //__RAPH_KERNEL_SHELL_H__
