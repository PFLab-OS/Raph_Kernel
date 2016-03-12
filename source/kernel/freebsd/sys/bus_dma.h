#ifndef _FREEBSD_BUS_DMA_H_
#define _FREEBSD_BUS_DMA_H_

#define BUS_DMA_WAITOK    0x00  /* safe to sleep (pseudo-flag) */
#define BUS_DMA_NOWAIT    0x01  /* not safe to sleep */
#define BUS_DMA_ALLOCNOW  0x02  /* perform resource allocation now */
#define BUS_DMA_COHERENT  0x04  /* hint: map memory in a coherent way */
#define BUS_DMA_ZERO    0x08  /* allocate zero'ed memory */
#define BUS_DMA_BUS1    0x10  /* placeholders for bus functions... */
#define BUS_DMA_BUS2    0x20
#define BUS_DMA_BUS3    0x40
#define BUS_DMA_BUS4    0x80

#define BUS_DMA_NOWRITE   0x100
#define BUS_DMA_NOCACHE   0x200


#endif /* _FREEBSD_BUS_DMA_H_ */
