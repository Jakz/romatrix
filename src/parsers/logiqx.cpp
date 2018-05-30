#include "parser.h"

#include "data/entry.h"
#include "libs/pugixml/pugixml.hpp"

namespace parsing
{
  ParseResult LogiqxParser::parse(const path& path)
  {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(path.c_str());
    
    if (result)
    {
      size_t count = 0;
      size_t sizeInBytes = 0;
      
      const auto& games = doc.child("datafile").children("game");
      
      for (pugi::xml_node game : games)
      {
        const auto& roms = game.children("rom");
        
        for (pugi::xml_node rom : roms)
        {
          Entry entry;
          
          entry.name = rom.attribute("name").as_string();
          entry.hash.size = (size_t)std::strtoull(rom.attribute("size").as_string(), nullptr, 10);
          entry.hash.crc32 = (hash::crc32_t)std::strtoull(rom.attribute("crc").as_string(), nullptr, 16);
          entry.hash.md5 = strings::toByteArray(rom.attribute("md5").as_string());
          entry.hash.sha1 = strings::toByteArray(rom.attribute("sha1").as_string());
       
          ++count;
          sizeInBytes += entry.hash.size;
        }
      }
    
      return { count, sizeInBytes };
    }
    
    return { 0, 0 };
  }
}
