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

#ifndef __RAPH_KERNEL_BUF_H__
#define __RAPH_KERNEL_BUF_H__
  
#include <stddef.h>
#include <raph.h>
#include <mem/virtmem.h>
#include <global.h>
#include <spinlock.h>
#include <task.h>
#include <functional.h>

class ListBuffer {
public:
  static ListBuffer *Create(size_t size) {
    ListBuffer *buf = reinterpret_cast<ListBuffer *>(virtmem_ctrl->Alloc(sizeof(ListBuffer)));
    buf->_addr = buf->Alloc(size);
    buf->_size = size;
    buf->_next = nullptr;
    buf->_before = nullptr;
    buf->_head = 0;
    buf->_len = size;
    return buf;
  }
  static void Destroy(ListBuffer *buf) {
    if (buf->_before == nullptr) {
      while (buf->_next != nullptr) {
        ListBuffer::Remove(buf->_next);
      }
      ListBuffer::Remove(buf);
    } else {
      ListBuffer::Destroy(buf->_before);
    }
  }
  static void Remove(ListBuffer *buf) {
    if (buf->_before != nullptr) {
      buf->_before->_next = buf->_next;
    }
    if (buf->_next != nullptr) {
      buf->_next->_before = buf->_before;
    }
    buf->Release();
    virtmem_ctrl->Free(reinterpret_cast<virt_addr>(buf));
  }
  void *GetAddr() {
    return _addr;
  }
  void CutHead(size_t len) {
    if (len > _len) {
      if (_next != nullptr) {
        _next->CutHead(len - _len);
      }
      len = _len;
    }
    _head += len;
  }
protected:
  virtual void *Alloc(size_t size) {
    return reinterpret_cast<void *>(virtmem_ctrl->Alloc(size));
  }
  virtual void Release() {
    virtmem_ctrl->Free(reinterpret_cast<virt_addr>(_addr));
  }
private:
  ListBuffer();
  ~ListBuffer();
  ListBuffer *_next;
  ListBuffer *_before;
  void *_addr;
  size_t _size;  // バッファ長（管理用）
  size_t _head;  // 有効領域先頭
  size_t _len;  // 有効領域長
};

template<class T, int S>
class RingBuffer {
 public:
  RingBuffer() {
    _head = 0;
    _tail = 0;
  }
  virtual ~RingBuffer() {
  }
  // 満杯の時は何もせず、falseを返す
  bool Push(T data) {
    Locker locker(_lock);
    int ntail = (_tail + 1) % S;
    if (ntail != _head) {
      _buffer[_tail] = data;
      _tail = ntail;
      return true;
    } else {
      return false;
    }
  }
  // 空の時は何もせず、falseを返す
  bool Pop(T &data) {
    Locker locker(_lock);
    if (_head != _tail) {
      data = _buffer[_head];
      _head = (_head + 1) % S;
      return true;
    } else {
      return false;
    }
  }
  bool IsFull() {
    Locker locker(_lock);
    int ntail = (_tail + 1) % S;
    return (ntail == _head);
  }
  bool IsEmpty() {
    Locker locker(_lock);
    return (_tail == _head);
  }
 private:
  T _buffer[S];
  int _head;
  int _tail;
  SpinLock _lock;
};

template<class T, int S>
  class FunctionalRingBuffer final : public Functional {
 public:
  FunctionalRingBuffer() {
  }
  ~FunctionalRingBuffer() {
  }
  bool Push(T data) {
    bool flag = _buf.Push(data);
    WakeupFunction();
    return flag;
  }
  bool Pop(T &data) {
    return _buf.Pop(data);
  }
  bool IsFull() {
    return _buf.IsFull();
  }
  bool IsEmpty() {
    return _buf.IsEmpty();
  }
 private:
  virtual bool ShouldFunc() override {
    return !_buf.IsEmpty();
  }
  RingBuffer<T, S> _buf;
};


#endif /* __RAPH_KERNEL_BUF_H__ */
