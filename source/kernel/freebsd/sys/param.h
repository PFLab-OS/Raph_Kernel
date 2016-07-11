/*-
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)param.h	8.3 (Berkeley) 4/4/95
 * $FreeBSD$
 */

#ifndef _FREEBSD_PARAM_H_
#define _FREEBSD_PARAM_H_

#ifndef LOCORE
#include <sys/types.h>
#endif

#ifdef _KERNEL
#include <sys/cdefs.h>
#include <sys/errno.h>
#ifndef LOCORE
// #include <sys/time.h>
// #include <sys/priority.h>
#endif

#ifndef FALSE
#define FALSE   0
#endif
#ifndef TRUE
#define TRUE    1
#endif
#endif

#define nitems(x) (sizeof((x)) / sizeof((x)[0]))
#define rounddown(x, y) (((x)/(y))*(y))
#define rounddown2(x, y) ((x)&(~((y)-1)))          /* if y is power of two */
#define roundup(x, y) ((((x)+((y)-1))/(y))*(y))  /* to any y */
#define roundup2(x, y)  (((x)+((y)-1))&(~((y)-1))) /* if y is powers of two */
#define powerof2(x) ((((x)-1)&(x))==0)

/* Macros for min/max. */
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#include <machine/param.h>

/*
 * Constants related to network buffer management.
 * MCLBYTES must be no larger than PAGE_SIZE.
 */
#ifndef	MSIZE
#define	MSIZE		256		/* size of an mbuf */
#endif

#ifndef	MCLSHIFT
#define MCLSHIFT	11		/* convert bytes to mbuf clusters */
#endif	/* MCLSHIFT */

#define MCLBYTES	(1 << MCLSHIFT)	/* size of an mbuf cluster */

/* clicks to bytes */
#ifndef ctob
#define ctob(x) ((x)<<PAGE_SHIFT)
#endif

#if PAGE_SIZE < 2048
#define	MJUMPAGESIZE	MCLBYTES
#elif PAGE_SIZE <= 8192
#define	MJUMPAGESIZE	PAGE_SIZE
#else
#define	MJUMPAGESIZE	(8 * 1024)
#endif

#define	MJUM9BYTES	(9 * 1024)	/* jumbo cluster 9k */
#define	MJUM16BYTES	(16 * 1024)	/* jumbo cluster 16k */

/* bytes to clicks */
#ifndef btoc
#define btoc(x) (((vm_offset_t)(x)+PAGE_MASK)>>PAGE_SHIFT)
#endif

#define dbtoc(db)                       /* calculates devblks to pages */ \
        ((db + (ctodb(1) - 1)) >> (PAGE_SHIFT - DEV_BSHIFT))

#define ctodb(db)                       /* calculates pages to devblks */ \
        ((db) << (PAGE_SHIFT - DEV_BSHIFT))


#endif /* _FREEBSD_PARAM_H_ */
