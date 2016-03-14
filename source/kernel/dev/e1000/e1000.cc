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

#include <stdint.h>
#include "e1000.h"
#include "../../timer.h"
#include "../../mem/paging.h"
#include "../../mem/virtmem.h"
#include "../../mem/physmem.h"
#include "../../tty.h"
#include "../../global.h"

#include "e1000_raph.h"
#include "e1000_api.h"
#include "e1000_osdep.h"
#include "./if_lem.h"

/*********************************************************************
 *  Legacy Em Driver version:
 *********************************************************************/
char lem_driver_version[] = "1.1.0";

/*********************************************************************
 *  PCI Device ID Table
 *
 *  Used by probe to select devices to load on
 *  Last field stores an index into e1000_strings
 *  Last entry must be all 0s
 *
 *  { Vendor ID, Device ID, SubVendor ID, SubDevice ID, String Index }
 *********************************************************************/

static em_vendor_info_t lem_vendor_info_array[] =
  {
    /* Intel(R) PRO/1000 Network Connection */
    { 0x8086, E1000_DEV_ID_82540EM,		PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82540EM_LOM,	PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82540EP,		PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82540EP_LOM,	PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82540EP_LP,	PCI_ANY_ID, PCI_ANY_ID, 0},

    { 0x8086, E1000_DEV_ID_82541EI,		PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82541ER,		PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82541ER_LOM,	PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82541EI_MOBILE,	PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82541GI,		PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82541GI_LF,	PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82541GI_MOBILE,	PCI_ANY_ID, PCI_ANY_ID, 0},

    { 0x8086, E1000_DEV_ID_82542,		PCI_ANY_ID, PCI_ANY_ID, 0},

    { 0x8086, E1000_DEV_ID_82543GC_FIBER,	PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82543GC_COPPER,	PCI_ANY_ID, PCI_ANY_ID, 0},

    { 0x8086, E1000_DEV_ID_82544EI_COPPER,	PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82544EI_FIBER,	PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82544GC_COPPER,	PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82544GC_LOM,	PCI_ANY_ID, PCI_ANY_ID, 0},

    { 0x8086, E1000_DEV_ID_82545EM_COPPER,	PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82545EM_FIBER,	PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82545GM_COPPER,	PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82545GM_FIBER,	PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82545GM_SERDES,	PCI_ANY_ID, PCI_ANY_ID, 0},

    { 0x8086, E1000_DEV_ID_82546EB_COPPER,	PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82546EB_FIBER,	PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82546EB_QUAD_COPPER, PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82546GB_COPPER,	PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82546GB_FIBER,	PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82546GB_SERDES,	PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82546GB_PCIE,	PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82546GB_QUAD_COPPER, PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82546GB_QUAD_COPPER_KSP3,
      PCI_ANY_ID, PCI_ANY_ID, 0},

    { 0x8086, E1000_DEV_ID_82547EI,		PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82547EI_MOBILE,	PCI_ANY_ID, PCI_ANY_ID, 0},
    { 0x8086, E1000_DEV_ID_82547GI,		PCI_ANY_ID, PCI_ANY_ID, 0},
    /* required last entry */
    { 0, 0, 0, 0, 0}
  };

/*********************************************************************
 *  Table of branding strings for all supported NICs.
 *********************************************************************/

static const char *lem_strings[] = {
  "Intel(R) PRO/1000 Legacy Network Connection"
};

/*********************************************************************
 *  Function prototypes
 *********************************************************************/
static int	lem_probe(device_t);
// static int	lem_attach(device_t);
// static int	lem_detach(device_t);
// static int	lem_shutdown(device_t);
// static int	lem_suspend(device_t);
// static int	lem_resume(device_t);
// static void	lem_start(if_t);
// static void	lem_start_locked(if_t ifp);
// static int	lem_ioctl(if_t, u_long, caddr_t);
// #if __FreeBSD_version >= 1100036
// static uint64_t	lem_get_counter(if_t, ift_counter);
// #endif
// static void	lem_init(void *);
// static void	lem_init_locked(struct adapter *);
// static void	lem_stop(void *);
// static void	lem_media_status(if_t, struct ifmediareq *);
// static int	lem_media_change(if_t);
static void	lem_identify_hardware(struct adapter *);
static int	lem_allocate_pci_resources(struct adapter *);
// static int	lem_allocate_irq(struct adapter *adapter);
// static void	lem_free_pci_resources(struct adapter *);
// static void	lem_local_timer(void *);
static int	lem_hardware_init(struct adapter *);
// static int	lem_setup_interface(device_t, struct adapter *);
// static void	lem_setup_transmit_structures(struct adapter *);
// static void	lem_initialize_transmit_unit(struct adapter *);
// static int	lem_setup_receive_structures(struct adapter *);
// static void	lem_initialize_receive_unit(struct adapter *);
// static void	lem_enable_intr(struct adapter *);
// static void	lem_disable_intr(struct adapter *);
// static void	lem_free_transmit_structures(struct adapter *);
// static void	lem_free_receive_structures(struct adapter *);
// static void	lem_update_stats_counters(struct adapter *);
// static void	lem_add_hw_stats(struct adapter *adapter);
// static void	lem_txeof(struct adapter *);
// static void	lem_tx_purge(struct adapter *);
static int	lem_allocate_receive_structures(struct adapter *);
static int	lem_allocate_transmit_structures(struct adapter *);
// static bool	lem_rxeof(struct adapter *, int, int *);
// #ifndef __NO_STRICT_ALIGNMENT
// static int	lem_fixup_rx(struct adapter *);
// #endif
// static void	lem_receive_checksum(struct adapter *, struct e1000_rx_desc *,
// 		    struct mbuf *);
// static void	lem_transmit_checksum_setup(struct adapter *, struct mbuf *,
// 		    u32 *, u32 *);
// static void	lem_set_promisc(struct adapter *);
// static void	lem_disable_promisc(struct adapter *);
// static void	lem_set_multi(struct adapter *);
// static void	lem_update_link_status(struct adapter *);
// static int	lem_get_buf(struct adapter *, int);
// static void	lem_register_vlan(void *, if_t, u16);
// static void	lem_unregister_vlan(void *, if_t, u16);
// static void	lem_setup_vlan_hw_support(struct adapter *);
// static int	lem_xmit(struct adapter *, struct mbuf **);
// static void	lem_smartspeed(struct adapter *);
// static int	lem_82547_fifo_workaround(struct adapter *, int);
// static void	lem_82547_update_fifo_head(struct adapter *, int);
// static int	lem_82547_tx_fifo_reset(struct adapter *);
// static void	lem_82547_move_tail(void *);
static int	lem_dma_malloc(struct adapter *, bus_size_t,
                               struct em_dma_alloc *, int);
// static void	lem_dma_free(struct adapter *, struct em_dma_alloc *);
// static int	lem_sysctl_nvm_info(SYSCTL_HANDLER_ARGS);
// static void	lem_print_nvm_info(struct adapter *);
static int 	lem_is_valid_ether_addr(u8 *);
// static u32	lem_fill_descriptors (bus_addr_t address, u32 length,
// 		    PDESC_ARRAY desc_array);
// static int	lem_sysctl_int_delay(SYSCTL_HANDLER_ARGS);
// static void	lem_add_int_delay_sysctl(struct adapter *, const char *,
// 		    const char *, struct em_int_delay_info *, int, int);
// static void	lem_set_flow_cntrl(struct adapter *, const char *,
// 		    const char *, int *, int);
// /* Management and WOL Support */
// static void	lem_init_manageability(struct adapter *);
// static void	lem_release_manageability(struct adapter *);
// static void     lem_get_hw_control(struct adapter *);
// static void     lem_release_hw_control(struct adapter *);
// static void	lem_get_wakeup(device_t);
// static void     lem_enable_wakeup(device_t);
// static int	lem_enable_phy_wakeup(struct adapter *);
// static void	lem_led_func(void *, int);

// static void	lem_intr(void *);
// static int	lem_irq_fast(void *);
// static void	lem_handle_rxtx(void *context, int pending);
// static void	lem_handle_link(void *context, int pending);
// static void	lem_add_rx_process_limit(struct adapter *, const char *,
// 		    const char *, int *, int);

/*********************************************************************
 *  Tunable default values.
 *********************************************************************/

#define EM_TICKS_TO_USECS(ticks)	((1024 * (ticks) + 500) / 1000)
#define EM_USECS_TO_TICKS(usecs)	((1000 * (usecs) + 512) / 1024)

#define MAX_INTS_PER_SEC	8000
#define DEFAULT_ITR		(1000000000/(MAX_INTS_PER_SEC * 256))

static int lem_tx_int_delay_dflt = EM_TICKS_TO_USECS(EM_TIDV);
static int lem_rx_int_delay_dflt = EM_TICKS_TO_USECS(EM_RDTR);
static int lem_tx_abs_int_delay_dflt = EM_TICKS_TO_USECS(EM_TADV);
static int lem_rx_abs_int_delay_dflt = EM_TICKS_TO_USECS(EM_RADV);
/*
 * increase lem_rxd and lem_txd to at least 2048 in netmap mode
 * for better performance.
 */
static int lem_rxd = EM_DEFAULT_RXD;
static int lem_txd = EM_DEFAULT_TXD;
static int lem_smart_pwr_down = FALSE;

/* Controls whether promiscuous also shows bad packets */
static int lem_debug_sbp = FALSE;

TUNABLE_INT("hw.em.tx_int_delay", &lem_tx_int_delay_dflt);
TUNABLE_INT("hw.em.rx_int_delay", &lem_rx_int_delay_dflt);
TUNABLE_INT("hw.em.tx_abs_int_delay", &lem_tx_abs_int_delay_dflt);
TUNABLE_INT("hw.em.rx_abs_int_delay", &lem_rx_abs_int_delay_dflt);
TUNABLE_INT("hw.em.rxd", &lem_rxd);
TUNABLE_INT("hw.em.txd", &lem_txd);
TUNABLE_INT("hw.em.smart_pwr_down", &lem_smart_pwr_down);
TUNABLE_INT("hw.em.sbp", &lem_debug_sbp);

/* Interrupt style - default to fast */
static int lem_use_legacy_irq = 0;
TUNABLE_INT("hw.em.use_legacy_irq", &lem_use_legacy_irq);

/* How many packets rxeof tries to clean at a time */
static int lem_rx_process_limit = 100;
TUNABLE_INT("hw.em.rx_process_limit", &lem_rx_process_limit);

/* Flow control setting - default to FULL */
static int lem_fc_setting = e1000_fc_full;
TUNABLE_INT("hw.em.fc_setting", &lem_fc_setting);

/* Global used in WOL setup with multiport cards */
static int global_quad_port_a = 0;

static int
lem_probe(device_t dev)
{
  char		adapter_name[60];
  u16		pci_vendor_id = 0;
  u16		pci_device_id = 0;
  u16		pci_subvendor_id = 0;
  u16		pci_subdevice_id = 0;
  em_vendor_info_t *ent;

  INIT_DEBUGOUT("em_probe: begin");

  pci_vendor_id = pci_get_vendor(dev);
  if (pci_vendor_id != EM_VENDOR_ID)
    return (ENXIO);

  pci_device_id = pci_get_device(dev);
  pci_subvendor_id = pci_get_subvendor(dev);
  pci_subdevice_id = pci_get_subdevice(dev);

  ent = lem_vendor_info_array;
  while (ent->vendor_id != 0) {
    if ((pci_vendor_id == ent->vendor_id) &&
        (pci_device_id == ent->device_id) &&

        ((pci_subvendor_id == ent->subvendor_id) ||
         (ent->subvendor_id == PCI_ANY_ID)) &&

        ((pci_subdevice_id == ent->subdevice_id) ||
         (ent->subdevice_id == PCI_ANY_ID))) {
      sprintf(adapter_name, "%s %s",
              lem_strings[ent->index],
              lem_driver_version);
      device_set_desc_copy(dev, adapter_name);
      return (BUS_PROBE_DEFAULT);
    }
    ent++;
  }

  return (ENXIO);
}

/*********************************************************************
 *  Device initialization routine
 *
 *  The attach entry point is called when the driver is being loaded.
 *  This routine identifies the type of hardware, allocates all resources
 *  and initializes the hardware.
 *
 *  return 0 on success, positive on failure
 *********************************************************************/

static int
lem_attach(device_t dev)
{
  struct adapter	*adapter;
  int		tsize, rsize;
  int		error = 0;

  INIT_DEBUGOUT("lem_attach: begin");

  adapter = reinterpret_cast<struct adapter *>(virtmem_ctrl->Alloc(sizeof(adapter)));
  new(&adapter->timer.callout) Callout;
  new(&adapter->tx_fifo_timer.callout) Callout;
  new(&adapter->core_mtx.lock) SpinLock;
  new(&adapter->tx_mtx.lock) SpinLock;
  new(&adapter->rx_mtx.lock) SpinLock;
  adapter->dev = adapter->osdep.dev = dev;
  EM_CORE_LOCK_INIT(adapter, device_get_nameunit(dev));
  EM_TX_LOCK_INIT(adapter, device_get_nameunit(dev));
  EM_RX_LOCK_INIT(adapter, device_get_nameunit(dev));

  /* SYSCTL stuff */
  SYSCTL_ADD_PROC(device_get_sysctl_ctx(dev),
                  SYSCTL_CHILDREN(device_get_sysctl_tree(dev)),
                  OID_AUTO, "nvm", CTLTYPE_INT|CTLFLAG_RW, adapter, 0,
                  lem_sysctl_nvm_info, "I", "NVM Information");

  callout_init_mtx(&adapter->timer, &adapter->core_mtx, 0);
  callout_init_mtx(&adapter->tx_fifo_timer, &adapter->tx_mtx, 0);

  /* Determine hardware and mac info */
  lem_identify_hardware(adapter);

  /* Setup PCI resources */
  if (lem_allocate_pci_resources(adapter)) {
    device_printf(dev, "Allocation of PCI resources failed\n");
    error = ENXIO;
    goto err_pci;
  }

  /* Do Shared Code initialization */
  if (e1000_setup_init_funcs(&adapter->hw, TRUE)) {
    device_printf(dev, "Setup of Shared code failed\n");
    error = ENXIO;
    goto err_pci;
  }

  e1000_get_bus_info(&adapter->hw);

  /* Set up some sysctls for the tunable interrupt delays */
  // lem_add_int_delay_sysctl(adapter, "rx_int_delay",
  //                          "receive interrupt delay in usecs", &adapter->rx_int_delay,
  //                          E1000_REGISTER(&adapter->hw, E1000_RDTR), lem_rx_int_delay_dflt);
  // 	lem_add_int_delay_sysctl(adapter, "tx_int_delay",
  // 	    "transmit interrupt delay in usecs", &adapter->tx_int_delay,
  // 	    E1000_REGISTER(&adapter->hw, E1000_TIDV), lem_tx_int_delay_dflt);

  // 	if (adapter->hw.mac.type >= e1000_82540) {
  // 		lem_add_int_delay_sysctl(adapter, "rx_abs_int_delay",
  // 		    "receive interrupt delay limit in usecs",
  // 		    &adapter->rx_abs_int_delay,
  // 		    E1000_REGISTER(&adapter->hw, E1000_RADV),
  // 		    lem_rx_abs_int_delay_dflt);
  // 		lem_add_int_delay_sysctl(adapter, "tx_abs_int_delay",
  // 		    "transmit interrupt delay limit in usecs",
  // 		    &adapter->tx_abs_int_delay,
  // 		    E1000_REGISTER(&adapter->hw, E1000_TADV),
  // 		    lem_tx_abs_int_delay_dflt);
  // 		lem_add_int_delay_sysctl(adapter, "itr",
  // 		    "interrupt delay limit in usecs/4",
  // 		    &adapter->tx_itr,
  // 		    E1000_REGISTER(&adapter->hw, E1000_ITR),
  // 		    DEFAULT_ITR);
  // 	}

  /* Sysctls for limiting the amount of work done in the taskqueue */
  // 	lem_add_rx_process_limit(adapter, "rx_processing_limit",
  // 	    "max number of rx packets to process", &adapter->rx_process_limit,
  // 	    lem_rx_process_limit);

  // #ifdef NIC_SEND_COMBINING
  // 	/* Sysctls to control mitigation */
  // 	lem_add_rx_process_limit(adapter, "sc_enable",
  // 	    "driver TDT mitigation", &adapter->sc_enable, 0);
  // #endif /* NIC_SEND_COMBINING */
  // #ifdef BATCH_DISPATCH
  // 	lem_add_rx_process_limit(adapter, "batch_enable",
  // 	    "driver rx batch", &adapter->batch_enable, 0);
  // #endif /* BATCH_DISPATCH */
  // #ifdef NIC_PARAVIRT
  // 	lem_add_rx_process_limit(adapter, "rx_retries",
  // 	    "driver rx retries", &adapter->rx_retries, 0);
  // #endif /* NIC_PARAVIRT */

  //         /* Sysctl for setting the interface flow control */
  // 	lem_set_flow_cntrl(adapter, "flow_control",
  // 	    "flow control setting",
  // 	    &adapter->fc_setting, lem_fc_setting);

  /*
   * Validate number of transmit and receive descriptors. It
   * must not exceed hardware maximum, and must be multiple
   * of E1000_DBA_ALIGN.
   */
  if (((lem_txd * sizeof(struct e1000_tx_desc)) % EM_DBA_ALIGN) != 0 ||
      (adapter->hw.mac.type >= e1000_82544 && lem_txd > EM_MAX_TXD) ||
      (adapter->hw.mac.type < e1000_82544 && lem_txd > EM_MAX_TXD_82543) ||
      (lem_txd < EM_MIN_TXD)) {
    device_printf(dev, "Using %d TX descriptors instead of %d!\n",
                  EM_DEFAULT_TXD, lem_txd);
    adapter->num_tx_desc = EM_DEFAULT_TXD;
  } else
    adapter->num_tx_desc = lem_txd;
  if (((lem_rxd * sizeof(struct e1000_rx_desc)) % EM_DBA_ALIGN) != 0 ||
      (adapter->hw.mac.type >= e1000_82544 && lem_rxd > EM_MAX_RXD) ||
      (adapter->hw.mac.type < e1000_82544 && lem_rxd > EM_MAX_RXD_82543) ||
      (lem_rxd < EM_MIN_RXD)) {
    device_printf(dev, "Using %d RX descriptors instead of %d!\n",
                  EM_DEFAULT_RXD, lem_rxd);
    adapter->num_rx_desc = EM_DEFAULT_RXD;
  } else
    adapter->num_rx_desc = lem_rxd;

  adapter->hw.mac.autoneg = DO_AUTO_NEG;
  adapter->hw.phy.autoneg_wait_to_complete = FALSE;
  adapter->hw.phy.autoneg_advertised = AUTONEG_ADV_DEFAULT;
  adapter->rx_buffer_len = 2048;

  e1000_init_script_state_82541(&adapter->hw, TRUE);
  e1000_set_tbi_compatibility_82543(&adapter->hw, TRUE);

  /* Copper options */
  if (adapter->hw.phy.media_type == e1000_media_type_copper) {
    adapter->hw.phy.mdix = AUTO_ALL_MODES;
    adapter->hw.phy.disable_polarity_correction = FALSE;
    adapter->hw.phy.ms_type = EM_MASTER_SLAVE;
  }

  /*
   * Set the frame limits assuming
   * standard ethernet sized frames.
   */
  adapter->max_frame_size = ETHERMTU + ETHER_HDR_LEN + ETHERNET_FCS_SIZE;
  adapter->min_frame_size = ETH_ZLEN + ETHERNET_FCS_SIZE;

  /*
   * This controls when hardware reports transmit completion
   * status.
   */
  adapter->hw.mac.report_tx_early = 1;

  // #ifdef NIC_PARAVIRT
  // 	device_printf(dev, "driver supports paravirt, subdev 0x%x\n",
  // 		adapter->hw.subsystem_device_id);
  // 	if (adapter->hw.subsystem_device_id == E1000_PARA_SUBDEV) {
  // 		uint64_t bus_addr;

  // 		device_printf(dev, "paravirt support on dev %p\n", adapter);
  // 		tsize = 4096; // XXX one page for the csb
  // 		if (lem_dma_malloc(adapter, tsize, &adapter->csb_mem, BUS_DMA_NOWAIT)) {
  // 			device_printf(dev, "Unable to allocate csb memory\n");
  // 			error = ENOMEM;
  // 			goto err_csb;
  // 		}
  // 		/* Setup the Base of the CSB */
  // 		adapter->csb = (struct paravirt_csb *)adapter->csb_mem.dma_vaddr;
  // 		/* force the first kick */
  // 		adapter->csb->host_need_txkick = 1; /* txring empty */
  // 		adapter->csb->guest_need_rxkick = 1; /* no rx packets */
  // 		bus_addr = adapter->csb_mem.dma_paddr;
  // 		lem_add_rx_process_limit(adapter, "csb_on",
  // 		    "enable paravirt.", &adapter->csb->guest_csb_on, 0);
  // 		lem_add_rx_process_limit(adapter, "txc_lim",
  // 		    "txc_lim", &adapter->csb->host_txcycles_lim, 1);

  // 		/* some stats */
  // #define PA_SC(name, var, val)		\
  // 	lem_add_rx_process_limit(adapter, name, name, var, val)
  // 		PA_SC("host_need_txkick",&adapter->csb->host_need_txkick, 1);
  // 		PA_SC("host_rxkick_at",&adapter->csb->host_rxkick_at, ~0);
  // 		PA_SC("guest_need_txkick",&adapter->csb->guest_need_txkick, 0);
  // 		PA_SC("guest_need_rxkick",&adapter->csb->guest_need_rxkick, 1);
  // 		PA_SC("tdt_reg_count",&adapter->tdt_reg_count, 0);
  // 		PA_SC("tdt_csb_count",&adapter->tdt_csb_count, 0);
  // 		PA_SC("tdt_int_count",&adapter->tdt_int_count, 0);
  // 		PA_SC("guest_need_kick_count",&adapter->guest_need_kick_count, 0);
  // 		/* tell the host where the block is */
  // 		E1000_WRITE_REG(&adapter->hw, E1000_CSBAH,
  // 			(u32)(bus_addr >> 32));
  // 		E1000_WRITE_REG(&adapter->hw, E1000_CSBAL,
  // 			(u32)bus_addr);
  // 	}
  // #endif /* NIC_PARAVIRT */

  tsize = roundup2(adapter->num_tx_desc * sizeof(struct e1000_tx_desc),
                   EM_DBA_ALIGN);

  /* Allocate Transmit Descriptor ring */
  if (lem_dma_malloc(adapter, tsize, &adapter->txdma, BUS_DMA_NOWAIT)) {
    device_printf(dev, "Unable to allocate tx_desc memory\n");
    error = ENOMEM;
    goto err_tx_desc;
  }
  adapter->tx_desc_base = 
    (struct e1000_tx_desc *)adapter->txdma.dma_vaddr;

  rsize = roundup2(adapter->num_rx_desc * sizeof(struct e1000_rx_desc),
                   EM_DBA_ALIGN);

  /* Allocate Receive Descriptor ring */
  if (lem_dma_malloc(adapter, rsize, &adapter->rxdma, BUS_DMA_NOWAIT)) {
    device_printf(dev, "Unable to allocate rx_desc memory\n");
    error = ENOMEM;
    goto err_rx_desc;
  }
  adapter->rx_desc_base =
    (struct e1000_rx_desc *)adapter->rxdma.dma_vaddr;

  /* Allocate multicast array memory. */
  adapter->mta = reinterpret_cast<u8 *>(virtmem_ctrl->Alloc(sizeof(u8) * ETH_ADDR_LEN *
                                                            MAX_NUM_MULTICAST_ADDRESSES));
  if (adapter->mta == NULL) {
    device_printf(dev, "Can not allocate multicast setup array\n");
    error = ENOMEM;
    goto err_hw_init;
  }

  /*
  ** Start from a known state, this is
  ** important in reading the nvm and
  ** mac from that.
  */
  e1000_reset_hw(&adapter->hw);

  /* Make sure we have a good EEPROM before we read from it */
  if (e1000_validate_nvm_checksum(&adapter->hw) < 0) {
    /*
    ** Some PCI-E parts fail the first check due to
    ** the link being in sleep state, call it again,
    ** if it fails a second time its a real issue.
    */
    if (e1000_validate_nvm_checksum(&adapter->hw) < 0) {
      device_printf(dev,
                    "The EEPROM Checksum Is Not Valid\n");
      error = EIO;
      goto err_hw_init;
    }
  }

  /* Copy the permanent MAC address out of the EEPROM */
  if (e1000_read_mac_addr(&adapter->hw) < 0) {
    device_printf(dev, "EEPROM read error while reading MAC"
                  " address\n");
    error = EIO;
    goto err_hw_init;
  }

  if (!lem_is_valid_ether_addr(adapter->hw.mac.addr)) {
    device_printf(dev, "Invalid MAC address\n");
    error = EIO;
    goto err_hw_init;
  }

  /* Initialize the hardware */
  if (lem_hardware_init(adapter)) {
    device_printf(dev, "Unable to initialize the hardware\n");
    error = EIO;
    goto err_hw_init;
  }

  /* Allocate transmit descriptors and buffers */
  if (lem_allocate_transmit_structures(adapter)) {
    device_printf(dev, "Could not setup transmit structures\n");
    error = ENOMEM;
    goto err_tx_struct;
  }

  /* Allocate receive descriptors and buffers */
  if (lem_allocate_receive_structures(adapter)) {
    device_printf(dev, "Could not setup receive structures\n");
    error = ENOMEM;
    goto err_rx_struct;
  }

  // 	/*
  // 	**  Do interrupt configuration
  // 	*/
  // 	error = lem_allocate_irq(adapter);
  // 	if (error)
  // 		goto err_rx_struct;

  // 	/*
  // 	 * Get Wake-on-Lan and Management info for later use
  // 	 */
  // 	lem_get_wakeup(dev);

  // 	/* Setup OS specific network interface */
  // 	if (lem_setup_interface(dev, adapter) != 0)
  // 		goto err_rx_struct;

  // 	/* Initialize statistics */
  // 	lem_update_stats_counters(adapter);

  // 	adapter->hw.mac.get_link_status = 1;
  // 	lem_update_link_status(adapter);

  // 	/* Indicate SOL/IDER usage */
  // 	if (e1000_check_reset_block(&adapter->hw))
  // 		device_printf(dev,
  // 		    "PHY reset is blocked due to SOL/IDER session.\n");

  // 	/* Do we need workaround for 82544 PCI-X adapter? */
  // 	if (adapter->hw.bus.type == e1000_bus_type_pcix &&
  // 	    adapter->hw.mac.type == e1000_82544)
  // 		adapter->pcix_82544 = TRUE;
  // 	else
  // 		adapter->pcix_82544 = FALSE;

  // 	/* Register for VLAN events */
  // 	adapter->vlan_attach = EVENTHANDLER_REGISTER(vlan_config,
  // 	    lem_register_vlan, adapter, EVENTHANDLER_PRI_FIRST);
  // 	adapter->vlan_detach = EVENTHANDLER_REGISTER(vlan_unconfig,
  // 	    lem_unregister_vlan, adapter, EVENTHANDLER_PRI_FIRST); 

  // 	lem_add_hw_stats(adapter);

  // 	/* Non-AMT based hardware can now take control from firmware */
  // 	if (adapter->has_manage && !adapter->has_amt)
  // 		lem_get_hw_control(adapter);

  // 	/* Tell the stack that the interface is not active */
  // 	if_setdrvflagbits(adapter->ifp, 0, IFF_DRV_OACTIVE | IFF_DRV_RUNNING);

  // 	adapter->led_dev = led_create(lem_led_func, adapter,
  // 	    device_get_nameunit(dev));

  // #ifdef DEV_NETMAP
  // 	lem_netmap_attach(adapter);
  // #endif /* DEV_NETMAP */
  // 	INIT_DEBUGOUT("lem_attach: end");

  // 	return (0);

  err_rx_struct:
  //  	lem_free_transmit_structures(adapter);
  err_tx_struct:
  err_hw_init:
  //	lem_release_hw_control(adapter);
  //	lem_dma_free(adapter, &adapter->rxdma);
  err_rx_desc:
  //	lem_dma_free(adapter, &adapter->txdma);
  err_tx_desc:
  // #ifdef NIC_PARAVIRT
  // 	lem_dma_free(adapter, &adapter->csb_mem);
  // err_csb:
  // #endif /* NIC_PARAVIRT */

  err_pci:
  // 	if (adapter->ifp != (void *)NULL)
  // 		if_free(adapter->ifp);
  // 	lem_free_pci_resources(adapter);
  // 	free(adapter->mta, M_DEVBUF);
  // 	EM_TX_LOCK_DESTROY(adapter);
  // 	EM_RX_LOCK_DESTROY(adapter);
  // 	EM_CORE_LOCK_DESTROY(adapter);

  return (error);
}

/*********************************************************************
 *
 *  Determine hardware revision.
 *
 **********************************************************************/
static void
lem_identify_hardware(struct adapter *adapter)
{
  device_t dev = adapter->dev;

  /* Make sure our PCI config space has the necessary stuff set */
  pci_enable_busmaster(dev);
  adapter->hw.bus.pci_cmd_word = pci_read_config(dev, PCIR_COMMAND, 2);

  /* Save off the information about this board */
  adapter->hw.vendor_id = pci_get_vendor(dev);
  adapter->hw.device_id = pci_get_device(dev);
  adapter->hw.revision_id = pci_read_config(dev, PCIR_REVID, 1);
  adapter->hw.subsystem_vendor_id =
    pci_read_config(dev, PCIR_SUBVEND_0, 2);
  adapter->hw.subsystem_device_id =
    pci_read_config(dev, PCIR_SUBDEV_0, 2);

  /* Do Shared Code Init and Setup */
  if (e1000_set_mac_type(&adapter->hw)) {
    device_printf(dev, "Setup init failure\n");
    return;
  }
}

static int
lem_allocate_pci_resources(struct adapter *adapter)
{
  device_t	dev = adapter->dev;
  int		val, rid, error = E1000_SUCCESS;

  rid = PCIR_BAR(0);
  adapter->memory = bus_alloc_resource_from_bar(dev, rid);
  if (adapter->memory == NULL) {
    device_printf(dev, "Unable to allocate bus resource: memory\n");
    return (ENXIO);
  }
  adapter->osdep.mem_bus_space_tag = 
    rman_get_bustag(adapter->memory);
  adapter->osdep.mem_bus_space_handle =
    rman_get_bushandle(adapter->memory);
  adapter->hw.hw_addr = (u8 *)&adapter->osdep.mem_bus_space_handle;

  /* Only older adapters use IO mapping */
  if (adapter->hw.mac.type > e1000_82543) {
    /* Figure our where our IO BAR is ? */
    for (rid = PCIR_BAR(0); rid < PCIR_CIS;) {
      val = pci_read_config(dev, rid, 4);
      if (EM_BAR_TYPE(val) == EM_BAR_TYPE_IO) {
        adapter->io_rid = rid;
        break;
      }
      rid += 4;
      /* check for 64bit BAR */
      if (EM_BAR_MEM_TYPE(val) == EM_BAR_MEM_TYPE_64BIT)
        rid += 4;
    }
    if (rid >= PCIR_CIS) {
      device_printf(dev, "Unable to locate IO BAR\n");
      return (ENXIO);
    }
    adapter->ioport = bus_alloc_resource_from_bar(dev, adapter->io_rid);
    if (adapter->ioport == NULL) {
      device_printf(dev, "Unable to allocate bus resource: "
                    "ioport\n");
      return (ENXIO);
    }
    adapter->hw.io_base = 0;
    adapter->osdep.io_bus_space_tag =
      rman_get_bustag(adapter->ioport);
    adapter->osdep.io_bus_space_handle =
      rman_get_bushandle(adapter->ioport);
  }
        
  adapter->hw.back = &adapter->osdep;

  return (error);
}

static int
lem_dma_malloc(struct adapter *adapter, bus_size_t size,
        struct em_dma_alloc *dma, int mapflags)
{
  PhysAddr paddr;
  physmem_ctrl->Alloc(paddr, PagingCtrl::RoundUpAddrOnPageBoundary(size));
  dma->dma_paddr = reinterpret_cast<bus_addr_t>(paddr.GetAddr());
  dma->dma_vaddr = reinterpret_cast<caddr_t>(p2v(paddr.GetAddr()));
  
  return (0);
}

static int
lem_is_valid_ether_addr(u8 *addr)
{
	char zero_addr[6] = { 0, 0, 0, 0, 0, 0 };

	if ((addr[0] & 1) || (!bcmp(addr, zero_addr, ETHER_ADDR_LEN))) {
		return (FALSE);
	}

	return (TRUE);
}

/*********************************************************************
 *
 *  Initialize the hardware to a configuration
 *  as specified by the adapter structure.
 *
 **********************************************************************/
static int
lem_hardware_init(struct adapter *adapter)
{
	device_t dev = adapter->dev;
	u16 	rx_buffer_size;

	INIT_DEBUGOUT("lem_hardware_init: begin");

	/* Issue a global reset */
	e1000_reset_hw(&adapter->hw);

	/* When hardware is reset, fifo_head is also reset */
	adapter->tx_fifo_head = 0;

	/*
	 * These parameters control the automatic generation (Tx) and
	 * response (Rx) to Ethernet PAUSE frames.
	 * - High water mark should allow for at least two frames to be
	 *   received after sending an XOFF.
	 * - Low water mark works best when it is very near the high water mark.
	 *   This allows the receiver to restart by sending XON when it has
	 *   drained a bit. Here we use an arbitary value of 1500 which will
	 *   restart after one full frame is pulled from the buffer. There
	 *   could be several smaller frames in the buffer and if so they will
	 *   not trigger the XON until their total number reduces the buffer
	 *   by 1500.
	 * - The pause time is fairly large at 1000 x 512ns = 512 usec.
	 */
	rx_buffer_size = ((E1000_READ_REG(&adapter->hw, E1000_PBA) &
	    0xffff) << 10 );

	adapter->hw.fc.high_water = rx_buffer_size -
	    roundup2(adapter->max_frame_size, 1024);
	adapter->hw.fc.low_water = adapter->hw.fc.high_water - 1500;

	adapter->hw.fc.pause_time = EM_FC_PAUSE_TIME;
	adapter->hw.fc.send_xon = TRUE;

        /* Set Flow control, use the tunable location if sane */
        if ((lem_fc_setting >= 0) && (lem_fc_setting < 4))
                adapter->hw.fc.requested_mode = lem_fc_setting;
        else
                adapter->hw.fc.requested_mode = e1000_fc_none;

	if (e1000_init_hw(&adapter->hw) < 0) {
		device_printf(dev, "Hardware Initialization Failed\n");
		return (EIO);
	}

	e1000_check_for_link(&adapter->hw);

	return (0);
}

/*********************************************************************
 *
 *  Allocate memory for tx_buffer structures. The tx_buffer stores all
 *  the information needed to transmit a packet on the wire.
 *
 **********************************************************************/
static int
lem_allocate_transmit_structures(struct adapter *adapter)
{
	device_t dev = adapter->dev;
	struct em_buffer *tx_buffer;
	int error;

	adapter->tx_buffer_area = reinterpret_cast<struct em_buffer *>(virtmem_ctrl->Alloc(sizeof(struct em_buffer) * adapter->num_tx_desc);)
	if (adapter->tx_buffer_area == NULL) {
		device_printf(dev, "Unable to allocate tx_buffer memory\n");
		error = ENOMEM;
		goto fail;
	}

	/* Create the descriptor buffer dma maps */
	for (int i = 0; i < adapter->num_tx_desc; i++) {
          tx_buffer = &adapter->tx_buffer_area[i];

          PhysAddr paddr;
          physmem_ctrl->Alloc(paddr, PagingCtrl::RoundUpAddrOnPageBoundary(MCLBYTES));
          tx_buffer->map = paddr.GetAddr();
	}

	return (0);
fail:
        //	lem_free_transmit_structures(adapter);
	return (error);
}

/*********************************************************************
 *
 *  Allocate memory for rx_buffer structures. Since we use one
 *  rx_buffer per received packet, the maximum number of rx_buffer's
 *  that we'll need is equal to the number of receive descriptors
 *  that we've allocated.
 *
 **********************************************************************/
static int
lem_allocate_receive_structures(struct adapter *adapter)
{
	device_t dev = adapter->dev;
	struct em_buffer *rx_buffer;
	int i, error;

	adapter->rx_buffer_area = reinterpret_cast<struct em_buffer *>(virtmem_ctrl->Alloc(sizeof(struct em_buffer) * adapter->num_rx_desc));
	if (adapter->rx_buffer_area == NULL) {
		device_printf(dev, "Unable to allocate rx_buffer memory\n");
		return (ENOMEM);
	}

	error = bus_dma_tag_create(bus_get_dma_tag(dev), /* parent */
				1, 0,			/* alignment, bounds */
				BUS_SPACE_MAXADDR,	/* lowaddr */
				BUS_SPACE_MAXADDR,	/* highaddr */
				NULL, NULL,		/* filter, filterarg */
				MCLBYTES,		/* maxsize */
				1,			/* nsegments */
				MCLBYTES,		/* maxsegsize */
				0,			/* flags */
				NULL,			/* lockfunc */
				NULL,			/* lockarg */
				&adapter->rxtag);
	if (error) {
		device_printf(dev, "%s: bus_dma_tag_create failed %d\n",
		    __func__, error);
		goto fail;
	}

	/* Create the spare map (used by getbuf) */
        PhysAddr paddr_spare;
        physmem_ctrl->Alloc(paddr_spare, PagingCtrl::RoundUpAddrOnPageBoundary(MCLBYTES));
        adapter->rx_sparemap = paddr_spare.GetAddr();

	rx_buffer = adapter->rx_buffer_area;
	for (i = 0; i < adapter->num_rx_desc; i++, rx_buffer++) {
          PhysAddr paddr;
          physmem_ctrl->Alloc(paddr, PagingCtrl::RoundUpAddrOnPageBoundary(MCLBYTES));
          rx_buffer->map = paddr.GetAddr();
	}

	return (0);

fail:
        //	lem_free_receive_structures(adapter);
	return (error);
}

void E1000::InitPCI(uint16_t vid, uint16_t did, uint8_t bus, uint8_t device, bool mf) {
  E1000 *addr = reinterpret_cast<E1000 *>(virtmem_ctrl->Alloc(sizeof(E1000)));
  addr = new(addr) E1000(bus, device, mf);
  addr->bsd.parent = addr;
  if (lem_probe(&addr->bsd) == BUS_PROBE_DEFAULT) {
    kassert(lem_attach(&addr->bsd) == 0);
  } else {
    virtmem_ctrl->Free(ptr2virtaddr(addr));
  }  
}


void E1000::Handle() {
}
