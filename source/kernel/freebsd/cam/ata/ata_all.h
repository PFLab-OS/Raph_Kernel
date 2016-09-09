/*-
 * Copyright (c) 2009 Alexander Motin <mav@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer,
 *    without modification, immediately at the beginning of the file.
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
 */

#ifndef	CAM_ATA_ALL_H
#define CAM_ATA_ALL_H 1

#include <sys/ata.h>

struct ccb_ataio;
struct cam_periph;
union  ccb;

#define	SID_DMA48	0x01 /* Abuse inq_flags bit to track enabled DMA48. */
#define	SID_AEN		0x04 /* Abuse inq_flags bit to track enabled AEN. */
#define	SID_DMA		0x10 /* Abuse inq_flags bit to track enabled DMA. */

struct ata_cmd {
	u_int8_t	flags;		/* ATA command flags */
#define		CAM_ATAIO_48BIT		0x01	/* Command has 48-bit format */
#define		CAM_ATAIO_FPDMA		0x02	/* FPDMA command */
#define		CAM_ATAIO_CONTROL	0x04	/* Control, not a command */
#define		CAM_ATAIO_NEEDRESULT	0x08	/* Request requires result. */
#define		CAM_ATAIO_DMA		0x10	/* DMA command */

	u_int8_t	command;
	u_int8_t	features;

	u_int8_t	lba_low;
	u_int8_t	lba_mid;
	u_int8_t	lba_high;
	u_int8_t	device;

	u_int8_t	lba_low_exp;
	u_int8_t	lba_mid_exp;
	u_int8_t	lba_high_exp;
	u_int8_t	features_exp;

	u_int8_t	sector_count;
	u_int8_t	sector_count_exp;
	u_int8_t	control;
};

struct ata_res {
	u_int8_t	flags;		/* ATA command flags */
#define		CAM_ATAIO_48BIT		0x01	/* Command has 48-bit format */

	u_int8_t	status;
	u_int8_t	error;

	u_int8_t	lba_low;
	u_int8_t	lba_mid;
	u_int8_t	lba_high;
	u_int8_t	device;

	u_int8_t	lba_low_exp;
	u_int8_t	lba_mid_exp;
	u_int8_t	lba_high_exp;

	u_int8_t	sector_count;
	u_int8_t	sector_count_exp;
};

struct sep_identify_data {
	uint8_t		length;		/* Enclosure descriptor length */
	uint8_t		subenc_id;	/* Sub-enclosure identifier */
	uint8_t		logical_id[8];	/* Enclosure logical identifier (WWN) */
	uint8_t		vendor_id[8];	/* Vendor identification string */
	uint8_t		product_id[16];	/* Product identification string */
	uint8_t		product_rev[4];	/* Product revision string */
	uint8_t		channel_id;	/* Channel identifier */
	uint8_t		firmware_rev[4];/* Firmware revision */
	uint8_t		interface_id[6];/* Interface spec ("S-E-S "/"SAF-TE")*/
	uint8_t		interface_rev[4];/* Interface spec revision */
	uint8_t		vend_spec[11];	/* Vendor specific information */
};

#endif
