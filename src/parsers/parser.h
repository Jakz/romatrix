#include "tbx/base/path.h"

#include "data/entry.h"

#include <vector>

namespace parsing
{
  struct ParseResult
  {
    size_t count;
    size_t sizeInBytes;
    std::vector<ParseEntry> entries;
  };
  
  class Parser
  {
  public:
    virtual ParseResult parse(const path& path) = 0;
  };
  
  class LogiqxParser : public Parser
  {
  public:
    ParseResult parse(const path& path) override;
  };
};
