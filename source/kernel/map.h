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

#pragma once

// refer to https://medium.com/@aozturk/simple-hash-map-hash-table-implementation-in-c-931965904250#.g1w9cjedo

template <typename K, typename V>
class HashNode {
public:
    HashNode(const K &key, const V &value) :
    key(key), value(value), next(nullptr) {
    }

    K getKey() const {
        return key;
    }

    V getValue() const {
        return value;
    }

    void setValue(V value) {
        HashNode::value = value;
    }

    HashNode *getNext() const {
        return next;
    }

    void setNext(HashNode *next) {
        HashNode::next = next;
    }

private:
    // key-value pair
    K key;
    V value;
    // next bucket with the same key
    HashNode *next;
};

template <typename K, int kHashMapTableSize>
struct KeyHash {
    unsigned long operator()(const K& key) const
    {
        return static_cast<unsigned long>(key) % kHashMapTableSize;
    }
};

template <typename K, typename V, int kHashMapTableSize = 1024, typename F = KeyHash<K, kHashMapTableSize>>
class HashMap {
public:

  HashMap() {
    // construct zero initialized hash table of size
    table = new HashNode<K, V> *[kHashMapTableSize]();
  }

  ~HashMap() {
    // destroy all buckets one by one
    for (int i = 0; i < kHashMapTableSize; ++i) {
      HashNode<K, V> *entry = table[i];
      while (entry != nullptr) {
        HashNode<K, V> *prev = entry;
        entry = entry->getNext();
        delete prev;
      }
      table[i] = nullptr;
    }
    // destroy the hash table
    delete [] table;
  }

  bool Get(const K &key, V &value) {
    unsigned long hashValue = hashFunc(key);
    HashNode<K, V> *entry = table[hashValue];


    while (entry != nullptr) {
      if (entry->getKey() == key) {
        value = entry->getValue();
        return true;
      }
      entry = entry->getNext();
    }
    return false;
  }

  void Put(const K &key, const V &value) {
    unsigned long hashValue = hashFunc(key);
    HashNode<K, V> *prev = nullptr;
    HashNode<K, V> *entry = table[hashValue];

    while (entry != nullptr && entry->getKey() != key) {
      prev = entry;
      entry = entry->getNext();
    }

    if (entry == nullptr) {
      entry = new HashNode<K, V>(key, value);
      if (prev == nullptr) {
        // insert as first bucket
        table[hashValue] = entry;
      } else {
        prev->setNext(entry);
      }
    } else {
      // just update the value
      entry->setValue(value);
    }
  }
  
  void Remove(const K &key) {
    unsigned long hashValue = hashFunc(key);
    HashNode<K, V> *prev = nullptr;
    HashNode<K, V> *entry = table[hashValue];

    while (entry != nullptr && entry->getKey() != key) {
      prev = entry;
      entry = entry->getNext();
    }

    if (entry == nullptr) {
      // key not found
      return;
    } else {
      if (prev == nullptr) {
        // remove first bucket of the list
        table[hashValue] = entry->getNext();
      } else {
        prev->setNext(entry->getNext());
      }
      delete entry;
    }
  }

private:
  // hash table
  HashNode<K, V> **table;
  F hashFunc;
};
