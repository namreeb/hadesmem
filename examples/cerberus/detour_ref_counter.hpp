// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <atomic>
#include <cstdint>

#include <hadesmem/config.hpp>

struct DetourRefCounter
{
public:
  DetourRefCounter(std::atomic<std::uint32_t>& ref_count) HADESMEM_DETAIL_NOEXCEPT
    : ref_count_{ &ref_count }
  {
    ++(*ref_count_);
  }

  DetourRefCounter(DetourRefCounter const&) = delete;

  DetourRefCounter& operator=(DetourRefCounter const&) = delete;

  ~DetourRefCounter()
  {
    --(*ref_count_);
  }

private:
  std::atomic<std::uint32_t>* ref_count_;
};
