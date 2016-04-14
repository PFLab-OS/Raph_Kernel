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

#include <functional.h>
#include <task.h>

void Functional::WakeupFunction() {
  // TODO : CPUを叩きおこす
  Locker locker(_lock);
  if (_state == FunctionState::kNotFunctioning && _func.CanExecute()) {
    _state = FunctionState::kFunctioning;
    Function func;
    func.Init(Handle, reinterpret_cast<void *>(this));
    task_ctrl->Register(_apicid, func);
  }
}

void Functional::Handle(void *p) {
  Functional *that = reinterpret_cast<Functional *>(p);
  while(true) {
    // ロック時間を最低限にするため、Emptyでない間はLockせずにループさせる
    if (that->ShouldFunc()) {
      that->_func.Execute();
    } else {
      Locker locker(that->_lock);
      if (!that->ShouldFunc()) {
        that->_state = FunctionState::kNotFunctioning;
        break;
      }
    }
  }
}

void Functional::SetFunction(int apicid, const Function &func) {
  kassert(!_func.CanExecute());
  _apicid = apicid;
  _func = func;
}
