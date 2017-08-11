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
  V6FileSystem(Storage &storage);
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
    kFree = 0,
    kDirectory = 1,
    kFile = 2,
    kDevice = 3,
  };
  class V6fsInode {
  public:
    struct V6fsInodeStruct {
      Type type;
      int16_t major;
      int16_t minor;
      int16_t nlink;
      uint32_t size;
      uint32_t addrs[kNumOfDirectBlocks];
      uint32_t indirect;
    } __attribute__((__packed__)) _st;
    V6fsInode() = delete;
    V6fsInode(int index, Storage &storage, SuperBlock &sb) : _index(index), _storage(storage), _sb(sb) {
    }
    void Init(Type type) {
      _st.type = type;
      _st.major = 0;
      _st.minor = 0;
      _st.nlink = 0;
      _st.size = 0;
      bzero(_st.addrs, sizeof(_st.addrs));
      _st.indirect = 0;
    }
    IoReturnState Read() __attribute__((warn_unused_result)) {
      return _storage.Read(_st, _sb.inodestart * kBlockSize + sizeof(V6fsInodeStruct) * _index);
    }
    IoReturnState Write() __attribute__((warn_unused_result)) {
      return _storage.Write(_st, _sb.inodestart * kBlockSize + sizeof(V6fsInodeStruct) * _index);
    }
    void GetStatOfInode(VirtualFileSystem::Stat &stat);
    const int _index;
    Storage &_storage;
    SuperBlock &_sb;
  };
  class V6fsInodeCtrl {
  public:
    V6fsInodeCtrl() = delete;
    V6fsInodeCtrl(Storage &storage, SuperBlock &sb) : _storage(storage), _sb(sb) {
    }
    IoReturnState Alloc(InodeNumber &inum, Type type) __attribute__((warn_unused_result));
    IoReturnState GetStatOfInode(InodeNumber inum, VirtualFileSystem::Stat &stat) __attribute__((warn_unused_result)) {
      assert(inum < _sb.ninodes);
      V6fsInode inode(inum, _storage, _sb);
      RETURN_IF_IOSTATE_NOT_OK(inode.Read());
      inode.GetStatOfInode(stat);
      stat.ino = inum;
      return IoReturnState::kOk;
    }
  private:
    Storage &_storage;
    SuperBlock &_sb;
  };
  using BlockIndex = uint32_t;
  class BlockCtrl {
  public:
    BlockCtrl() = delete;
    BlockCtrl(Storage &storage, SuperBlock &sb) : _storage(storage), _sb(sb) {
    }
    IoReturnState Alloc(BlockIndex &index) __attribute__((warn_unused_result));
    IoReturnState ClearBlock(BlockIndex index) __attribute__((warn_unused_result));
  private:
    Storage &_storage;
    SuperBlock &_sb;
    SpinLock _lock;
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
    assert(false);
  }
  static VirtualFileSystem::FileType ConvType(Type type) {
    switch(type) {
    case Type::kDirectory:
      return VirtualFileSystem::FileType::kDirectory;
    case Type::kFile:
      return VirtualFileSystem::FileType::kFile;
    case Type::kDevice:
      return VirtualFileSystem::FileType::kDevice;
    case Type::kFree:
      assert(false);
    };
    assert(false);
  }
  virtual IoReturnState AllocInode(InodeNumber &inum, VirtualFileSystem::FileType type) override __attribute__((warn_unused_result)) {
    return _inode_ctrl.Alloc(inum, ConvType(type));
  }
  virtual IoReturnState GetStatOfInode(InodeNumber inum, VirtualFileSystem::Stat &stat) override __attribute__((warn_unused_result)) {
    return _inode_ctrl.GetStatOfInode(inum, stat);
  }
  virtual InodeNumber GetRootInodeNum() override {
    return 1;
  }
  virtual IoReturnState ReadDataFromInode(uint8_t *data, InodeNumber inum, size_t offset, size_t &size) override __attribute__((warn_unused_result)) {
    assert(inum < _sb.ninodes);
    V6fsInode inode(inum, _storage, _sb);
    IoReturnState rstate = inode.Read();
    if (rstate != IoReturnState::kOk) {
      return rstate;
    }
    return ReadDataFromInode(data, inode, offset, size);
  }
  IoReturnState ReadDataFromInode(uint8_t *data, V6fsInode &inode, size_t offset, size_t &size) __attribute__((warn_unused_result));
  virtual IoReturnState DirLookup(InodeNumber dinode, char *name, int &offset, InodeNumber &inode) override __attribute__((warn_unused_result)) {
    assert(dinode < _sb.ninodes);
    V6fsInode dinode_(dinode, _storage, _sb);
    RETURN_IF_IOSTATE_NOT_OK(dinode_.Read());
    return DirLookup(dinode_, name, offset, inode);
  }
  IoReturnState DirLookup(V6fsInode &dinode, char *name, int &offset, InodeNumber &inode) __attribute__((warn_unused_result));
  IoReturnState MapBlock(V6fsInode &inode, int index, uint32_t &addr);

  Storage &_storage;
  V6fsInodeCtrl _inode_ctrl;
  BlockCtrl _block_ctrl;
};
