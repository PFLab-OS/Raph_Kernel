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

#include "v6fs.h"

V6FileSystem::V6FileSystem(Storage &storage) : _storage(storage), _inode_ctrl(storage, _sb), _block_ctrl(storage, _sb) {
  if (_storage.Read(_sb, kBlockSize) != IoReturnState::kOk) {
    kernel_panic("V6FS", "storage error");
  }
}

IoReturnState V6FileSystem::V6fsInodeCtrl::Alloc(InodeNumber &inum, Type type) {
  for (InodeNumber i = 1; i < _sb.ninodes; i++) {
    V6fsInode inode(i, _storage, _sb);
    RETURN_IF_IOSTATE_NOT_OK(inode.Read());
    if (inode._st.type == Type::kFree) {
      inode.Init(type);
      RETURN_IF_IOSTATE_NOT_OK(inode.Write());
      inum = i;
      return IoReturnState::kOk;
    }
  }
  return IoReturnState::kErrNoHwResource;
}

void V6FileSystem::V6fsInode::GetStatOfInode(VirtualFileSystem::Stat &stat) {
  stat.type = ConvType(_st.type);
  stat.nlink = _st.nlink;
  stat.size = _st.size;
  return;
}

/**
 * @brief allocate block
 * @param index block index allocated by this function (returned by this function)
 */
IoReturnState V6FileSystem::BlockCtrl::Alloc(BlockIndex &index) {
  Locker locker(_lock);
  for (uint32_t b = 0; b < _sb.size; b+=8) {
    uint8_t flag;
    _storage.Read(flag, _sb.bmapstart * kBlockSize + b / 8);
    for (int bi = 0; bi < 8; bi++) {
      uint8_t m = 1 << bi;
      if ((flag & m) == 0) {
        flag |= m;
        _storage.Write(flag, _sb.bmapstart * kBlockSize + b / 8);
        RETURN_IF_IOSTATE_NOT_OK(ClearBlock(b));
        index = b;
        return IoReturnState::kOk;
      }
    }
  }
  return IoReturnState::kErrNoHwResource;
}

/**
 * @brief clear block
 * @param index block index to clear
 */
IoReturnState V6FileSystem::BlockCtrl::ClearBlock(BlockIndex index) {
  uint32_t buf[kBlockSize] = {};
  return _storage.Write(buf, index * kBlockSize);
}


/**
 * @brief read data from inode
 * @param buf buffer for storing data
 * @param inode target inode number
 * @param offset offset in target inode file
 * @param size size of the data to be read. This function overwrites the value with the actually size read.
 * 
 * Caller must allocate 'data'. The size of 'data' must be larger than 'size'.
 */
IoReturnState V6FileSystem::ReadDataFromInode(uint8_t *buf, V6fsInode &inode, size_t offset, size_t &size) {
  IoReturnState rstate;
  if (offset > inode._st.size || offset + size < offset) {
    return IoReturnState::kErrInvalid;
  }
  if (offset + size > inode._st.size) {
    size = inode._st.size - offset;
  }

  for (size_t total = 0; total < size;) {
    uint32_t addr;
    rstate = MapBlock(inode, offset / kBlockSize, addr);
    if (rstate != IoReturnState::kOk) {
      size = total;
      return rstate;
    }
    size_t cur_size = kBlockSize - offset % kBlockSize;
    if (cur_size > size - total) {
      cur_size = size - total;
    }
    rstate = _storage.Read(buf + total, addr * kBlockSize + offset % kBlockSize, cur_size);
    if (rstate != IoReturnState::kOk) {
      size = total;
      return rstate;
    }
    total += cur_size;
    offset += cur_size;
  }
  return IoReturnState::kOk;
}

/**
 * @brief map block to inode. if already mapped, return mapped address. 
 * @param inode target inode
 * @param index block index of inode
 * @param addr mapped block address (returned by this function)
 *
 * Block address will be set to 'addr' if succeeds.
 */
IoReturnState V6FileSystem::MapBlock(V6fsInode &inode, int index, uint32_t &addr) {
  if (index < kNumOfDirectBlocks) {
    addr = inode._st.addrs[index];
    if (addr == 0) {
      RETURN_IF_IOSTATE_NOT_OK(_block_ctrl.Alloc(addr));
      inode._st.addrs[index] = addr;
      RETURN_IF_IOSTATE_NOT_OK(inode.Write());
    }
    return IoReturnState::kOk;
  }
  index -= kNumOfDirectBlocks;

  if (index < kNumEntriesOfIndirectBlock) {
    addr = inode._st.addrs[kNumOfDirectBlocks];
    if (addr == 0) {
      RETURN_IF_IOSTATE_NOT_OK(_block_ctrl.Alloc(addr));
      inode._st.addrs[kNumOfDirectBlocks] = addr;
    }

    uint32_t entry;
    _storage.Read(entry, addr * kBlockSize + index * sizeof(uint32_t));
    addr = entry;
    if (addr == 0) {
      RETURN_IF_IOSTATE_NOT_OK(_block_ctrl.Alloc(addr));
      _storage.Write(entry, addr * kBlockSize + index * sizeof(uint32_t));
    }

    return IoReturnState::kOk;
  } else {
    return IoReturnState::kErrInvalid;
  }
}

/**
 * @brief lookup directory and return inode
 * @param dinode inode of directory
 * @param name entry name to lookup
 * @param offset offset of entry (returned by this function)
 * @param inode found inode (returned by this function)
 */
IoReturnState V6FileSystem::DirLookup(V6fsInode &dinode, char *name, int &offset, InodeNumber &inode) {
  if (dinode._st.type != Type::kDirectory) {
    return IoReturnState::kErrInvalid;
  }

  DirEntry entry;
  
  for (uint32_t i = 0; i < dinode._st.size; i += sizeof(DirEntry)) {
    size_t size = sizeof(DirEntry);
    RETURN_IF_IOSTATE_NOT_OK(ReadDataFromInode(reinterpret_cast<uint8_t *>(&entry), dinode, i, size));
    if (size != sizeof(DirEntry)) {
      return IoReturnState::kErrInvalid;
    }
    if (entry.inum == 0) {
      continue;
    }
    if (strcmp(name, entry.name) == 0) {
      offset = i;
      inode = entry.inum;
      return IoReturnState::kOk;
    }
  }
  return IoReturnState::kErrNotFound;
}
