/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Berkeley Software Design Inc's name may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BERKELEY SOFTWARE DESIGN INC ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL BERKELEY SOFTWARE DESIGN INC BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from BSDI $Id: mutex.h,v 2.7.2.35 2000/04/27 03:10:26 cp Exp $
 * $FreeBSD$
 */

#ifndef _SYS_MUTEX_H_
#define _SYS_MUTEX_H_

#include <sys/queue.h>
#include <sys/_lock.h>
#include <sys/_mutex.h>

#ifdef _KERNEL
#include <sys/lockstat.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void	_mtx_init(struct lock_object *l);
void	_mtx_destroy(struct lock_object *l);
void	mtx_sysinit(void *arg);
int	_mtx_trylock_flags_(struct lock_object *l);
void	mutex_init(void);
void	__mtx_lock_sleep(struct lock_object *l);
void	__mtx_unlock_sleep(struct lock_object *l);
#ifdef SMP
void	_mtx_lock_spin_cookie(struct lock_object *l);
#endif
void	__mtx_lock_flags(struct lock_object *l);
void	__mtx_unlock_flags(struct lock_object *l);
void	__mtx_lock_spin_flags(struct lock_object *l);
void	__mtx_unlock_spin_flags(struct lock_object *l);
#if defined(INVARIANTS) || defined(INVARIANT_SUPPORT)
void	__mtx_assert(struct lock_object *l);
#endif

/*
 * Top-level macros to provide lock cookie once the actual mtx is passed.
 * They will also prevent passing a malformed object to the mtx KPI by
 * failing compilation as the mtx_lock reserved member will not be found.
 */
#define	mtx_init(m, n, t, o)						\
	_mtx_init(&(m)->lock_object)
#define	mtx_destroy(m)							\
	_mtx_destroy(&(m)->lock_object)
#define	mtx_trylock_flags_(m, o, f, l)					\
	_mtx_trylock_flags_(&(m)->lock_object)
#define	_mtx_lock_sleep(m, t, o, f, l)					\
	__mtx_lock_sleep(&(m)->lock_object)
#define	_mtx_unlock_sleep(m, o, f, l)					\
	__mtx_unlock_sleep(&(m)->lock_object, o, f, l)
#ifdef SMP
#define	_mtx_lock_spin(m, t, o, f, l)					\
	_mtx_lock_spin_cookie(&(m)->lock_object)
#endif
#define	_mtx_lock_flags(m, o, f, l)					\
	__mtx_lock_flags(&(m)->lock_object)
#define	_mtx_unlock_flags(m, o, f, l)					\
	__mtx_unlock_flags(&(m)->lock_object)
#define	_mtx_lock_spin_flags(m, o, f, l)				\
	__mtx_lock_spin_flags(&(m)->lock_object)
#define	_mtx_unlock_spin_flags(m, o, f, l)				\
	__mtx_unlock_spin_flags(&(m)->lock_object)

  void __mtx_lock(struct lock_object *l);
  void __mtx_unlock(struct lock_object *l);
  
#ifndef LOCK_DEBUG
#error LOCK_DEBUG not defined, include <sys/lock.h> before <sys/mutex.h>
#endif
#if LOCK_DEBUG > 0 || defined(MUTEX_NOINLINE)
#define	mtx_lock_flags_(m)				\
	_mtx_lock_flags((m))
#define	mtx_unlock_flags_(m)				\
	_mtx_unlock_flags((m))
#define	mtx_lock_spin_flags_(m)			\
	_mtx_lock_spin_flags((m))
#define	mtx_unlock_spin_flags_(m)			\
	_mtx_unlock_spin_flags((m))
#else	/* LOCK_DEBUG == 0 && !MUTEX_NOINLINE */
#define	mtx_lock_flags_(m)				\
	__mtx_lock(&(m)->lock_object)
#define	mtx_unlock_flags_(m)				\
	__mtx_unlock(&(m)->lock_object)
#define	mtx_lock_spin_flags_(m)			\
	__mtx_lock_spin((m))
#define	mtx_unlock_spin_flags_(m)			\
	__mtx_unlock_spin((m))
#endif	/* LOCK_DEBUG > 0 || MUTEX_NOINLINE */
  
#ifdef INVARIANTS
#define	mtx_assert_(m)				\
	_mtx_assert((m))

#define GIANT_REQUIRED	mtx_assert_(&Giant, MA_OWNED, __FILE__, __LINE__)

#else	/* INVARIANTS */
#define mtx_assert_(m)	(void)0
#define GIANT_REQUIRED
#endif	/* INVARIANTS */

#define	mtx_lock_flags(m, opts)                       \
	mtx_lock_flags_((m))
#define	mtx_unlock_flags(m, opts)					\
	mtx_unlock_flags_((m))
#define	mtx_lock_spin_flags(m, opts)					\
	mtx_lock_spin_flags_((m))
#define	mtx_unlock_spin_flags(m, opts)					\
	mtx_unlock_spin_flags_((m))
#define mtx_trylock_flags(m, opts)					\
	mtx_trylock_flags_((m))
#define	mtx_assert(m, what)						\
	mtx_assert_((m))

/*
 * Exported lock manipulation interface.
 *
 * mtx_lock(m) locks MTX_DEF mutex `m'
 *
 * mtx_lock_spin(m) locks MTX_SPIN mutex `m'
 *
 * mtx_unlock(m) unlocks MTX_DEF mutex `m'
 *
 * mtx_unlock_spin(m) unlocks MTX_SPIN mutex `m'
 *
 * mtx_lock_spin_flags(m, opts) and mtx_lock_flags(m, opts) locks mutex `m'
 *     and passes option flags `opts' to the "hard" function, if required.
 *     With these routines, it is possible to pass flags such as MTX_QUIET
 *     to the appropriate lock manipulation routines.
 *
 * mtx_trylock(m) attempts to acquire MTX_DEF mutex `m' but doesn't sleep if
 *     it cannot. Rather, it returns 0 on failure and non-zero on success.
 *     It does NOT handle recursion as we assume that if a caller is properly
 *     using this part of the interface, he will know that the lock in question
 *     is _not_ recursed.
 *
 * mtx_trylock_flags(m, opts) is used the same way as mtx_trylock() but accepts
 *     relevant option flags `opts.'
 *
 * mtx_initialized(m) returns non-zero if the lock `m' has been initialized.
 *
 * mtx_owned(m) returns non-zero if the current thread owns the lock `m'
 *
 * mtx_recursed(m) returns non-zero if the lock `m' is presently recursed.
 */ 
#define mtx_lock(m)		mtx_lock_flags((m), 0)
#define mtx_lock_spin(m)	mtx_lock_spin_flags((m), 0)
#define mtx_trylock(m)		mtx_trylock_flags((m), 0)
#define mtx_unlock(m)		mtx_unlock_flags((m), 0)
#define mtx_unlock_spin(m)	mtx_unlock_spin_flags((m), 0)
  
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* _KERNEL */

#endif	/* _SYS_MUTEX_H_ */
