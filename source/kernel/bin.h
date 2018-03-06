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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * Author: Liva
 *
 */

#pragma once

#include <mem/virtmem.h>
#include <loader.h>

class BinObjectInterface {
 public:
  enum class ErrorState {
    kOk,
    kNotSupportedType,
  };
  virtual ErrorState Init() __attribute__((warn_unused_result)) = 0;
};

class ExecutableObject : public BinObjectInterface {
 public:
  virtual void Resume() = 0;
  virtual ErrorState LoadMemory(bool) = 0;
};
