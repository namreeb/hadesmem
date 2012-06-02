// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <string>

#include <boost/config.hpp>

#include <windows.h>

#include "hadesmem/config.hpp"

namespace hadesmem
{

class Process
{
public:
  explicit Process(DWORD id);
  
  Process(Process&& other) BOOST_NOEXCEPT;
  
  Process& operator=(Process&& other) BOOST_NOEXCEPT;
  
  ~Process() BOOST_NOEXCEPT;
  
  DWORD GetId() const BOOST_NOEXCEPT;
  
  HANDLE GetHandle() const BOOST_NOEXCEPT;
  
  void Cleanup();
  
private:
  Process(Process const&);
  Process& operator=(Process const&);
  
  void CheckWoW64() const;
  
  HANDLE Open(DWORD id);
  
  void CleanupUnchecked() BOOST_NOEXCEPT;
  
  DWORD id_;
  HANDLE handle_;
};

bool operator==(Process const& lhs, Process const& rhs) BOOST_NOEXCEPT;

bool operator!=(Process const& lhs, Process const& rhs) BOOST_NOEXCEPT;

std::string GetPath(Process const& process);

bool IsWoW64(Process const& process);

}
