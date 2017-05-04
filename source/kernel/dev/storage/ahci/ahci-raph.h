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

#ifndef __RAPH_KERNEL_DEV_DISK_AHCI_AHCI_RAPH_H__
#define __RAPH_KERNEL_DEV_DISK_AHCI_AHCI_RAPH_H__

#include <raph.h>
#include <queue.h>
#include <sys/types-raph.h>
#include <sys/bus-raph.h>
#include <sys/ata.h>
#include <cam/cam.h>
#include <cam/cam_ccb.h>
#include <ptr.h>
#include <array.h>

class AhciChannel;

class PacketAtaio {
public:
  target_id_t	target_id;
  lun_id_t target_lun;
  xpt_opcode func_code;
  uint32_t flags;
  uint32_t status;
  ccb_spriv_area	sim_priv;
	uint32_t	timeout;

  uint32_t aux;
	uint8_t ata_flags;
  struct ata_res	res;
  struct ata_cmd  cmd;
	uint32_t  resid;
  uint32_t dxfer_len;
  uint8_t *data_ptr;

  uptr<Array<uint8_t>> ptr;

  enum class Status {
    kErr,
    kSuccess,
  };
  Status proc_result;

  static PacketAtaio *XptAlloc() {
    return new PacketAtaio();
  }
  static void Release(PacketAtaio *ataio) {
    // TODO free data_ptr
    delete ataio;
  }
  void XptCopyHeader(PacketAtaio *ataio) {
    target_id = ataio->target_id;
    func_code = ataio->func_code;
    flags = ataio->flags;
    status = ataio->flags;
    sim_priv = ataio->sim_priv;
    timeout = ataio->timeout;
  }
  void XptFree() {
    delete this;
  }
private:
};

class PacketAtaioCtrl {
public:
  static void Init() {
    if (!_is_initailized) {
      new (&_ctrl) PacketAtaioCtrl();
    }
  }
  static PacketAtaioCtrl &GetCtrl() {
    return _ctrl;
  }
private:
  PacketAtaioCtrl() {
  }
  static PacketAtaioCtrl _ctrl;
  static bool _is_initailized;
};

class AhciCtrl : public BsdDevPci {
public:
  AhciCtrl(uint8_t bus, uint8_t device, uint8_t function) : BsdDevPci(bus, device, function) {
    PacketAtaioCtrl::Init();
  }
  static DevPci *InitPci(uint8_t bus, uint8_t device, uint8_t function);
private:
  virtual int DevMethodBusProbe() override final;
  virtual int DevMethodBusAttach() override final;
};

class AhciChannel : public BsdDevBus {
public:
  class DevQueue final : public Functional {
  public:
    DevQueue() {
    }
    ~DevQueue() {
    }
    void Push(PacketAtaio *data) {
      _queue.Push(data);
      WakeupFunction();
    }
    bool Pop(PacketAtaio *&data) {
      return _queue.Pop(data);
    }
    bool IsEmpty() {
      return _queue.IsEmpty();
    }
    void Freeze(int n) {
      Locker locker(lock);
      freeze += n;
    }
    void Release(int n) {
      Locker locker(lock);
      freeze -= n;
      kassert(freeze >= 0);
      if (freeze == 0) {
        WakeupFunction();
      }
    }
  private:
    virtual bool ShouldFunc() override {
      Locker locker(lock);
      return (freeze == 0) && !_queue.IsEmpty();
    }
    Queue2<PacketAtaio *> _queue;
    int freeze = 0;
    SpinLock lock;
  };
  AhciChannel(AhciCtrl *ctrl) : BsdDevBus(ctrl, "ahcich", -1), _ctrl(ctrl) {
    bzero(&ident_data, sizeof(struct ata_params));
  }
  void Identify();
  void Read(int lba, int count);
  void Write(int lba, uptr<Array<uint8_t>> ptr);
  void DonePacket(PacketAtaio *ataio);
  void Handle(void *);
  static AhciChannel *Init(AhciCtrl *ctrl);
  DevQueue	devq;
  PacketAtaio	*frozen = nullptr;
  Queue2<PacketAtaio *>	doneq;
  Queue2<PacketAtaio *>	tmp_doneq;
  bool IsChannelReady() {
    return (ident_data.capabilities1 & (ATA_SUPPORT_DMA | ATA_SUPPORT_LBA)) != 0;
  }
  int GetLogicalSectorSize() {
    kassert(IsChannelReady());
    if ((ident_data.pss & ATA_PSS_VALID_MASK) == ATA_PSS_VALID_VALUE &&
        (ident_data.pss & ATA_PSS_LSSABOVE512) != 0) {
      return (static_cast<uint32_t>(ident_data.lss_1) | (static_cast<u_int32_t>(ident_data.lss_2) << 16)) * 2;
    } else {
      return 512;
    }
  }
  uint32_t GetLogicalSectorNum() {
    if (SupportAddress48()) {
      return static_cast<uint64_t>(ident_data.lba_size48_1) |
        (static_cast<uint64_t>(ident_data.lba_size48_2) << 16) |
        (static_cast<uint64_t>(ident_data.lba_size48_3) << 32) |
        (static_cast<uint64_t>(ident_data.lba_size48_4) << 48);
    } else {
      return static_cast<uint32_t>(ident_data.lba_size_1) |
        (static_cast<uint32_t>(ident_data.lba_size_2) << 16);
    }
  }
private:
  AhciChannel();
  PacketAtaio *MakePacket(uint32_t lba, uint8_t count, uptr<Array<uint8_t>> ptr, uint32_t flags, uint8_t cmd_flags, uint8_t command);
  virtual int DevMethodBusProbe() override final;
  virtual int DevMethodBusAttach() override final;
  virtual int DevMethodBusSetupIntr(struct resource *r, int flags, driver_filter_t filter, driver_intr_t ithread, void *arg, void **cookiep) override final;
  virtual struct resource *DevMethodBusAllocResource(int type, int *rid, rman_res_t start, rman_res_t end, rman_res_t count, u_int flags) override final;
  virtual int DevMethodBusReleaseResource(int type, int rid, struct resource *r) override final;
  AhciCtrl *_ctrl;
  bool SupportAddress48() {
    kassert(IsChannelReady());
    return (ident_data.support.command2 & ATA_SUPPORT_ADDRESS48) != 0;
  }
  struct ata_params ident_data;
};

#endif // __RAPH_KERNEL_DEV_DISK_AHCI_AHCI_RAPH_H__
