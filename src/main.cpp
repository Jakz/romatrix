//
//  main.cpp
//  romatrix
//
//  Created by Jack on 30/05/18.
//  Copyright © 2018 Jack. All rights reserved.
//

#define FUSE_USE_VERSION 26
#include <cstring>
#include <cerrno>
#include <fuse/fuse.h>

#include <iostream>

#include "base/path.h"

static const char* hello_str = "Hello World!\n";
static const char* hello_path = "/hello";

using fs_ret = int;
using fs_path = path;

class MatrixFS
{
private:
  static MatrixFS* instance;
  struct fuse_operations ops;
  fuse* fs;
  
  static int statsfs(const char* foo, struct statvfs* stats)
  {
    stats->f_bsize = 2048;
    stats->f_blocks = 2;
    stats->f_bfree = std::numeric_limits<fsblkcnt_t>::max();
    stats->f_bavail = std::numeric_limits<fsblkcnt_t>::max();
    stats->f_files = 3;
    stats->f_ffree = std::numeric_limits<fsfilcnt_t>::max();
    stats->f_namemax = 256;
    return 0;
  }

  static int access(const char* path, int) { return 0; }
  
  static int sgetattr(const char *path, struct stat *stbuf) { return instance->getattr(path, stbuf); }
  static int sreaddir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) { return instance->readdir(path, buf, filler, offset, fi); }
  
  static int open(const char *path, struct fuse_file_info *fi)
  {
    //std::cout << "open" << std::endl;
    
    if (strcmp(path, hello_path) != 0)
      return -ENOENT;
    if ((fi->flags & O_ACCMODE) != O_RDONLY)
      return -EACCES;
    return 0;
  }
  
  static int read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
  {
    //std::cout << "read" << std::endl;
    
    size_t len;
    (void) fi;
    if(strcmp(path, hello_path) != 0)
      return -ENOENT;
    len = strlen(hello_str);
    if (offset < len) {
      if (offset + size > len)
        size = len - offset;
      memcpy(buf, hello_str + offset, size);
    } else
      size = 0;
    return (int)size;
  }
  
  fs_ret getattr(const fs_path& path, struct stat* stbuf);
  fs_ret readdir(const fs_path& path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi);

  
public:
  MatrixFS()
  {
    instance = this;
    
    memset(&ops, 0, sizeof(fuse_operations));
    
    ops.statfs = statsfs;
    ops.access = access;
    ops.getattr = sgetattr;
    ops.readdir = sreaddir;
    ops.open = open;
    ops.read = read;
  }
  
  void createHandle()
  {
    char* argv[] = { "fuse", "-d", "/Users/jack/mount" };
    int i =  fuse_main(3, argv, &ops, nullptr);
    
  }
};

MatrixFS* MatrixFS::instance;

#include <iostream>

#include "libs/pugixml/pugixml.hpp"
#include "parsers/parser.h"

#include "base/file_system.h"

#include "data/entry.h"

#include <unordered_set>
#include <numeric>


#include <cstdio>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
class Hasher
{
  using fd_t = int;
  
  hash::crc32_digester crc;
  hash::sha1_digester sha1;
  hash::md5_digester md5;
  
  size_t sizeOfFile(fd_t fd)
  {
    struct stat statbuf;
    fstat(fd, &statbuf);
    return statbuf.st_size;
  }
  
public:
  
  HashData compute(const path& path)
  {
    fd_t fd = open(path.c_str(), O_RDONLY);
    size_t size = sizeOfFile(fd);
    
    byte* buffer = reinterpret_cast<byte*>(mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0));
    
    crc.update(buffer, size);
    md5.update(buffer, size);
    sha1.update(buffer, size);
    
    
    munmap(buffer, size);
    
    HashData hashData;
    
    hashData.size = size;
    hashData.crc32 = crc.get();
    hashData.md5 = md5.get();
    hashData.sha1 = sha1.get();
    
    return hashData;
  }
  
  void reset()
  {
    crc.reset();
    md5.reset();
    sha1.reset();
  }
};



class Database
{
public:
  virtual bool init() = 0;
  virtual bool shutdown() = 0;
  
  virtual bool contains(std::string key) = 0;
  virtual bool write(std::string key, std::string value) = 0;
  
  bool write(const byte* key, size_t keyLen, const byte* value, size_t valueLen)
  {
    return write(std::string((const char*)key, keyLen), std::string((const char*)value, valueLen));
  }
};

#include "leveldb/db.h"
class LevelDBDatabase : public Database
{
private:
  leveldb::DB* db;
  leveldb::Options options;
  
public:
  bool init() override
  {
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, "database", &db);
    return status.ok();
  }
  
  bool shutdown() override
  {
    leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
    
    size_t count = 0, sizeInBytes = 0;
    for (it->SeekToFirst(); it->Valid(); it->Next())
    {
      //const hash::sha1_t* key = (const hash::sha1_t*) it->key().data();
      const HashData* data = (const HashData*) it->value().data();
      
      ++count;
      sizeInBytes += data->size;
    }
    
    std::cout << count << " entries in db " << strings::humanReadableSize(sizeInBytes, true, 2) << std::endl;

    
    delete it;
    
    delete db;
    //leveldb::DestroyDB("database", options);

    return true;
  }
  
  bool contains(std::string key) override
  {
    std::string value;
    leveldb::Status status = db->Get(leveldb::ReadOptions(), key, &value);
    return status.ok() && !status.IsNotFound();
  }
  
  bool write(std::string key, std::string value) override
  {
    leveldb::Status status = db->Put(leveldb::WriteOptions(), key, value);
    return status.ok();
  }
};

std::vector<DatFile> datFiles;
    
    
int main(int argc, const char* argv[])
{
  auto dats = FileSystem::i()->contentsOfFolder("dats");
  
  parsing::LogiqxParser parser;
  
  std::unordered_set<HashData, HashData::hasher> data;
  parsing::ParseResult tresult;
  
  //Database* database = new LevelDBDatabase();
  //database->init();
  
  Hasher hasher;
  
  for (const auto& dat : dats)
  {
    HashData hash = hasher.compute(dat);
    hasher.reset();
    std::cout << std::hex << hash.crc32 << " " << hash.md5.operator std::string() << " " << hash.sha1.operator std::string() << std::endl;
    
    
    auto result = parser.parse(dat);
    
    tresult.sizeInBytes += result.sizeInBytes;
    tresult.count += result.count;
    
    for (auto& entry : result.entries)
    {
      const byte* key = entry.hash.sha1.inner();
      const byte* value = (const byte*) &entry.hash;
      
      /*if (!database->contains(std::string((const char*)key)))
        database->write(key, sizeof(hash::sha1_t), value, sizeof(HashData));*/
      
      
      data.insert(std::move(entry.hash));
    }
    
    datFiles.push_back({ dat.filename(), dat.filename() });
  }
  
  std::cout << tresult.count << " entries in " << strings::humanReadableSize(tresult.sizeInBytes, true, 2) << std::endl;
  std::cout << data.size() << " entries in " << strings::humanReadableSize(std::accumulate(data.begin(), data.end(), 0UL, [](u64 v, const HashData& e) { return v += e.size; }), true, 2) << std::endl;

  //database->shutdown();
  
  MatrixFS fs;
  fs.createHandle();
  
  return 0;
}

#define ATTR_AS_FILE(x) x->st_mode = S_IFREG | 0444;
#define ATTR_AS_DIR(x) x->st_mode = S_IFDIR | 0755;

fs_ret MatrixFS::getattr(const fs_path& path, struct stat* stbuf)
{
    int res = 0;
    memset(stbuf, 0, sizeof(struct stat));
    stbuf->st_nlink = 1;
    
    if (path == "/")
    {
      ATTR_AS_DIR(stbuf);
    }
    else if (path.isAbsolute())
    {
      auto it = std::find_if(datFiles.begin(), datFiles.end(), [&path](const DatFile& dat) { return path == "/" + dat.folderName; });
      if (it != datFiles.end())
        ATTR_AS_DIR(stbuf);
      
      /*stbuf->st_mode = S_IFREG | 0444;
      stbuf->st_nlink = 1;
      stbuf->st_size = strlen(hello_str);*/
    }
    else
      res = -ENOENT;
    
    return res;
}


fs_ret MatrixFS::readdir(const fs_path& path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi)
{
  if (path == "/")
  {
    filler(buf, ".", nullptr, 0);
    filler(buf, "..", nullptr, 0);
    
    for (const auto& dat : datFiles)
      filler(buf, dat.folderName.c_str(), nullptr, 0);
    
    return 0;
  }
  else if (path.isAbsolute())
  {
    auto it = std::find_if(datFiles.begin(), datFiles.end(), [&path](const DatFile& dat) { return path.relativizeToParent(::path("/")) == dat.folderName; });
    if (it != datFiles.end())
    {
      filler(buf, ".", nullptr, 0);
      filler(buf, "..", nullptr, 0);
      return 0;
    }
  }
  
  return -ENOENT;
}
