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

V6FileSystem::V6FileSystem(Storage &storage) {
  _storage.Read(_sb, kBlockSize);
}

IoReturnState V6FileSystem::V6fsInodeCtrl::Alloc(InodeNumber &inum, uint16_t type) {
  for (InodeNumber i = 1; i < _sb.ninodes; i++) {
    V6fsInode inode(i);
    ReadV6fsInode(inode);
    if (inode.type == 0) {
      // a free inode
      inode.Init(type);
      WriteV6fsInode(inode);
      inum = i;
      return IoReturnState::kOk;
    }
  }
  return IoReturnState::kErrNoHwResource;
}

void V6FileSystem::V6fsInode::GetStatOfInode(Stat &stat) {
  stat.type = type;
  stat.nlink = nlink;
  stat.size = size;
  return;
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
IoReturnState V6FileSystem::ReadDataFromInode(uint8_t *buf, Inode &inode, size_t offset, size_t &size) {
  IoReturnState rstate;
  if (offset > inode.size || offset + size < offset) {
    return IoReturnState::kErrInvalid;
  }
  if (offset + size > inode.size) {
    size = inode.size - offset;
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
    rstate = _storage.Read(buf + total, addr, cur_size);
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
 * @param addr mapped block address
 *
 * Block address will be set to 'addr' if succeeds.
 */
IoReturnState V6FileSystem::MapBlock(V6fsInode &inode, int index, uint32_t &addr) {
  if (index < kNumOfDirectBlocks) {
    addr = inode.addr[index];
    if (addr == 0) {
      addr = balloc();//RAPH_DEBUG
      inode.addr[index] = addr;
      WriteV6fsInode(inode);
    }
    return IoReturnState::kOk;
  }
  index -= kNumOfDirectBlocks;

  if (index < kNumEntriesOfIndirectBlock) {
    addr = info->addr[kNumOfDirectBlocks];
    if (addr == 0) {
      addr = balloc();//RAPH_DEBUG
      info->addr[kNumOfDirectBlocks] = addr;
    }

    uint32_t entry;
    _storage.Read(entry, addr * kBlockSize + index * sizeof(uint32_t));
    addr = entry;
    if (addr == 0) {
      addr = balloc(); //RAPH_DEBUG
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
 * @param offset offset of entry (no need to pass, returned by this function)
 * @param inode found inode (no need to pass, returned by this function)
 */
IoReturnState V6FileSystem::DirLookup(V6fsInode &dinode, char *name, int &offset, InodeNumber &inode) {
  Stat dinode_stat;
  dinode.GetStatOfInode(dinode_stat);
  if (dinode_stat.type != Stat::Type::kDirectory) {
    return IoReturnState::kErrInvalid;
  }

  DirEntry entry;
  
  for (uint32_t i = 0; i < dinode_stat.size; i += sizeof(DirEntry)) {
    size_t size = sizeof(DirEntry);
    IoReturnState rstate = ReadDataFromInode(reinterpret_cast<uint8_t *>(&dinode), dinode, i, size);
    RETURN_IF_IOSTATE_NOT_OK(rstate);
    if (size != sizeof(DirEntry)) {
      return IoReturnState::kErrInvalid;
    }
    if (dinode.inum == 0) {
      continue;
    }
    if (strcmp(name, dinode.name) == 0) {
      offset = i;
      inode = dinode.inum;
      return IoReturnState::kOk;
    }
  }
  return IoReturnState::kErrNotFound;
}
