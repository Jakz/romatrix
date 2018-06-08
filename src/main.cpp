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

#include "tbx/base/path.h"

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
  static int sreaddir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, fuse_file_info *fi) { return instance->readdir(path, buf, filler, offset, fi); }
  static int sopendir(const char* path, fuse_file_info* fi) { return instance->opendir(path, fi); }
  
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
  
  fs_ret opendir(const fs_path& path, fuse_file_info* fi);
  fs_ret readdir(const fs_path& path, void* buf, fuse_fill_dir_t filler, off_t offset, fuse_file_info* fi);

  
public:
  MatrixFS()
  {
    instance = this;
    
    memset(&ops, 0, sizeof(fuse_operations));
    
    ops.statfs = statsfs;
    ops.access = access;
    ops.getattr = sgetattr;
    
    ops.opendir = sopendir;
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

#include "tbx/base/file_system.h"

#include "data/entry.h"

#include <unordered_set>
#include <unordered_map>
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
  
  HashData get()
  {
    HashData hashData;
    
    hashData.size = 0;
    hashData.crc32 = crc.get();
    hashData.md5 = md5.get();
    hashData.sha1 = sha1.get();
    
    return hashData;
  }
  
  void update(const void* data, size_t length)
  {
    crc.update(data, length);
    md5.update(data, length);
    sha1.update(data, length);
  }
  
  void reset()
  {
    crc.reset();
    md5.reset();
    sha1.reset();
  }
};



class DatabaseStore
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
class LevelDBDatabase : public DatabaseStore
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
    
class DatabaseData
{
public:
  using dat_list = std::unordered_map<std::string, DatFile>;
  using hash_map = std::unordered_set<HashData, HashData::hasher>;
  
private:
  dat_list _dats;
  hash_map _hashes;
  
public:
  //dat_list& dats() { return _dats; }
  const hash_map& hashes() const { return _hashes; }
  const dat_list& dats() const { return _dats; }
  
  DatFile* addDatFile(const DatFile& dat) { return &(*_dats.insert(std::make_pair(dat.name, dat)).first).second; }
  const HashData* addHashData(const HashData&& hash) { return &(*_hashes.insert(hash).first); }
  
  const DatFile* datForName(const std::string& name) const
  {
    auto it = _dats.find(name);
    return it != _dats.end() ? &it->second : nullptr;
  }
  
  size_t hashesCount() { return _hashes.size(); }
};

DatabaseData data;
    

using cataloguer_t = std::function<path(const HashData&)>;
    
    
int main(int argc, const char* argv[])
{  
  auto files = FileSystem::i()->contentsOfFolder("/Volumes/RAMDisk/input");
  auto root = path("/Volumes/RAMDisk/output");
  
  cataloguer_t cataloguer = [] (const HashData& hd) {
    std::string path = hd.sha1.operator std::string().substr(0,2);
    return path;
  };
  
  for (const auto& file : files)
  {
    Hasher hasher;
    HashData data = hasher.compute(file);
    
    path destination = root + cataloguer(data);
    FileSystem::i()->createFolder(destination);
    destination += file.filename();
    FileSystem::i()->copy(file, destination);
  }


  return 0;
  
  auto datFiles = FileSystem::i()->contentsOfFolder("dats");
  
  parsing::LogiqxParser parser;
  
  parsing::ParseResult tresult;
  
  //Database* database = new LevelDBDatabase();
  //database->init();
  
  Hasher hasher;
  
  for (const auto& dat : datFiles)
  {
    HashData hash = hasher.compute(dat);
    hasher.reset();
    std::cout << std::hex << hash.crc32 << " " << hash.md5.operator std::string() << " " << hash.sha1.operator std::string() << std::endl;
    
    
    auto result = parser.parse(dat);
    
    tresult.sizeInBytes += result.sizeInBytes;
    tresult.count += result.count;
    
    DatFile* datFile = data.addDatFile({ dat.filename(), dat.filename() });

    
    for (auto& entry : result.entries)
    {
      const byte* key = entry.hash.sha1.inner();
      const byte* value = (const byte*) &entry.hash;
      
      /*if (!database->contains(std::string((const char*)key)))
        database->write(key, sizeof(hash::sha1_t), value, sizeof(HashData));*/
      
      const HashData* hash = data.addHashData(std::move(entry.hash));
      
      datFile->entries.insert(std::make_pair( entry.name, hash ));
    }
    
  }
  
  std::cout << tresult.count << " entries in " << strings::humanReadableSize(tresult.sizeInBytes, true, 2) << std::endl;
  std::cout << data.hashes().size() << " entries in " << strings::humanReadableSize(std::accumulate(data.hashes().begin(), data.hashes().end(), 0UL, [](u64 v, const HashData& e) { return v += e.size; }), true, 2) << std::endl;

  //database->shutdown();
  
  MatrixFS fs;
  fs.createHandle();
  
  return 0;
}

#define ATTR_AS_FILE(x) x->st_mode = S_IFREG | 0444
#define ATTR_AS_DIR(x) x->st_mode = S_IFDIR | 0755

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
      auto tpath = ::path(path.c_str()+1);
      
      const DatFile* dat = data.datForName(tpath.str());
      
      if (dat)
        ATTR_AS_DIR(stbuf);
      else
      {
        auto ppath = tpath.parent();
        
        const DatFile* dat = data.datForName(ppath.str());

        if (dat)
        {
          auto fileName = tpath.filename();
          
          auto it = dat->entries.find(fileName);
          
          if (it != dat->entries.end())
          {
            ATTR_AS_FILE(stbuf);
            stbuf->st_size = it->second->size;
          }
        }
      }
      
      /*stbuf->st_mode = S_IFREG | 0444;
      stbuf->st_nlink = 1;
      stbuf->st_size = strlen(hello_str);*/
    }
    else
      res = -ENOENT;
    
    return res;
}
    
using fuse_opaque_handle = uint64_t;
constexpr static fuse_opaque_handle DAT_LIST_FH_HANDLE = 0xFFFFFFFFFFFFFFFFULL;
    
fs_ret MatrixFS::opendir(const fs_path& path, fuse_file_info* fi)
{
  /* opendir is called before readdir, we can store in fi->fh values used
     later by readdir */
  
  fs_ret ret = 0;
  fi->fh = 0ULL;
  
  /* if path is root use special value to signal it */
  if (path == "/")
    fi->fh = DAT_LIST_FH_HANDLE;
  else if (path.isAbsolute())
  {
    /* path is made as "/some_nice_text", find a corresponding DAT */
    const DatFile* dat = data.datForName(path.makeRelative().str());

    if (dat)
      fi->fh = reinterpret_cast<u64>(dat);
    else
      fi->fh = -ENOENT;
  }

  return ret;
}

fs_ret MatrixFS::readdir(const fs_path& path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi)
{
  /* special case, all the DATs folders */
  if (fi->fh == DAT_LIST_FH_HANDLE)
  {
    filler(buf, ".", nullptr, 0);
    filler(buf, "..", nullptr, 0);
    
    for (const auto& dat : data.dats())
      filler(buf, dat.second.folderName.c_str(), nullptr, 0);
    
    return 0;
  }
  else if (fi->fh)
  {
    const DatFile* dat = reinterpret_cast<const DatFile*>(fi->fh);
    
    if (dat)
    {
      filler(buf, ".", nullptr, 0);
      filler(buf, "..", nullptr, 0);
      
      /* fill with all entry from DAT */
      for (const auto& entry : dat->entries)
        filler(buf, entry.first.c_str(), nullptr, 0);
        
      return 0;
    }
  }
  
  return -ENOENT;
}
