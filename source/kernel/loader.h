/*
 *
 * Copyright (c) 2017 Raphine Project
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

#ifndef __RAPH_KERNEL_LOADER_H__
#define __RAPH_KERNEL_LOADER_H__

#include <mem/virtmem.h>

using FType = int (*)(int, char*[]);

class LoaderInterface {
public:
  virtual void MapAddr(virt_addr start, virt_addr end) = 0;
  virtual void Execute(FType f) = 0;
};

#include <arch_loader.h>

#endif /* __RAPH_KERNEL_LOADER_H__ */
