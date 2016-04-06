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
 * Author: Levelfour
 * 
 */

#ifndef __RAPH_KERNEL_NET_NETCTRL_H__
#define __RAPH_KERNEL_NET_NETCTRL_H__

// L2/L3/L4Ctrls$B$N=i4|2=(B
// main$B$+$i8F$S$@$9(B
// main$B$N=i4|2=%7!<%1%s%9$+$i%M%C%H%o!<%/%3%s%H%m!<%i$r1#JC$9$kL\E*(B
// -> $B%M%C%H%o!<%/%3%s%H%m!<%i$O(Bsocket$B7PM3$G%"%/%;%9$9$k(B
void InitNetCtrl();
void DismissNetCtrl();

#endif // __RAPH_KERNEL_NET_NETCTRL_H__
