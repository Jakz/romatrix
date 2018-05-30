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

int main(int argc, const char* argv[])
{
  auto dats = FileSystem::i()->contentsOfFolder("dats");
  
  parsing::LogiqxParser parser;
  
  for (const auto& dat : dats)
  {
    auto result = parser.parse(dat);
    
    std::cout << dat << " -- " << result.entries << " entries in " << strings::humanReadableSize(result.size, true, 2) << std::endl;
  }
  
  
  return 0;
}
