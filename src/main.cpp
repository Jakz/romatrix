//
//  main.cpp
//  romatrix
//
//  Created by Jack on 30/05/18.
//  Copyright © 2018 Jack. All rights reserved.
//

#include <iostream>

#include "libs/pugixml/pugixml.hpp"
#include "parsers/parser.h"

#include "base/file_system.h"

#include "data/entry.h"

#include <unordered_set>
#include <numeric>


#include "leveldb/db.h"

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


int main(int argc, const char* argv[])
{
  auto dats = FileSystem::i()->contentsOfFolder("dats");
  
  parsing::LogiqxParser parser;
  
  std::unordered_set<HashData, HashData::hasher> data;
  parsing::ParseResult tresult;
  
  Database* database = new LevelDBDatabase();
  database->init();
  
  for (const auto& dat : dats)
  {
    auto result = parser.parse(dat);
    
    tresult.sizeInBytes += result.sizeInBytes;
    tresult.count += result.count;
    
    for (auto& entry : result.entries)
    {
      const byte* key = entry.hash.sha1.inner();
      const byte* value = (const byte*) &entry.hash;
      
      if (!database->contains(std::string((const char*)key)))
        database->write(key, sizeof(hash::sha1_t), value, sizeof(HashData));
      
      
      data.insert(std::move(entry.hash));
    }
  }
  
  std::cout << tresult.count << " entries in " << strings::humanReadableSize(tresult.sizeInBytes, true, 2) << std::endl;
  std::cout << data.size() << " entries in " << strings::humanReadableSize(std::accumulate(data.begin(), data.end(), 0UL, [](u64 v, const HashData& e) { return v += e.size; }), true, 2) << std::endl;

  database->shutdown();
  
  return 0;
}
