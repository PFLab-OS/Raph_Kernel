/*
 *
 * Copyright (c) 2017 Raphine Project
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

#include "keyboard.h"
// RAPH_DEBUG
#include <tty.h>
#include <global.h>

DevUsb *DevUsbKeyboard::InitUsb(DevUsbController *controller, int addr) {
  DevUsbKeyboard *that = new DevUsbKeyboard(controller, addr);

  that->LoadDeviceDescriptor();

  that->LoadCombinedDescriptors();
  UsbCtrl::InterfaceDescriptor *interface_desc = that->GetInterfaceDescriptorInCombinedDescriptors(0);
  if (interface_desc->class_code == 3 && interface_desc->protocol_code == 1) {
    gtty->CprintfRaw("kbd: info: usb keyboard detected\n");
    that->Init();
    return that;
  } else {
    delete that;
    return nullptr;
  }
}

void DevUsbKeyboard::InitSub() {
  if (GetDescriptorNumInCombinedDescriptors(UsbCtrl::DescriptorType::kEndpoint) != 1) {
    kernel_panic("DevUsbKeyboard", "not supported");
  }
  UsbCtrl::EndpointDescriptor *ed0 = GetEndpointDescriptorInCombinedDescriptors(0);
  assert(ed0->GetMaxPacketSize() == kMaxPacketSize);
  SetupInterruptTransfer(kTdNum, reinterpret_cast<uint8_t *>(_buffer));

}
