/*-
 * Copyright 1998 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby
 * granted, provided that both the above copyright notice and this
 * permission notice appear in all copies, that both the above
 * copyright notice and this permission notice appear in all
 * supporting documentation, and that the name of M.I.T. not be used
 * in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  M.I.T. makes
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 * 
 * THIS SOFTWARE IS PROVIDED BY M.I.T. ``AS IS''.  M.I.T. DISCLAIMS
 * ALL EXPRESS OR IMPLIED WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT
 * SHALL M.I.T. BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef _FREEBSD_SYS_RMAN_H_
#define	_FREEBSD_SYS_RMAN_H_	1

#ifndef _KERNEL
#include <sys/queue.h>
#else
#include <machine/_bus.h>
#include <machine/resource.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define	RF_ALLOCATED	0x0001	/* resource has been reserved */
#define	RF_ACTIVE	0x0002	/* resource allocation has been activated */
#define	RF_SHAREABLE	0x0004	/* resource permits contemporaneous sharing */
#define	RF_SPARE1	0x0008
#define	RF_SPARE2	0x0010
#define	RF_FIRSTSHARE	0x0020	/* first in sharing list */
#define	RF_PREFETCHABLE	0x0040	/* resource is prefetchable */
#define	RF_OPTIONAL	0x0080	/* for bus_alloc_resources() */

  enum	rman_type { RMAN_UNINIT = 0, RMAN_GAUGE, RMAN_ARRAY };

  /*
   * String length exported to userspace for resource names, etc.
   */
#define RM_TEXTLEN	32

#define	RM_MAX_END	(~(rman_res_t)0)

#define	RMAN_IS_DEFAULT_RANGE(s,e)	((s) == 0 && (e) == RM_MAX_END)

#ifdef _KERNEL

  struct resource_i;

  TAILQ_HEAD(resource_head, resource_i);

  struct rman {
    struct	resource_head 	rm_list;
    struct	mtx *rm_mtx;	/* mutex used to protect rm_list */
    TAILQ_ENTRY(rman)	rm_link; /* link in list of all rmans */
    rman_res_t	rm_start;	/* index of globally first entry */
    rman_res_t	rm_end;	/* index of globally last entry */
    enum	rman_type rm_type; /* what type of resource this is */
    const	char *rm_descr;	/* text descripion of this resource */
  };
  bus_space_tag_t rman_get_bustag(struct resource *r);
  bus_space_handle_t rman_get_bushandle(struct resource *r);

#endif /* _KERNEL */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _FREEBSD_SYS_RMAN_H_ */
