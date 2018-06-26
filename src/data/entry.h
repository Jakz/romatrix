#pragma once

#include "tbx/hash/hash.h"

struct HashData
{
  struct
  {
    u64 md5enabled : 1;
    u64 sha1enabled : 1;
    u64 crc32enabled : 1;
    u64 padding : 1;
    u64 size : 60;
  };
  
  hash::crc32_t crc32;
  hash::md5_t md5;
  hash::sha1_t sha1;
  
  HashData() : md5enabled(false), sha1enabled(false), crc32enabled(false)
  {
    static_assert(sizeof(HashData) == sizeof(u64) + sizeof(hash::crc32_t) + sizeof(hash::sha1_t) + sizeof(hash::md5_t), "");
  }
  
  /*HashData& operator=(const HashData&& other)
  {
    size = other.size;
    crc32 = other.crc32;
    md5 = std::move(other.md5);
    sha1 = std::move(other.sha1);
    return *this;
  }*/
  
  //TODO: take into account the fact that some fields could not be enabled
  bool operator==(const HashData& other) const
  {
    return size == other.size
      && crc32 == other.crc32
      && md5 == other.md5
      && sha1 == other.sha1;
  }
  
  bool operator!=(const HashData& other) const
  {
    return !(this->operator==(other));
  }
  
  struct hasher
  {
    size_t operator()(const HashData& o) const
    {
      assert(o.md5enabled || o.sha1enabled);
      
      if (o.md5enabled)
        return *reinterpret_cast<const size_t*>(o.md5.inner());
      else
        return *reinterpret_cast<const size_t*>(o.sha1.inner());
    }
  };
};

struct ParseEntry
{
  HashData hash;
  std::string name;
};

#include <unordered_map>

struct DatFile
{
  std::string name;
  std::string folderName;
  std::unordered_map<std::string, const HashData*> entries;
};
