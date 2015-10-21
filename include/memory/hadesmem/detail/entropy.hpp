// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cmath>
#include <cstdint>
#include <string>
#include <map>
#include <algorithm>

namespace hadesmem
{
namespace detail
{
double Log2(double number)
{
  return std::log(number) / std::log(2);
}

double GetEntropy(void const* p, std::size_t s)
{
  std::map<std::uint8_t, std::size_t> frequencies;
  for (std::size_t i = 0; i != s; ++i)
  {
    ++frequencies[static_cast<std::uint8_t const*>(p)[i]];
  }

  double entropy = 0;
  for (auto const& f : frequencies)
  {
    double freq = static_cast<double>(f.second) / s;
    entropy += freq * Log2(freq);
  }

  entropy *= -1;

  return entropy;
}
}
}
