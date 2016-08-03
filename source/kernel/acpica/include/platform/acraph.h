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

#ifndef __ACRAPH_H__
#define __ACRAPH_H__

#define ACPI_USE_SYSTEM_CLIBRARY
#define ACPI_USE_DO_WHILE_0
#define ACPI_MUTEX_TYPE             ACPI_OSL_MUTEX

#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define ACPI_FLUSH_CPU_CACHE()

#define ACPI_CACHE_T                ACPI_MEMORY_LIST
#define ACPI_USE_LOCAL_CACHE        1

#define ACPI_MACHINE_WIDTH          64
#define COMPILER_DEPENDENT_INT64    int64_t
#define COMPILER_DEPENDENT_UINT64   uint64_t

#ifndef __cdecl
#define __cdecl
#endif

#include "acgcc.h"

#endif /* __ACRAPH_H__ */
