#include "base/path.h"

namespace parsing
{
  struct ParseResult
  {
    size_t entries;
    size_t size;
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
