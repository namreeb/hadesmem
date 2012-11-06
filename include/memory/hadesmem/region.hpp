// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <iosfwd>

#include <boost/config.hpp>

#include <windows.h>

namespace hadesmem
{

class Process;

class Region
{
public:
  Region(Process const* process, LPCVOID address);
  
  Region(Process const* process, MEMORY_BASIC_INFORMATION const& mbi) 
    BOOST_NOEXCEPT;
  
  PVOID GetBase() const BOOST_NOEXCEPT;
  
  PVOID GetAllocBase() const BOOST_NOEXCEPT;
  
  DWORD GetAllocProtect() const BOOST_NOEXCEPT;
  
  SIZE_T GetSize() const BOOST_NOEXCEPT;
  
  DWORD GetState() const BOOST_NOEXCEPT;
  
  DWORD GetProtect() const BOOST_NOEXCEPT;
  
  DWORD GetType() const BOOST_NOEXCEPT;
  
private:
  Process const* process_;
  MEMORY_BASIC_INFORMATION mbi_;
};

bool operator==(Region const& lhs, Region const& rhs) BOOST_NOEXCEPT;

bool operator!=(Region const& lhs, Region const& rhs) BOOST_NOEXCEPT;

bool operator<(Region const& lhs, Region const& rhs) BOOST_NOEXCEPT;

bool operator<=(Region const& lhs, Region const& rhs) BOOST_NOEXCEPT;

bool operator>(Region const& lhs, Region const& rhs) BOOST_NOEXCEPT;

bool operator>=(Region const& lhs, Region const& rhs) BOOST_NOEXCEPT;

std::ostream& operator<<(std::ostream& lhs, Region const& rhs);

std::wostream& operator<<(std::wostream& lhs, Region const& rhs);

}
