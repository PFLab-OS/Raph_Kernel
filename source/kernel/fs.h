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
 * UNIX v6 file system driver (inspired from xv6 source code)
 * 
 */

#pragma once

#include <stdint.h>
#include <string.h>
#include <dev/storage/storage.h>
#include <assert.h>
#include <spinlock.h>

class DiskFileSystem;

//TODO replace spinlock to job qeueue

using InodeNumber = uint32_t;

struct Stat {
  enum class Type : int16_t {
    kDirectory = 1,
    kFile = 2,
    kDevice = 3,
  };
  Type type;
  
  int32_t dev;
  InodeNumber ino;
  int16_t nlink;
  uint32_t size;
};

class InodeContainer;

class Inode {
public:
  void Init(DiskFileSystem *fs) {
    _fs = fs;
  }
  bool IsEmpty() {
    return _ref == 0;
  }
private:
  friend InodeContainer;
  SpinLock _lock;
  int _ref = 0;
  int _flags;
  DiskFileSystem *_fs = nullptr;
  InodeNumber _inum;
};

class InodeContainer;

class VirtualFileSystem {
public:
  VirtualFileSystem() = delete;
  VirtualFileSystem(DiskFileSystem *dfs) : _dfs(dfs), _inode_ctrl(dfs) {
  }
  void Init() {
    _inode_ctrl.Init();
  }
  enum class FileType {
    kDirectory,
    kFile,
    kDevice,
  };
  IoReturnState LookupInodeFromPath(InodeContainer &inode, char *path, bool parent, char *name);
  static const size_t kMaxDirectoryNameLength = 14;
private:
  
  class InodeCtrl {
  public:
    InodeCtrl() = delete;
    InodeCtrl(DiskFileSystem *fs) : _dfs(fs) {
    }
    void Init() {
      for (int i = 0; i < kNodesNum; i++) {
        _nodes[i].Init(_dfs);
      }
    }
    IoReturnState Alloc(FileType type) __attribute__((warn_unused_result));
    IoReturnState Get(InodeContainer &icontainer, uint32_t inum) __attribute__((warn_unused_result));
  private:
    static const int kNodesNum = 50;
    SpinLock _lock;
    Inode _nodes[kNodesNum];
    DiskFileSystem *_dfs;
  } _inode_ctrl;
  
  static char *GetNextPathNameFromPath(char *path, char *name);
  IoReturnState ReadDataFromInode(uint8_t *data, InodeContainer &inode, size_t offset, size_t size) __attribute__((warn_unused_result));
  DiskFileSystem *_dfs;
};

class DiskFileSystem {
public:
  virtual ~DiskFileSystem() {
  }
  virtual IoReturnState AllocInode(InodeNumber &inum, VirtualFileSystem::FileType type) __attribute__((warn_unused_result)) = 0;
  virtual void GetStatOfInode(InodeNumber inum, Stat &stat) = 0;
  virtual InodeNumber GetRootInodeNum() = 0;
  /**
   * @brief read data from inode
   * @param buf buffer for storing data
   * @param inum target inode number
   * @param offset offset in target inode file
   * @param size size of the data to be read. This function overwrites the value with the actually size read.
   * 
   * Caller must allocate 'data'. The size of 'data' must be larger than 'size'.
   */
  virtual IoReturnState ReadDataFromInode(uint8_t *data, InodeNumber inum, size_t offset, size_t size) __attribute__((warn_unused_result)) = 0;
  /**
   * @brief lookup directory and return inode
   * @param dinode inode of directory
   * @param name entry name to lookup
   * @param offset offset of entry (no need to pass, returned by this function)
   * @param inode found inode (no need to pass, returned by this function)
   */
  virtual IoReturnState DirLookup(InodeNumber dinode, char *name, int &offset, InodeNumber &inode) __attribute__((warn_unused_result)) = 0;
private:
};

/**
 * @brief wrapper class of Inode
 * 
 */
class InodeContainer {
public:
  InodeContainer() = default;
  InodeContainer(const InodeContainer &obj) {
    _node = obj._node;
    _node->_ref++;
  }
  ~InodeContainer() {
    if (_node != nullptr) {
      Locker locker(_node->_lock);
      _node->_ref--;
      if (_node->_ref == 0) {
        // TODO release inode
        kernel_panic("inode", "not implemented");
      }
    }
  }
  void InitInode(Inode *inode, InodeNumber inum) {
    if (_node != nullptr) {
      Locker locker(_node->_lock);
      _node->_ref--;
      // TODO release inode
    }
    _node = inode;
    if (_node != nullptr) {
      Locker locker(_node->_lock);
      if (_node->_ref != 0) {
        kernel_panic("Inode", "unknown error");
      }
      _node->_ref = 1;
      _node->_inum = inum;
      _node->_flags = 0;
    }
  }
  bool SetIfSame(Inode *inode, InodeNumber inum) {
    if (inode == nullptr) {
      return false;
    }
    Locker locker(inode->_lock);
    if (inode->_ref != 0 && inode->_inum == inum) { 
      _node = inode;
      _node->_ref++;
      return true;
    } else {
      return false;
    }
  }
  void GetStatOfInode(Stat &st) {
    Locker locker(_node->_lock);
    assert(_node != nullptr);
    st.dev = 0; // dummy
    _node->_fs->GetStatOfInode(_node->_inum, st);
  }
  IoReturnState ReadData(uint8_t *data, size_t offset, size_t size) __attribute__((warn_unused_result)) {
    Locker locker(_node->_lock);
    return _node->_fs->ReadDataFromInode(data, _node->_inum, offset, size);
  }
  IoReturnState DirLookup(char *name, int &offset, InodeNumber &inum) __attribute__((warn_unused_result)) {
    Locker locker(_node->_lock);
    return _node->DirLookup(_node->_inodem name, offset, inum);
  }
private:
  Inode *_node = nullptr;
};

