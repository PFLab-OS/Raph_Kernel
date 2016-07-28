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

#ifndef __RAPH_KERNEL_MEASURE_H__
#define __RAPH_KERNEL_MEASURE_H__

#include <timer.h>
#include <tty.h>
#include <global.h>

#define measure for(uint64_t t1 = 0, t2 = timer->ReadMainCnt(); ({if (t1 != 0) gtty->Printf("s","<","d",(timer->ReadMainCnt() - t2) * timer->GetCntClkPeriod(),"s","ns>"); (t1 == 0);}) ; t1++)

#endif // __RAPH_KERNEL_MEASURE_H__

