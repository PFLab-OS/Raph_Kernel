/*
 *
 * Copyright (c) 2015 Raphine Project
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

#include <raph.h>
#include <task.h>
#include <cpu.h>
#include <global.h>

void membench4(sptr<TaskWithStack> task);
void membench2(sptr<TaskWithStack> task);
void membench3(sptr<TaskWithStack> task);

void register_membench2_callout() {
  for (int i = 0; i < cpu_ctrl->GetHowManyCpus(); i++) {
    CpuId cpuid(i);
    
    auto task_ = make_sptr(new TaskWithStack(cpuid));
    task_->Init();
    task_->SetFunc(make_uptr(new Function<sptr<TaskWithStack>>([](sptr<TaskWithStack> task){
            membench4(task);
            // membench2(task);
            // membench3(task);
          }, task_)));
    task_ctrl->Register(cpuid, task_);
  }
}

