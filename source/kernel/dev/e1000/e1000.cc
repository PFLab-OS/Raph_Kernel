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
#include "../../tty.h"
#include "../../global.h"

#include "e1000_api.h"

#include "if_lem.h"

/*********************************************************************
 *  Driver version: this version is specifically for FreeBSD 11 and later
 *********************************************************************/
char em_driver_version[] = "7.6.1";

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
 *  Device initialization routine
 *
 *  The attach entry point is called when the driver is being loaded.
 *  This routine identifies the type of hardware, allocates all resources
 *  and initializes the hardware.
 *
 *  return 0 on success, positive on failure
 *********************************************************************/

static int
lem_attach(E1000 *dev)
{
	struct adapter	*adapter;
	int		tsize, rsize;
	int		error = 0;

	INIT_DEBUGOUT("lem_attach: begin");
        adapter = reinterpret_cast<struct adapter *>(virtmem_ctrl->Alloc(sizeof(struct adapter)));
        dev->adapter = adapter;
	adapter->dev = adapter->osdep.dev = dev;
	EM_CORE_LOCK_INIT(adapter, device_get_nameunit(dev));
	EM_TX_LOCK_INIT(adapter, device_get_nameunit(dev));
	EM_RX_LOCK_INIT(adapter, device_get_nameunit(dev));

	/* SYSCTL stuff */
	/*SYSCTL_ADD_PROC(device_get_sysctl_ctx(dev),
                        SYSCTL_CHILDREN(device_get_sysctl_tree(dev)),
                        OID_AUTO, "nvm", CTLTYPE_INT|CTLFLAG_RW, adapter, 0,
                        lem_sysctl_nvm_info, "I", "NVM Information");*/
          
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
	lem_add_int_delay_sysctl(adapter, "rx_int_delay",
	    "receive interrupt delay in usecs", &adapter->rx_int_delay,
	    E1000_REGISTER(&adapter->hw, E1000_RDTR), lem_rx_int_delay_dflt);
	lem_add_int_delay_sysctl(adapter, "tx_int_delay",
	    "transmit interrupt delay in usecs", &adapter->tx_int_delay,
	    E1000_REGISTER(&adapter->hw, E1000_TIDV), lem_tx_int_delay_dflt);
	if (adapter->hw.mac.type >= e1000_82540) {
		lem_add_int_delay_sysctl(adapter, "rx_abs_int_delay",
		    "receive interrupt delay limit in usecs",
		    &adapter->rx_abs_int_delay,
		    E1000_REGISTER(&adapter->hw, E1000_RADV),
		    lem_rx_abs_int_delay_dflt);
		lem_add_int_delay_sysctl(adapter, "tx_abs_int_delay",
		    "transmit interrupt delay limit in usecs",
		    &adapter->tx_abs_int_delay,
		    E1000_REGISTER(&adapter->hw, E1000_TADV),
		    lem_tx_abs_int_delay_dflt);
		lem_add_int_delay_sysctl(adapter, "itr",
		    "interrupt delay limit in usecs/4",
		    &adapter->tx_itr,
		    E1000_REGISTER(&adapter->hw, E1000_ITR),
		    DEFAULT_ITR);
	}

	/* Sysctls for limiting the amount of work done in the taskqueue */
	lem_add_rx_process_limit(adapter, "rx_processing_limit",
	    "max number of rx packets to process", &adapter->rx_process_limit,
	    lem_rx_process_limit);

#ifdef NIC_SEND_COMBINING
	/* Sysctls to control mitigation */
	lem_add_rx_process_limit(adapter, "sc_enable",
	    "driver TDT mitigation", &adapter->sc_enable, 0);
#endif /* NIC_SEND_COMBINING */
#ifdef BATCH_DISPATCH
	lem_add_rx_process_limit(adapter, "batch_enable",
	    "driver rx batch", &adapter->batch_enable, 0);
#endif /* BATCH_DISPATCH */
#ifdef NIC_PARAVIRT
	lem_add_rx_process_limit(adapter, "rx_retries",
	    "driver rx retries", &adapter->rx_retries, 0);
#endif /* NIC_PARAVIRT */

        /* Sysctl for setting the interface flow control */
	lem_set_flow_cntrl(adapter, "flow_control",
	    "flow control setting",
	    &adapter->fc_setting, lem_fc_setting);

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

#ifdef NIC_PARAVIRT
	device_printf(dev, "driver supports paravirt, subdev 0x%x\n",
		adapter->hw.subsystem_device_id);
	if (adapter->hw.subsystem_device_id == E1000_PARA_SUBDEV) {
		uint64_t bus_addr;

		device_printf(dev, "paravirt support on dev %p\n", adapter);
		tsize = 4096; // XXX one page for the csb
		if (lem_dma_malloc(adapter, tsize, &adapter->csb_mem, BUS_DMA_NOWAIT)) {
			device_printf(dev, "Unable to allocate csb memory\n");
			error = ENOMEM;
			goto err_csb;
		}
		/* Setup the Base of the CSB */
		adapter->csb = (struct paravirt_csb *)adapter->csb_mem.dma_vaddr;
		/* force the first kick */
		adapter->csb->host_need_txkick = 1; /* txring empty */
		adapter->csb->guest_need_rxkick = 1; /* no rx packets */
		bus_addr = adapter->csb_mem.dma_paddr;
		lem_add_rx_process_limit(adapter, "csb_on",
		    "enable paravirt.", &adapter->csb->guest_csb_on, 0);
		lem_add_rx_process_limit(adapter, "txc_lim",
		    "txc_lim", &adapter->csb->host_txcycles_lim, 1);

		/* some stats */
#define PA_SC(name, var, val)		\
	lem_add_rx_process_limit(adapter, name, name, var, val)
		PA_SC("host_need_txkick",&adapter->csb->host_need_txkick, 1);
		PA_SC("host_rxkick_at",&adapter->csb->host_rxkick_at, ~0);
		PA_SC("guest_need_txkick",&adapter->csb->guest_need_txkick, 0);
		PA_SC("guest_need_rxkick",&adapter->csb->guest_need_rxkick, 1);
		PA_SC("tdt_reg_count",&adapter->tdt_reg_count, 0);
		PA_SC("tdt_csb_count",&adapter->tdt_csb_count, 0);
		PA_SC("tdt_int_count",&adapter->tdt_int_count, 0);
		PA_SC("guest_need_kick_count",&adapter->guest_need_kick_count, 0);
		/* tell the host where the block is */
		E1000_WRITE_REG(&adapter->hw, E1000_CSBAH,
			(u32)(bus_addr >> 32));
		E1000_WRITE_REG(&adapter->hw, E1000_CSBAL,
			(u32)bus_addr);
	}
#endif /* NIC_PARAVIRT */

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
	adapter->mta = malloc(sizeof(u8) * ETH_ADDR_LEN *
	    MAX_NUM_MULTICAST_ADDRESSES, M_DEVBUF, M_NOWAIT);
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

	/*
	**  Do interrupt configuration
	*/
	error = lem_allocate_irq(adapter);
	if (error)
		goto err_rx_struct;

	/*
	 * Get Wake-on-Lan and Management info for later use
	 */
	lem_get_wakeup(dev);

	/* Setup OS specific network interface */
	if (lem_setup_interface(dev, adapter) != 0)
		goto err_rx_struct;

	/* Initialize statistics */
	lem_update_stats_counters(adapter);

	adapter->hw.mac.get_link_status = 1;
	lem_update_link_status(adapter);

	/* Indicate SOL/IDER usage */
	if (e1000_check_reset_block(&adapter->hw))
		device_printf(dev,
		    "PHY reset is blocked due to SOL/IDER session.\n");

	/* Do we need workaround for 82544 PCI-X adapter? */
	if (adapter->hw.bus.type == e1000_bus_type_pcix &&
	    adapter->hw.mac.type == e1000_82544)
		adapter->pcix_82544 = TRUE;
	else
		adapter->pcix_82544 = FALSE;

	/* Register for VLAN events */
	/*adapter->vlan_attach = EVENTHANDLER_REGISTER(vlan_config,
	    lem_register_vlan, adapter, EVENTHANDLER_PRI_FIRST);
	adapter->vlan_detach = EVENTHANDLER_REGISTER(vlan_unconfig,
        lem_unregister_vlan, adapter, EVENTHANDLER_PRI_FIRST); */

	lem_add_hw_stats(adapter);

	/* Non-AMT based hardware can now take control from firmware */
	if (adapter->has_manage && !adapter->has_amt)
		lem_get_hw_control(adapter);

	/* Tell the stack that the interface is not active */
	/*if_setdrvflagbits(adapter->ifp, 0, IFF_DRV_OACTIVE | IFF_DRV_RUNNING);*/

	adapter->led_dev = led_create(lem_led_func, adapter,
	    device_get_nameunit(dev));

#ifdef DEV_NETMAP
	lem_netmap_attach(adapter);
#endif /* DEV_NETMAP */
	INIT_DEBUGOUT("lem_attach: end");

	return (0);

err_rx_struct:
	lem_free_transmit_structures(adapter);
err_tx_struct:
err_hw_init:
	lem_release_hw_control(adapter);
	lem_dma_free(adapter, &adapter->rxdma);
err_rx_desc:
	lem_dma_free(adapter, &adapter->txdma);
err_tx_desc:
#ifdef NIC_PARAVIRT
	lem_dma_free(adapter, &adapter->csb_mem);
err_csb:
#endif /* NIC_PARAVIRT */

err_pci:
	if (adapter->ifp != (void *)NULL)
		if_free(adapter->ifp);
	lem_free_pci_resources(adapter);
	free(adapter->mta, M_DEVBUF);
	EM_TX_LOCK_DESTROY(adapter);
	EM_RX_LOCK_DESTROY(adapter);
	EM_CORE_LOCK_DESTROY(adapter);

	return (error);
}


void E1000::InitPCI(uint16_t vid, uint16_t did, uint8_t bus, uint8_t device, bool mf) {
	char		adapter_name[60];
	u16		pci_vendor_id = 0;
	u16		pci_device_id = 0;
	u16		pci_subvendor_id = 0;
	u16		pci_subdevice_id = 0;
	em_vendor_info_t *ent;
        DevPCI pci(bus, device, mf);

	INIT_DEBUGOUT("em_probe: begin");

	pci_vendor_id = vid;
	if (pci_vendor_id != EM_VENDOR_ID)
		return;

	pci_device_id = did;
        kassert((pci.ReadReg<uint8_t>(PCICtrl::kHeaderTypeReg) & PCICtrl::kHeaderTypeRegMaskDeviceType) == PCICtrl::kHeaderTypeRegValueDeviceTypeNormal);
	pci_subvendor_id = pci.ReadReg<uint16_t>(PCICtrl::kSubVendorIdReg);
	pci_subdevice_id = pci.ReadReg<uint16_t>(PCICtrl::kSubsystemIdReg);

	ent = lem_vendor_info_array;
	while (ent->vendor_id != 0) {
		if ((pci_vendor_id == ent->vendor_id) &&
		    (pci_device_id == ent->device_id) &&

		    ((pci_subvendor_id == ent->subvendor_id) ||
		    (ent->subvendor_id == PCI_ANY_ID)) &&

		    ((pci_subdevice_id == ent->subdevice_id) ||
		    (ent->subdevice_id == PCI_ANY_ID))) {
                  /*sprintf(adapter_name, "%s %s",
				lem_strings[ent->index],
				lem_driver_version);
                                device_set_desc_copy(dev, adapter_name);*/
                  E1000 *addr = reinterpret_cast<E1000 *>(virtmem_ctrl->Alloc(sizeof(E1000)));
                  e1000 = new(addr) E1000(bus, device, mf);
                  kassert(lem_attach(e1000) == 0);
			return;
		}
		ent++;
	}
        
	return;
}


void E1000::Handle() {
}
