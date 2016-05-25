/*-
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	From: @(#)if.h	8.1 (Berkeley) 6/10/93
 * $FreeBSD$
 */

#ifndef _FREEBSD_NET_IF_VAR_H_
#define _FREEBSD_NET_IF_VAR_H_

#include <mem/virtmem.h>
#include <global.h>
#include <dev/netdev.h>
#include <freebsd/net/if.h>
#include <freebsd/net/ethernet.h>

struct ifnet {
  int	if_drv_flags;
  void *if_softc;
  int if_capenable;
  int if_capabilities;
};
typedef ifnet *if_t;

struct BsdDevice;

// Raphineでは未定義
// static inline if_t if_gethandle(uint8_t type);

static inline int if_setdrvflagbits(if_t ifp, int set_flags, int clear_flags)
{
  ifp->if_drv_flags |= set_flags;
  ifp->if_drv_flags &= ~clear_flags;

  return (0);
}

static inline int if_getdrvflags(if_t ifp) {
  return ifp->if_drv_flags;
}

static inline int if_setdrvflags(if_t ifp, int flags) {
  ifp->if_drv_flags = flags;
  return (0);
}

static inline void *if_getsoftc(if_t ifp) {
  return ifp->if_softc;
}

static inline int if_setsoftc(if_t ifp, void *softc)
{
  ifp->if_softc = softc;
  return (0);
}

static inline int if_getmtu(if_t ifp) {
  return ETHERMTU;
}

static inline int if_setcapabilities(if_t ifp, int capabilities)
{
  ifp->if_capabilities = capabilities;
  return (0);
}

static inline int if_setcapabilitiesbit(if_t ifp, int setbit, int clearbit)
{
  ifp->if_capabilities |= setbit;
  ifp->if_capabilities &= ~clearbit;

  return (0);
}

static inline int if_getcapabilities(if_t ifp)
{
  return ifp->if_capabilities;
}

static inline int if_getcapenable(if_t ifp) {
  return ifp->if_capenable;
}

static inline int if_setcapenablebit(if_t ifp, int setcap, int clearcap) {
  if(setcap) {
    ifp->if_capenable |= setcap;
  }
  if(clearcap) {
    ifp->if_capenable &= ~clearcap;
  }

  return 0;
}

static inline int if_setcapenable(if_t ifp, int capenable) {
  ifp->if_capenable = capenable;
  return (0);
}

static inline int if_setdev(if_t ifp, void *dev) {
  return 0;
}

#endif /* _FREEBSD_NET_IF_VAR_H_ */
