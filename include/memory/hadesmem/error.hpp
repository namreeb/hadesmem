// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <string>
#include <sstream>
#include <typeinfo>
#include <exception>

#include <windows.h>

#include "hadesmem/config.hpp"

namespace hadesmem
{

namespace detail
{

template <typename Tag, typename T>
class ErrorInfo
{
public:
  typedef Tag tag_type;
  typedef T value_type;

  explicit ErrorInfo(T const& t)
    : t_(t)
  { }

  T const& Value() const
  {
    return t_;
  }

private:
  T t_;
};

}

typedef detail::ErrorInfo<struct TagErrorString, std::string> ErrorString;
typedef detail::ErrorInfo<struct TagErrorCodeWinRet, DWORD_PTR> 
  ErrorCodeWinRet;
typedef detail::ErrorInfo<struct TagErrorCodeWinLast, DWORD> ErrorCodeWinLast;
typedef detail::ErrorInfo<struct TagErrorCodeWinOther, DWORD_PTR> 
  ErrorCodeWinOther;
typedef detail::ErrorInfo<struct TagErrorCodeOther, DWORD_PTR> ErrorCodeOther;
typedef detail::ErrorInfo<struct TagErrorFunc, std::string> ErrorFunc;
typedef detail::ErrorInfo<struct TagErrorFile, std::string> ErrorFile;
typedef detail::ErrorInfo<struct TagErrorLine, int> ErrorLine;

class Error : public std::exception
{
public:
  Error();
  
  virtual char const* what() const HADESMEM_NOEXCEPT;

private:
  template <typename T>
  void Add(T const& t) const
  {
    std::stringstream str;
    str << t.Value();
    what_ += "\n[" + std::string(typeid(typename T::tag_type*).name()) + "] = " 
      + str.str();
  }

  template <typename Tag, typename T>
  friend Error const& operator<<(Error const& x, 
    detail::ErrorInfo<Tag, T> const& e);

  mutable std::string what_;
};

template <typename Tag, typename T>
Error const& operator<<(Error const& x, detail::ErrorInfo<Tag, T> const& e)
{
  x.Add(e);
  return x;
}

#define HADESMEM_THROW_EXCEPTION(x)\
  throw (x << \
  hadesmem::ErrorFunc(HADESMEM_CURRENT_FUNCTION) << \
  hadesmem::ErrorFile(__FILE__) << \
  hadesmem::ErrorLine(__LINE__))

}
