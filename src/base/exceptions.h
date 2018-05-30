#pragma once

#include <exception>
#include <string>

#include "path.h"

namespace args
{
  class ArgumentParser;
}

namespace exceptions
{
  class rm_exception : public std::exception
  {
    
  };
  
  class rm_messaged_exception : public rm_exception
  {
  private:
    std::string _message;
    
  public:
    rm_messaged_exception(const std::string& message) : _message(message) { }
    const char* what() const noexcept override { return _message.c_str(); }
  };
  class file_not_found : public rm_exception
  {
  private:
    path path;
    
  public:
    file_not_found(const class path& path) : path(path) { }

    const char* what() const noexcept override { return path.c_str(); }
  };
  
  class path_non_relative : public rm_exception
  {
  private:
    path parent;
    path children;
    
  public:
    path_non_relative(const path& parent, const path& children) : parent(parent), children(children) { }
    
    const char* what() const noexcept override { return parent.c_str(); }
  };
  
  class path_exception : public rm_exception
  {
  private:
    std::string _message;
    
  public:
    path_exception(const std::string& message) : _message(message) { }
    
    const char* what() const noexcept override { return _message.c_str(); }
  };
  
  class error_opening_file : public rm_exception
  {
  private:
    path path;
    
  public:
    error_opening_file(const class path& path) : path(path) { }
    
    const char* what() const noexcept override { return path.c_str(); }
  };
  
  class error_reading_from_file : public rm_exception
  {
  private:
    path path;
    
  public:
    error_reading_from_file(const class path& path) : path(path) { }
    
    const char* what() const noexcept override { return path.c_str(); }
  };
  
  using file_format_error = rm_exception;
}
