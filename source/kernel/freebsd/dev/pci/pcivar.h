/*-
 * Copyright (c) 1997, Stefan Esser <se@freebsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 *
 */

#ifndef _PCIVAR_H_
#define	_PCIVAR_H_

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  int pci_msi_count(device_t dev);
  int pci_msix_count(device_t dev);
  int pci_alloc_msi(device_t dev, int *count);
  int pci_alloc_msix(device_t dev, int *count);
  int pci_release_msi(device_t dev);

  uint16_t pci_get_vendor(device_t dev);
  uint16_t pci_get_device(device_t dev);
  uint32_t pci_get_devid(device_t dev);
  uint8_t pci_get_class(device_t dev);
  uint8_t pci_get_subclass(device_t dev);
  uint8_t pci_get_progif(device_t dev);
  uint8_t pci_get_revid(device_t dev);
  uint16_t pci_get_subvendor(device_t dev);
  uint16_t pci_get_subdevice(device_t dev);
  void pci_write_config(device_t dev, int reg, uint32_t val, int width);
  uint32_t pci_read_config(device_t dev, int reg, int width);
  int pci_enable_busmaster(device_t dev);
  int pci_find_cap(device_t dev, int capability, int *capreg);

#ifdef __cplusplus
}
#endif /* __cplusplus */
  
#endif /* _PCIVAR_H_ */
