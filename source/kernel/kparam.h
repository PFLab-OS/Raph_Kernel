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

#include <string.h>

class KernelParamCtrl {
public:
  void Init();

  enum class ErrorState {
    kOk,
    kNotFound,
    kAlreadyExist,
    kProtected,
    kBufShortage,
  };
  ErrorState GetKernelParamValue(const char* name,char* buf,size_t len);

  ErrorState ChangeKernelParamValue(const char* name,const char* val);

  ErrorState CreateKernelParam(const char* name,const char* pval,bool is_protected = false);

  static KernelParamCtrl& GetCtrl() {
    static KernelParamCtrl instance;
    return instance;
  }

  static const int kMaxNameLength = 64;
  static const int kMaxValueLength = 64;

private:
  KernelParamCtrl() {
    _list_head = new Param("dummy","dummy",true);
  }
  ~KernelParamCtrl() {
  }

  class Param {
  public:
    Param() = delete;
    //TODO:TBC naming rule
    Param(const char* pname,const char* pval,bool is_protected = false) : protect(is_protected) {
      strncpy(value,pval,kMaxValueLength);
      strncpy(name,pname,kMaxNameLength);
    }

    char name[kMaxNameLength];
    char value[kMaxValueLength];
    Param* next = nullptr;
    const bool protect;
  };
  
  Param* _list_head;

};
