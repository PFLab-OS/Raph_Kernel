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

#ifndef __RAPH_KERNEL_DEV_DEVICE_H__
#define __RAPH_KERNEL_DEV_DEVICE_H__

// !!! important !!!
// 派生クラスはstatic void Attach(); を作成する事
class Device {
 public:
  Device() {
  }
  ~Device() {
  }
  static void Attach() {} // dummy
};

template<class T>
static inline void AttachDevices() {
  T::Attach();
}

template<class T1, class T2, class... Rest>
static inline void AttachDevices() {
  T1::Attach();
  AttachDevices<T2, Rest...>();
}

#endif /* __RAPH_KERNEL_DEV_DEVICE_H__ */
