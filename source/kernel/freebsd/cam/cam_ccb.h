/*-
 * Data structures and definitions for CAM Control Blocks (CCBs).
 *
 * Copyright (c) 1997, 1998 Justin T. Gibbs.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification, immediately at the beginning of the file.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef _CAM_CAM_CCB_H
#define _CAM_CAM_CCB_H 1

#include <sys/queue.h>
#include <sys/cdefs.h>
// #include <sys/time.h>
// #include <sys/limits.h>
#ifndef _KERNEL
#include <sys/callout.h>
#endif
// #include <cam/cam_debug.h>
// #include <cam/scsi/scsi_all.h>
#include <cam/ata/ata_all.h>

/* General allocation length definitions for CCB structures */
#define	IOCDBLEN	CAM_MAX_CDBLEN	/* Space for CDB bytes/pointer */
#define	VUHBALEN	14		/* Vendor Unique HBA length */
#define	SIM_IDLEN	16		/* ASCII string len for SIM ID */
#define	HBA_IDLEN	16		/* ASCII string len for HBA ID */
#define	DEV_IDLEN	16		/* ASCII string len for device names */
#define CCB_PERIPH_PRIV_SIZE 	2	/* size of peripheral private area */
#define CCB_SIM_PRIV_SIZE 	2	/* size of sim private area */

/* Struct definitions for CAM control blocks */

/* Common CCB header */
/* CAM CCB flags */
typedef enum {
	CAM_CDB_POINTER		= 0x00000001,/* The CDB field is a pointer    */
	CAM_QUEUE_ENABLE	= 0x00000002,/* SIM queue actions are enabled */
	CAM_CDB_LINKED		= 0x00000004,/* CCB contains a linked CDB     */
	CAM_NEGOTIATE		= 0x00000008,/*
					      * Perform transport negotiation
					      * with this command.
					      */
	CAM_DATA_ISPHYS		= 0x00000010,/* Data type with physical addrs */
	CAM_DIS_AUTOSENSE	= 0x00000020,/* Disable autosense feature     */
	CAM_DIR_BOTH		= 0x00000000,/* Data direction (00:IN/OUT)    */
	CAM_DIR_IN		= 0x00000040,/* Data direction (01:DATA IN)   */
	CAM_DIR_OUT		= 0x00000080,/* Data direction (10:DATA OUT)  */
	CAM_DIR_NONE		= 0x000000C0,/* Data direction (11:no data)   */
	CAM_DIR_MASK		= 0x000000C0,/* Data direction Mask	      */
	CAM_DATA_VADDR		= 0x00000000,/* Data type (000:Virtual)       */
	CAM_DATA_PADDR		= 0x00000010,/* Data type (001:Physical)      */
	CAM_DATA_SG		= 0x00040000,/* Data type (010:sglist)        */
	CAM_DATA_SG_PADDR	= 0x00040010,/* Data type (011:sglist phys)   */
	CAM_DATA_BIO		= 0x00200000,/* Data type (100:bio)           */
	CAM_DATA_MASK		= 0x00240010,/* Data type mask                */
	CAM_SOFT_RST_OP		= 0x00000100,/* Use Soft reset alternative    */
	CAM_ENG_SYNC		= 0x00000200,/* Flush resid bytes on complete */
	CAM_DEV_QFRZDIS		= 0x00000400,/* Disable DEV Q freezing	      */
	CAM_DEV_QFREEZE		= 0x00000800,/* Freeze DEV Q on execution     */
	CAM_HIGH_POWER		= 0x00001000,/* Command takes a lot of power  */
	CAM_SENSE_PTR		= 0x00002000,/* Sense data is a pointer	      */
	CAM_SENSE_PHYS		= 0x00004000,/* Sense pointer is physical addr*/
	CAM_TAG_ACTION_VALID	= 0x00008000,/* Use the tag action in this ccb*/
	CAM_PASS_ERR_RECOVER	= 0x00010000,/* Pass driver does err. recovery*/
	CAM_DIS_DISCONNECT	= 0x00020000,/* Disable disconnect	      */
	CAM_MSG_BUF_PHYS	= 0x00080000,/* Message buffer ptr is physical*/
	CAM_SNS_BUF_PHYS	= 0x00100000,/* Autosense data ptr is physical*/
	CAM_CDB_PHYS		= 0x00400000,/* CDB poiner is physical	      */
	CAM_ENG_SGLIST		= 0x00800000,/* SG list is for the HBA engine */

/* Phase cognizant mode flags */
	CAM_DIS_AUTOSRP		= 0x01000000,/* Disable autosave/restore ptrs */
	CAM_DIS_AUTODISC	= 0x02000000,/* Disable auto disconnect	      */
	CAM_TGT_CCB_AVAIL	= 0x04000000,/* Target CCB available	      */
	CAM_TGT_PHASE_MODE	= 0x08000000,/* The SIM runs in phase mode    */
	CAM_MSGB_VALID		= 0x10000000,/* Message buffer valid	      */
	CAM_STATUS_VALID	= 0x20000000,/* Status buffer valid	      */
	CAM_DATAB_VALID		= 0x40000000,/* Data buffer valid	      */

/* Host target Mode flags */
	CAM_SEND_SENSE		= 0x08000000,/* Send sense data with status   */
	CAM_TERM_IO		= 0x10000000,/* Terminate I/O Message sup.    */
	CAM_DISCONNECT		= 0x20000000,/* Disconnects are mandatory     */
	CAM_SEND_STATUS		= 0x40000000,/* Send status after data phase  */

	CAM_UNLOCKED		= 0x80000000 /* Call callback without lock.   */
} ccb_flags;

typedef enum {
	CAM_USER_DATA_ADDR	= 0x00000002,/* Userspace data pointers */
	CAM_SG_FORMAT_IOVEC	= 0x00000004,/* iovec instead of busdma S/G*/
	CAM_UNMAPPED_BUF	= 0x00000008 /* use unmapped I/O */
} ccb_xflags;

/* XPT Opcodes for xpt_action */
typedef enum {
/* Function code flags are bits greater than 0xff */
	XPT_FC_QUEUED		= 0x100,
				/* Non-immediate function code */
	XPT_FC_USER_CCB		= 0x200,
	XPT_FC_XPT_ONLY		= 0x400,
				/* Only for the transport layer device */
	XPT_FC_DEV_QUEUED	= 0x800 | XPT_FC_QUEUED,
				/* Passes through the device queues */
/* Common function commands: 0x00->0x0F */
	XPT_NOOP 		= 0x00,
				/* Execute Nothing */
	XPT_SCSI_IO		= 0x01 | XPT_FC_DEV_QUEUED,
				/* Execute the requested I/O operation */
	XPT_GDEV_TYPE		= 0x02,
				/* Get type information for specified device */
	XPT_GDEVLIST		= 0x03,
				/* Get a list of peripheral devices */
	XPT_PATH_INQ		= 0x04,
				/* Path routing inquiry */
	XPT_REL_SIMQ		= 0x05,
				/* Release a frozen device queue */
	XPT_SASYNC_CB		= 0x06,
				/* Set Asynchronous Callback Parameters */
	XPT_SDEV_TYPE		= 0x07,
				/* Set device type information */
	XPT_SCAN_BUS		= 0x08 | XPT_FC_QUEUED | XPT_FC_USER_CCB
				       | XPT_FC_XPT_ONLY,
				/* (Re)Scan the SCSI Bus */
	XPT_DEV_MATCH		= 0x09 | XPT_FC_XPT_ONLY,
				/* Get EDT entries matching the given pattern */
	XPT_DEBUG		= 0x0a,
				/* Turn on debugging for a bus, target or lun */
	XPT_PATH_STATS		= 0x0b,
				/* Path statistics (error counts, etc.) */
	XPT_GDEV_STATS		= 0x0c,
				/* Device statistics (error counts, etc.) */
	XPT_DEV_ADVINFO		= 0x0e,
				/* Get/Set Device advanced information */
	XPT_ASYNC		= 0x0f | XPT_FC_QUEUED | XPT_FC_USER_CCB
				       | XPT_FC_XPT_ONLY,
				/* Asynchronous event */
/* SCSI Control Functions: 0x10->0x1F */
	XPT_ABORT		= 0x10,
				/* Abort the specified CCB */
	XPT_RESET_BUS		= 0x11 | XPT_FC_XPT_ONLY,
				/* Reset the specified SCSI bus */
	XPT_RESET_DEV		= 0x12 | XPT_FC_DEV_QUEUED,
				/* Bus Device Reset the specified SCSI device */
	XPT_TERM_IO		= 0x13,
				/* Terminate the I/O process */
	XPT_SCAN_LUN		= 0x14 | XPT_FC_QUEUED | XPT_FC_USER_CCB
				       | XPT_FC_XPT_ONLY,
				/* Scan Logical Unit */
	XPT_GET_TRAN_SETTINGS	= 0x15,
				/*
				 * Get default/user transfer settings
				 * for the target
				 */
	XPT_SET_TRAN_SETTINGS	= 0x16,
				/*
				 * Set transfer rate/width
				 * negotiation settings
				 */
	XPT_CALC_GEOMETRY	= 0x17,
				/*
				 * Calculate the geometry parameters for
				 * a device give the sector size and
				 * volume size.
				 */
	XPT_ATA_IO		= 0x18 | XPT_FC_DEV_QUEUED,
				/* Execute the requested ATA I/O operation */

	XPT_GET_SIM_KNOB_OLD	= 0x18, /* Compat only */

	XPT_SET_SIM_KNOB	= 0x19,
				/*
				 * Set SIM specific knob values.
				 */

	XPT_GET_SIM_KNOB	= 0x1a,
				/*
				 * Get SIM specific knob values.
				 */

	XPT_SMP_IO		= 0x1b | XPT_FC_DEV_QUEUED,
				/* Serial Management Protocol */

	XPT_SCAN_TGT		= 0x1E | XPT_FC_QUEUED | XPT_FC_USER_CCB
				       | XPT_FC_XPT_ONLY,
				/* Scan Target */

/* HBA engine commands 0x20->0x2F */
	XPT_ENG_INQ		= 0x20 | XPT_FC_XPT_ONLY,
				/* HBA engine feature inquiry */
	XPT_ENG_EXEC		= 0x21 | XPT_FC_DEV_QUEUED,
				/* HBA execute engine request */

/* Target mode commands: 0x30->0x3F */
	XPT_EN_LUN		= 0x30,
				/* Enable LUN as a target */
	XPT_TARGET_IO		= 0x31 | XPT_FC_DEV_QUEUED,
				/* Execute target I/O request */
	XPT_ACCEPT_TARGET_IO	= 0x32 | XPT_FC_QUEUED | XPT_FC_USER_CCB,
				/* Accept Host Target Mode CDB */
	XPT_CONT_TARGET_IO	= 0x33 | XPT_FC_DEV_QUEUED,
				/* Continue Host Target I/O Connection */
	XPT_IMMED_NOTIFY	= 0x34 | XPT_FC_QUEUED | XPT_FC_USER_CCB,
				/* Notify Host Target driver of event (obsolete) */
	XPT_NOTIFY_ACK		= 0x35,
				/* Acknowledgement of event (obsolete) */
	XPT_IMMEDIATE_NOTIFY	= 0x36 | XPT_FC_QUEUED | XPT_FC_USER_CCB,
				/* Notify Host Target driver of event */
	XPT_NOTIFY_ACKNOWLEDGE	= 0x37 | XPT_FC_QUEUED | XPT_FC_USER_CCB,
				/* Acknowledgement of event */

/* Vendor Unique codes: 0x80->0x8F */
	XPT_VUNIQUE		= 0x80
} xpt_opcode;

#define XPT_FC_GROUP_MASK		0xF0
#define XPT_FC_GROUP(op) ((op) & XPT_FC_GROUP_MASK)
#define XPT_FC_GROUP_COMMON		0x00
#define XPT_FC_GROUP_SCSI_CONTROL	0x10
#define XPT_FC_GROUP_HBA_ENGINE		0x20
#define XPT_FC_GROUP_TMODE		0x30
#define XPT_FC_GROUP_VENDOR_UNIQUE	0x80

#define XPT_FC_IS_DEV_QUEUED(ccb) 	\
    (((ccb)->ccb_h.func_code & XPT_FC_DEV_QUEUED) == XPT_FC_DEV_QUEUED)
#define XPT_FC_IS_QUEUED(ccb) 	\
    (((ccb)->ccb_h.func_code & XPT_FC_QUEUED) != 0)

typedef union {
	LIST_ENTRY(ccb_hdr) le;
	SLIST_ENTRY(ccb_hdr) sle;
	TAILQ_ENTRY(ccb_hdr) tqe;
	STAILQ_ENTRY(ccb_hdr) stqe;
} camq_entry;

struct ccb_hdr {
	// cam_pinfo	pinfo;		/* Info for priority scheduling */
	camq_entry	xpt_links;	/* For chaining in the XPT layer */	
	camq_entry	sim_links;	/* For chaining in the SIM layer */	
	camq_entry	periph_links;	/* For chaining in the type driver */
	u_int32_t	retry_count;
	void		(*cbfcnp)(struct cam_periph *, union ccb *);
					/* Callback on completion function */
  xpt_opcode	func_code;	/* XPT function code */
	u_int32_t	status;		/* Status returned by CAM subsystem */
	// struct		cam_path *path;	/* Compiled path for this ccb */
	// path_id_t	path_id;	/* Path ID for the request */
	target_id_t	target_id;	/* Target device ID */
  lun_id_t	target_lun;	/* Target LUN number */
	u_int32_t	flags;		/* ccb_flags */
	u_int32_t	xflags;		/* Extended flags */
	// ccb_ppriv_area	periph_priv;
	// ccb_spriv_area	sim_priv;
	// ccb_qos_area	qos;
	u_int32_t	timeout;	/* Hard timeout value in mseconds */
	// struct timeval	softtimeout;	/* Soft timeout value in sec + usec */
};

/*
 * Union of all CCB types for kernel space allocation.  This union should
 * never be used for manipulating CCBs - its only use is for the allocation
 * and deallocation of raw CCB space and is the return type of xpt_ccb_alloc
 * and the argument to xpt_ccb_free.
 */
union ccb {
	struct	ccb_hdr			ccb_h;	/* For convenience */
	// struct	ccb_scsiio		csio;
	// struct	ccb_getdev		cgd;
	// struct	ccb_getdevlist		cgdl;
	// struct	ccb_pathinq		cpi;
	// struct	ccb_relsim		crs;
	// struct	ccb_setasync		csa;
	// struct	ccb_setdev		csd;
	// struct	ccb_pathstats		cpis;
	// struct	ccb_getdevstats		cgds;
	// struct	ccb_dev_match		cdm;
	// struct	ccb_trans_settings	cts;
	// struct	ccb_calc_geometry	ccg;	
	// struct	ccb_sim_knob		knob;	
	// struct	ccb_abort		cab;
	// struct	ccb_resetbus		crb;
	// struct	ccb_resetdev		crd;
	// struct	ccb_termio		tio;
	// struct	ccb_accept_tio		atio;
	// struct	ccb_scsiio		ctio;
	// struct	ccb_en_lun		cel;
	// struct	ccb_immed_notify	cin;
	// struct	ccb_notify_ack		cna;
	// struct	ccb_immediate_notify	cin1;
	// struct	ccb_notify_acknowledge	cna2;
	// struct	ccb_eng_inq		cei;
	// struct	ccb_eng_exec		cee;
	// struct	ccb_smpio		smpio;
	// struct 	ccb_rescan		crcn;
	// struct  ccb_debug		cdbg;
	// struct	ccb_ataio		ataio;
	// struct	ccb_dev_advinfo		cdai;
	// struct	ccb_async		casync;
};

#endif /* _CAM_CAM_CCB_H */
