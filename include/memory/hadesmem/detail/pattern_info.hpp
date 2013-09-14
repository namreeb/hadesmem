// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <string>
#include <vector>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

namespace hadesmem
{

namespace detail
{

struct PatternInfo
{
  std::wstring name;
  std::wstring data;
};

struct ManipInfo
{
  struct Manipulator
  {
    enum 
    {
      kAdd, 
      kSub, 
      kRel, 
      kLea
    };
  };
  
  int type;
  std::vector<DWORD_PTR> operands;
};

struct PatternInfoFull
{
  PatternInfo pattern;
  std::vector<ManipInfo> manipulators;
};

}

}

BOOST_FUSION_ADAPT_STRUCT(hadesmem::detail::PatternInfo, 
  (std::wstring, name)
  (std::wstring, data))

BOOST_FUSION_ADAPT_STRUCT(hadesmem::detail::ManipInfo, 
  (int, type)
  (std::vector<DWORD_PTR>, operands))

BOOST_FUSION_ADAPT_STRUCT(hadesmem::detail::PatternInfoFull, 
  (hadesmem::detail::PatternInfo, pattern)
  (std::vector<hadesmem::detail::ManipInfo>, manipulators))
