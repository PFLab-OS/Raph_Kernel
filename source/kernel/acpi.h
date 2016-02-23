/*
 *
 * Copyright (c) 2015 Project Raphine
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

#ifndef __RAPH_KERNEL_ACPI_H__
#define __RAPH_KERNEL_ACPI_H__

#include <stdint.h>
#include <string.h>
#include "global.h"
#include "mem/physmem.h"

// see acpi spec

// RSDP v1
struct RSDPDescriptor {
  char Signature[8];
  uint8_t Checksum;
  char OEMID[6];
  uint8_t Revision;
  uint32_t RsdtAddress;
} __attribute__ ((packed));

// RSDP v2
struct RSDPDescriptor20 {
  RSDPDescriptor firstPart;
  uint32_t Length;
  uint64_t XsdtAddress;
  uint8_t ExtendedChecksum;
  uint8_t reserved[3];
} __attribute__ ((packed));

struct ACPISDTHeader {
  char Signature[4];
  uint32_t Length;
  uint8_t Revision;
  uint8_t Checksum;
  char OEMID[6];
  char OEMTableID[8];
  uint32_t OEMRevision;
  uint32_t CreatorID;
  uint32_t CreatorRevision;
} __attribute__ ((packed));

struct FADT {
  ACPISDTHeader header;
  uint32_t FirmwareCtrl;
  uint32_t DSDTAddr;
  uint8_t Reserved1;
  uint8_t PreferredPmProfile;
  uint16_t SciInt;
  uint32_t SmiCmd;
  uint8_t AcpiEnable;
  uint8_t AcpiDisable;
  uint8_t S4BiosReq;
  uint8_t PsateCnt;
  uint32_t Pm1aEvtBlk;
  uint32_t Pm1bEvtBlk;
  uint32_t Pm1aCntBlk;
  uint32_t Pm1bCntBlk;
  uint32_t Pm2CntBlk;
  uint32_t PmTmrBlk;
  uint32_t Gpe0Blk;
  uint32_t Gpe1Blk;
  uint8_t Pm1EvtLen;
  uint8_t Pm1CntLen;
  uint8_t Pm2CntLen;
  uint8_t PmTmrLen;
  uint8_t Gpe0BlkLen;
  uint8_t Gpe1BlkLen;
  uint8_t Gpe1Base;
  uint8_t CstCnt;
  uint16_t PLvl2Lat;
  uint16_t PLvl3Lat;
  uint16_t FlushSize;
  uint16_t FlushStride;
  uint8_t DutyOffset;
  uint8_t DutyWidth;
  uint8_t DayAlrm;
  uint8_t MonAlrm;
  uint8_t Century;
  uint16_t IapcBootArch;
  uint8_t Reserved2;
  uint32_t Flags;
  uint32_t ResetReg1;
  uint32_t ResetReg2;
  uint32_t ResetReg3;
  uint8_t ResetValue;
  uint8_t Reserved3;
  uint8_t Reserved4;
  uint8_t Reserved5;
  uint64_t XFirmwareCtrl;
  uint64_t XDsdt;
  uint32_t XPm1aEvtBlk1;
  uint32_t XPm1aEvtBlk2;
  uint32_t XPm1aEvtBlk3;
  uint32_t XPm1bEvtBlk1;
  uint32_t XPm1bEvtBlk2;
  uint32_t XPm1bEvtBlk3;
  uint32_t XPm1aCntBlk1;
  uint32_t XPm1aCntBlk2;
  uint32_t XPm1aCntBlk3;
  uint32_t XPm1bCntBlk1;
  uint32_t XPm1bCntBlk2;
  uint32_t XPm1bCntBlk3;
  uint32_t XPm2CntBlk1;
  uint32_t XPm2CntBlk2;
  uint32_t XPm2CntBlk3;
  uint32_t XPmTmrBlk1;
  uint32_t XPmTmrBlk2;
  uint32_t XPmTmrBlk3;
  uint32_t XGpe0Blk1;
  uint32_t XGpe0Blk2;
  uint32_t XGpe0Blk3;
  uint32_t XGpe1Blk1;
  uint32_t XGpe1Blk2;
  uint32_t XGpe1Blk3;
  uint32_t SleepCtrlReg1;
  uint32_t SleepCtrlReg2;
  uint32_t SleepCtrlReg3;
  uint32_t SleepStatusReg1;
  uint32_t SleepStatusReg2;
  uint32_t SleepStatusReg3;
} __attribute__ ((packed));

struct MCFG;
struct HPETDT;

class AcpiCtrl {
public:
  AcpiCtrl() {}
  void Setup(RSDPDescriptor *rsdp);
  void Setup(RSDPDescriptor20 *rsdp) {
    Setup(&rsdp->firstPart);
  }
  MCFG *GetMCFG() {
    return _mcfg;
  }
  HPETDT *GetHPETDT() {
    return _hpetdt;
  }
  FADT *GetFADT() {
    return _fadt;
  }
private:
  int CheckACPISDTHeader(ACPISDTHeader *header) {
    uint8_t sum = 0;
    uint8_t *byte = reinterpret_cast<uint8_t *>(header);
    for (uint32_t i = 0; i < header->Length; i++, byte++) {
      sum += *byte;
    }
    return (sum == 0);
  }
  MCFG *_mcfg = nullptr;
  HPETDT *_hpetdt = nullptr;
  FADT *_fadt = nullptr;
};

#endif /* __RAPH_KERNEL_ACPI_H__ */
