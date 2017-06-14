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

#include <string.h>
#include "fs.h"

/**
 * @brief allocate vfs inode
 * @param type type of inode
 * @param inode allocated inode (returned by this function)
 */
IoReturnState VirtualFileSystem::InodeCtrl::Alloc(FileType type, InodeContainer &inode) {
  InodeNumber inum;
  IoReturnState state1 = _dfs->AllocInode(inum, type);
  if (state1 != IoReturnState::kOk) {
    return state1;
  }
  IoReturnState state2 = Get(inode, inum);
  return state2;
}

IoReturnState VirtualFileSystem::InodeCtrl::Get(InodeContainer &icontainer, uint32_t inum) {
  Locker locker(_lock);
  Inode *empty = nullptr;
  for (int i = 0; i < kNodesNum; i++) {
    if (icontainer.SetIfSame(&_nodes[i], inum)) {
      return IoReturnState::kOk;
    } else if (_nodes[i].IsEmpty()) {
      empty = &_nodes[i];
    }
  }
  if (empty == nullptr) {
    return IoReturnState::kErrNoSwResource;
  }
  icontainer.InitInode(empty, inum);
  return IoReturnState::kOk;
}

/**
 * @brief get next path name from path
 * @param path path name
 * @param name next name
 * 
 * if no next path name, return nullptr.
 * the buffer size of 'name' must be 'kMaxDirectoryNameLength + 1'.
 */
const char *VirtualFileSystem::GetNextPathNameFromPath(const char *path, char *name) {
  while(*path == '/') {
    path++;
  }
  if (*path == '\0') {
    return nullptr;
  }
  const char *start = path;
  while(*path != '/' && *path != '\0') {
    path++;
  }
  size_t len = path - start;
  if (len > kMaxDirectoryNameLength) {
    len = kMaxDirectoryNameLength;
  }
  memcpy(name, start, len);
  name[len] = '\0';
  while(*path == '/') {
    path++;
  }
  return path;
}

/**
 * @brief read data from inode
 * @param buf buffer for storing data
 * @param inode target inode
 * @param offset offset in target inode file
 * @param size size of the data to be read. This function overwrites the value with the actually size read.
 * 
 * Caller must allocate 'data'. The size of 'data' must be larger than 'size'.
 */
IoReturnState VirtualFileSystem::ReadDataFromInode(uint8_t *data, InodeContainer &inode, size_t offset, size_t &size) {
  Stat stat;
  RETURN_IF_IOSTATE_NOT_OK(inode.GetStatOfInode(stat));
  if (stat.type == FileType::kDevice) {
    //TODO impl
    kernel_panic("VFS", "needs implementation!");
  }

  return inode.ReadData(data, offset, size);
}

/**
 * @brief lookup inode from path
 * @param inode found inode (returned by this function)
 * @param path path to lookup
 * @param parent return parent inode if true
 */
IoReturnState VirtualFileSystem::LookupInodeFromPath(InodeContainer &inode, const char *path, bool parent) {
  InodeContainer cinode;
  if (*path == '/') {
    RETURN_IF_IOSTATE_NOT_OK(_inode_ctrl.Get(cinode, _dfs->GetRootInodeNum()));
  } else {
    // TODO get cwd
    kernel_panic("VFS", "needs implementation!");
  }

  char name[kMaxDirectoryNameLength * 1];
  while((path = GetNextPathNameFromPath(path, name)) != nullptr) {
    Stat st;
    RETURN_IF_IOSTATE_NOT_OK(cinode.GetStatOfInode(st));

    if (st.type != FileType::kDirectory) {
      return IoReturnState::kErrInvalid;
    }

    if (*path == '\0' && parent) {
      inode = cinode;
      return IoReturnState::kOk;
    }

    InodeNumber inum;
    int offset;
    RETURN_IF_IOSTATE_NOT_OK(cinode.DirLookup(name, offset, inum));

    RETURN_IF_IOSTATE_NOT_OK(_inode_ctrl.Get(cinode, inum));
  }
  if (parent) {
    return IoReturnState::kErrInvalid;
  }
  inode = cinode;
  return IoReturnState::kOk;
}
