#pragma once

#include "data/hash/hash.h"

struct HashData
{
  size_t size;
  hash::crc32_t crc32;
  hash::md5_t md5;
  hash::sha1_t sha1;
  
  HashData()
  {
    static_assert(sizeof(HashData) == sizeof(size_t) + sizeof(hash::crc32_t) + sizeof(hash::sha1_t) + sizeof(hash::md5_t), "");
  }
};

struct Entry
{
  HashData hash;
  std::string name;
};
