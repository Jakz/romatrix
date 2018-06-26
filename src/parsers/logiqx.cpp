#include "parser.h"

#include "data/entry.h"
#include "libs/pugixml/pugixml.hpp"

namespace parsing
{
  enum class Status { BADDUMP, NODUMP, GOOD, VERIFIED };
  
  ParseResult LogiqxParser::parse(const path& path)
  {
    pugi::xml_document doc;
    pugi::xml_parse_result xmlResult = doc.load_file(path.c_str());
    
    ParseResult result = { 0, 0 };
    
    if (xmlResult)
    {
      const auto& games = doc.child("datafile").children("game");
      
      for (pugi::xml_node game : games)
      {
        const auto& roms = game.children("rom");
        
        for (pugi::xml_node rom : roms)
        {
          ParseEntry entry;
          
          const std::string statusAttribute = rom.attribute("status").as_string();
          Status status = Status::GOOD;

          if (statusAttribute == "nodump") status = Status::NODUMP;
          else if (statusAttribute == "baddump") status = Status::BADDUMP;
          else if (statusAttribute == "verified") status = Status::VERIFIED;
          
          if (status != Status::VERIFIED && status != Status::GOOD)
            continue;
          
          entry.name = rom.attribute("name").as_string();
          entry.hash.size = (size_t)std::strtoull(rom.attribute("size").as_string(), nullptr, 10);
          
          const auto crc = rom.attribute("crc"), md5 = rom.attribute("md5"), sha1 = rom.attribute("sha1");
          
          if (!crc.empty())
          {
            entry.hash.crc32 = (hash::crc32_t)std::strtoull(rom.attribute("crc").as_string(), nullptr, 16);
            entry.hash.crc32enabled = true;
          }
          
          if (!md5.empty())
          {
            entry.hash.md5 = strings::toByteArray(rom.attribute("md5").as_string());
            entry.hash.md5enabled = true;
          }
          
          if (!sha1.empty())
          {
            entry.hash.sha1 = strings::toByteArray(rom.attribute("sha1").as_string());
            entry.hash.sha1enabled = true;
          }
          
          assert(entry.hash.md5enabled || entry.hash.sha1enabled);
                 
          ++result.count;
          result.sizeInBytes += entry.hash.size;
          result.entries.push_back(entry);
        }
      }
    }
  
    return result;
  }
}
