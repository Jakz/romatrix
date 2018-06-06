#pragma once

#include "hash/hash.h"

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
  
  /*HashData& operator=(const HashData&& other)
  {
    size = other.size;
    crc32 = other.crc32;
    md5 = std::move(other.md5);
    sha1 = std::move(other.sha1);
    return *this;
  }*/
  
  bool operator==(const HashData& other) const
  {
    return size == other.size && crc32 == other.crc32 &&
      md5 == other.md5 && sha1 == other.sha1;
  }
  
  bool operator!=(const HashData& other) const
  {
    return size != other.size || crc32 != other.crc32 ||
      md5 != other.md5 || sha1 != other.sha1;
  }
  
  struct hasher
  {
    size_t operator()(const HashData& o) const
    {
      return *reinterpret_cast<const size_t*>(o.md5.inner());
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
