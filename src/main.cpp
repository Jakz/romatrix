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

int main(int argc, const char* argv[])
{
  auto dats = FileSystem::i()->contentsOfFolder("dats");
  
  parsing::LogiqxParser parser;
  
  std::unordered_set<HashData, HashData::hasher> data;
  parsing::ParseResult tresult;
  
  for (const auto& dat : dats)
  {
    auto result = parser.parse(dat);
    
    
    tresult.sizeInBytes += result.sizeInBytes;
    tresult.count += result.count;
    
    for (auto& entry : result.entries)
    {
      data.insert(std::move(entry.hash));
    }
  }
  
  std::cout << tresult.count << " entries in " << strings::humanReadableSize(tresult.sizeInBytes, true, 2) << std::endl;
  std::cout << data.size() << " entries in " << strings::humanReadableSize(std::accumulate(data.begin(), data.end(), 0UL, [](u64 v, const HashData& e) { return v += e.size; }), true, 2) << std::endl;

  
  
  return 0;
}
