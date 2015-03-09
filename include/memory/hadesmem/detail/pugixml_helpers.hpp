// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <string>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <pugixml.hpp>
#include <pugixml.cpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/error.hpp>

namespace hadesmem
{
namespace detail
{
namespace pugixml
{
inline std::wstring GetAttributeValue(pugi::xml_node const& node,
                                      std::wstring const& name)
{
  auto const attr = node.attribute(name.c_str());
  if (!attr)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error{} << ErrorString{"Failed to find attribute for node."});
  }

  std::wstring const value = attr.value();
  if (value.empty())
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error{} << ErrorString{"Failed to find value for attribute."});
  }

  return value;
}

inline std::wstring GetOptionalAttributeValue(pugi::xml_node const& node,
                                              std::wstring const& name)
{
  auto const attr = node.attribute(name.c_str());
  if (!attr)
  {
    return {};
  }

  std::wstring const value = attr.value();
  if (value.empty())
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error{} << ErrorString{"Failed to find value for attribute."});
  }

  return value;
}
}
}
}