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
 * Author: mumumu
 *
 */

#include <kparam.h>
#include <string.h>
#include <tty.h>


void KernelParamCtrl::Init() {
  char buf[256];
  GetKernelParamValue("dummy",buf,256);
  gtty->Printf(buf);
  CreateKernelParam("test","aa");
  GetKernelParamValue("test",buf,256);
  gtty->Printf(buf);
  ChangeKernelParamValue("test","IIIII");
  GetKernelParamValue("test",buf,256);
  gtty->Printf(buf);

}

KernelParamCtrl::ErrorState KernelParamCtrl::GetKernelParamValue(const char* name,char* buf,size_t len) {
  Param* param = _list_head;
  while(param != nullptr) {
    if(strcmp(param->name,name) == 0) { 
      if(len >= strlen(param->name)) {
          strncpy(buf,param->value,len);
          return KernelParamCtrl::ErrorState::kOk;
      }
      return KernelParamCtrl::ErrorState::kBufShortage;
    }
    param = param->next;
  }
  return KernelParamCtrl::ErrorState::kNotFound;
}

KernelParamCtrl::ErrorState KernelParamCtrl::ChangeKernelParamValue(const char* name,const char* val) {
  Param* param = _list_head;
  while(param != nullptr) {
    if(strcmp(param->name,name) == 0) { 
        if(param->protect == false) {
          strncpy(param->value,val,kMaxValueLength);
          return KernelParamCtrl::ErrorState::kOk;
        }
        gtty->Printf("dq");
        return KernelParamCtrl::ErrorState::kProtected;
    }
    param = param->next;
        gtty->Printf("d1");
  }
  return KernelParamCtrl::ErrorState::kNotFound;
}


KernelParamCtrl::ErrorState KernelParamCtrl::CreateKernelParam(const char* name,const char* val,bool is_protected) {
  Param* tmp;
  Param* param = _list_head;
  while(param != nullptr) {
    if(strcmp(param->name,name) == 0) {
      return KernelParamCtrl::ErrorState::kAlreadyExist;
    }
    tmp = param;
    param = param->next;
  }

  tmp->next = new Param(name,val,is_protected);
  tmp->next->next = nullptr;

  return KernelParamCtrl::ErrorState::kOk;
}
