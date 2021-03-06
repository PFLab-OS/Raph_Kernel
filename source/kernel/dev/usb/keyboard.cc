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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * Author: Liva
 *
 */

#include "keyboard.h"
#include <tty.h>
#include <global.h>

DevUsb *DevUsbKeyboard::InitUsb(DevUsbController *controller, int addr) {
  DevUsbKeyboard *that = new DevUsbKeyboard(controller, addr);

  that->LoadDeviceDescriptor();

  that->LoadCombinedDescriptors();
  UsbCtrl::InterfaceDescriptor *interface_desc =
      that->GetInterfaceDescriptorInCombinedDescriptors(0);

  if (interface_desc->class_code == 3 && interface_desc->subclass_code == 1 &&
      interface_desc->protocol_code == 1) {
    gtty->Printf("kbd: info: usb keyboard detected\n");
    that->Init();
    return that;
  } else {
    delete that;
    return nullptr;
  }
}

void DevUsbKeyboard::InitSub() {
  // if
  // (GetDescriptorNumInCombinedDescriptors(UsbCtrl::DescriptorType::kEndpoint)
  // != 1) {
  //   kernel_panic("DevUsbKeyboard", "not supported");
  // }
  UsbCtrl::EndpointDescriptor *ed0 =
      GetEndpointDescriptorInCombinedDescriptors(0);
  assert(ed0->GetMaxPacketSize() == kMaxPacketSize);
  for (int i = 0; i < kTdNum; i++) {
    memset(_buffer[i].buf, i, kMaxPacketSize);
  }
  SetupInterruptTransfer(
      kTdNum, reinterpret_cast<uint8_t *>(_buffer),
      make_uptr(new ClassFunction1<void, DevUsbKeyboard, void *,
                                   uptr<Array<uint8_t>>>(
          this, &DevUsbKeyboard::Handle, nullptr)));

  while (true) {
    UsbCtrl::DeviceRequest *request = nullptr;

    bool retry = true;

    do {
      if (!UsbCtrl::GetCtrl().AllocDeviceRequest(request)) {
        break;
      }

      // Set_Protocol
      // see HID1_11 7.2.6 Set_Protocol Request
      request->MakePacket(0b00100001, 0x0B, 0, 0, 0);

      if (!SendControlTransfer(request, 0, 0)) {
        break;
      }

      retry = false;
    } while (0);

    if (request != nullptr) {
      assert(UsbCtrl::GetCtrl().ReuseDeviceRequest(request));
    }

    if (!retry) {
      break;
    }
  }

  _dev.Setup();
}

void DevUsbKeyboard::Handle(void *, uptr<Array<uint8_t>> buf) {
  bool same = true;
  for (int i = 2; i < 8; i++) {
    if ((*buf)[i] != _prev_buf[i]) {
      same = false;
    }
  }

  for (int i = 2; i < 8; i++) {
    int c = (*buf)[i];
    if (c != 0) {
      bool exist = false;
      for (int j = 0; j < 8; j++) {
        if (c == _prev_buf[j]) {
          exist = true;
        }
      }
      if (same || !exist) {
        // TODO support multiple input
        // TODO support special keys
        Keyboard::KeyInfo ki;
        for (int k = 0; k < Keyboard::kKeyBufSize; k++) {
          ki.c[k] = 0;
        }
        ki.c[0] = kScanCode[c];
        _dev.Push(ki);
      }
    }
  }

  for (int i = 0; i < 8; i++) {
    _prev_buf[i] = (*buf)[i];
  }
}

const char DevUsbKeyboard::kScanCode[256] = {
    '!',  '!',  '!', '!',  'a',  'b', 'c', 'd', 'e', 'f', 'g',  'h', 'i',
    'j',  'k',  'l', 'm',  'n',  'o', 'p', 'q', 'r', 's', 't',  'u', 'v',
    'w',  'x',  'y', 'z',  '1',  '2', '3', '4', '5', '6', '7',  '8', '9',
    '0',  '\n', '!', '\b', '\t', ' ', '-', '=', '[', ']', '\\', '!', ';',
    '\'', '!',  ',', '.',  '/',  '!', '!', '!', '!', '!', '!',  '!', '!',
    '!',  '!',  '!', '!',  '!',  '!', '!', '!', '!', '!', '!',  '!', '!',
    '!',  '!',  '!', '!',  '!',  '!', '!', '!', '!', '!', '!',  '!', '!',
    '!',  '!',  '!', '!',  '!',  '!', '!', '!', '!', '!', '!',  '!', '!',
    '!',  '!',  '!', '!',  '!',  '!', '!', '!', '!', '!', '!',  '!', '!',
    '!',  '!',  '!', '!',  '!',  '!', '!', '!', '!', '!', '!',
};
