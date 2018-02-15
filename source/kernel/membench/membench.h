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

#include <x86.h>

struct Uint64 {
  uint64_t i;
} __attribute__((aligned(64)));

extern SyncLow sync_1;
extern Sync2Low sync_2;
extern Sync2Low sync_3;
extern SyncLow sync_4;

extern Uint64 f_array[256];
extern volatile Uint64 monitor[37 * 8];

extern const char ip_addr[];
extern const char port[];

static inline bool is_knl() { return x86::get_display_family_model() == 0x0657; }
