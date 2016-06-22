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

#include <sys/lock.h>
#include <sys/mutex.h>
#include <spinlock.h>

void	_mtx_init(struct lock_object *l) {
  l->lock = new SpinLock();
}

void	_mtx_destroy(struct lock_object *l) {
  delete l->lock;
}

void __mtx_lock(struct lock_object *l) {
  l->lock->Lock();
}

void __mtx_unlock(struct lock_object *l) {
  l->lock->Unlock();
}
