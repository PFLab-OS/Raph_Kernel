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

#include <fs.h>

class V6FileSystem : public DiskFileSystem{
public:
  V6FileSystem() = delete;
  V6FileSystem(Storage &storage) : _storage(storage), _inode_ctrl(_sb) {
  }
  virtual ~V6FileSystem() {
  }
private:
  static const int kBlockSize = 512;
  static const int kNumOfDirectBlocks = 12;
  static const int kNumEntriesOfIndirectBlock = kBlockSize / sizeof(uint32_t);
  struct SuperBlock {
    uint32_t size;
    uint32_t nblocks;
    uint32_t ninodes;
    uint32_t nlog;
    uint32_t logstart;
    uint32_t inodestart;
    uint32_t bmapstart;
  } __attribute__((__packed__)) _sb;
  static const size_t kMaxDirectoryNameLength = 14;
  struct DirEntry {
    uint16_t inum;
    char name[kMaxDirectoryNameLength];
  } __attribute__((__packed__));
  enum class Type : uint16_t {
    kDirectory = 1,
    kFile = 2,
    kDevice = 3,
  };
  class V6fsInode {
  public:
    struct V6fsInodeStruct {
      int16_t type;
      int16_t major;
      int16_t minor;
      int16_t nlink;
      uint32_t size;
      uint32_t addrs[kNumOfDirectBlocks];
      uint32_t indirect;
    } __attribute__((__packed__)) _st;
    V6fsInode() = delete;
    V6fsInode(int index) : _index(index) {
    }
    void Init(uint16_t type) {
      _st.type = type;
      _st.major = 0;
      _st.minor = 0;
      _st.nlink = 0;
      _st.size = 0;
      bzero(_st.addrs, sizeof(addrs));
      _st.indirect = 0;
    }
    void GetStatOfInode(Stat &stat);
    const int _index;
  };
  static Type ConvType(VirtualFileSystem::FileType type) {
    switch(type) {
    case VirtualFileSystem::FileType::kDirectory:
      return Type::kDirectory;
    case VirtualFileSystem::FileType::kFile:
      return Type::kFile;
    case VirtualFileSystem::FileType::kDevice:
      return Type::kDevice;
    };
  }
  IoReturnState ReadV6fsInode(V6fsInode &inode) __attribute__((warn_unused_result)) {
    return _storage.Read(inode._st, _sb.inodestart + sizeof(V6fsInodeStruct) * inode._index);
  }
  IoReturnState WriteV6fsInode(V6fsInode &inode) __attribute__((warn_unused_result)) {
    return _storage.Write(inode._st, _sb.inodestart + sizeof(V6fsInodeStruct) * inode._index);
  }
  class V6fsInodeCtrl {
  public:
    V6fsInodeCtrl() = delete;
    V6fsInodeCtrl(SuperBlock &sb) : _sb(sb) {
    }
    IoReturnState Alloc(InodeNumber &inum, uint16_t type) __attribute__((warn_unused_result));
    void GetStatOfInode(InodeNumber inum, Stat &stat) {
      assert(inum < _sb.ninodes);
      V6fsInode inode(inum);
      ReadV6fsInode(inode);
      inode.GetStatOfInode(stat);
      stat.ino = inum;
    }
  private:
    SuperBlock &_sb;
  } _inode_ctrl;
  virtual IoReturnState AllocInode(InodeNumber &inum, VirtualFileSystem::FileType type) __attribute__((warn_unused_result)) override {
    return _inode_ctrl.Alloc(inum, ConvType(type));
  } _inode_ctrl;
  virtual void GetStatOfInode(InodeNumber inum, Stat &stat) override {
    _inode_ctrl.GetStatOfInode(inum, stat);
  }
  virtual InodeNumber GetRootInodeNum() override {
    return 0;
  }
  virtual IoReturnState ReadDataFromInode(uint8_t *data, InodeNumber inum, size_t offset, size_t size) __attribute__((warn_unused_result)) override {
    assert(inum < _sb.ninodes);
    V6fsInode inode(inum);
    IoReturnState rstate = ReadV6fsInode(inode);
    if (rstate != IoReturnState::kOk) {
      return rstate;
    }
    return ReadDataFromInode(data, inode, offset, size);
  }
  IoReturnState ReadDataFromInode(uint8_t *data, Inode &inode, size_t offset, size_t &size) __attribute__((warn_unused_result));
  virtual IoReturnState V6FileSystem::DirLookup(InodeNumber dinode, char *name, int &offset, InodeNumber &inode) __attribute__((warn_unused_result)) override {
    assert(inum < _sb.ninodes);
    V6fsInode dinode_(inum);
    IoReturnState rstate = ReadV6fsInode(dinode_);
    if (rstate != IoReturnState::kOk) {
      return rstate;
    }
    return DirLookup(dinode_, name, offset, inode);
  }
  IoReturnState V6FileSystem::DirLookup(Inode &dinode, char *name, int &offset, InodeNumber &inode) __attribute__((warn_unused_result));
  IoReturnState MapBlock(V6fsInode &inode, int index, uint32_t &addr);
  Storage &_storage;
};
