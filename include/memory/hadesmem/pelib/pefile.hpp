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

enum class PeFileType
{
  Image, 
  Data
};

class PeFile
{
public:
  PeFile(Process const* process, PVOID base, PeFileType type);

  PeFile(PeFile const& other) BOOST_NOEXCEPT;

  PeFile& operator=(PeFile const& other) BOOST_NOEXCEPT;
  
  PeFile(PeFile&& other) BOOST_NOEXCEPT;
  
  PeFile& operator=(PeFile&& other) BOOST_NOEXCEPT;
  
  ~PeFile();

  PVOID GetBase() const BOOST_NOEXCEPT;

  PeFileType GetType() const BOOST_NOEXCEPT;

  PVOID RvaToVa(DWORD rva) const;
  
private:
  PVOID RvaToVaForImage(DWORD rva) const;

  PVOID RvaToVaForData(DWORD rva) const;

  Process const* process_;
  PVOID base_;
  PeFileType type_;
};

bool operator==(PeFile const& lhs, PeFile const& rhs) BOOST_NOEXCEPT;

bool operator!=(PeFile const& lhs, PeFile const& rhs) BOOST_NOEXCEPT;

bool operator<(PeFile const& lhs, PeFile const& rhs) BOOST_NOEXCEPT;

bool operator<=(PeFile const& lhs, PeFile const& rhs) BOOST_NOEXCEPT;

bool operator>(PeFile const& lhs, PeFile const& rhs) BOOST_NOEXCEPT;

bool operator>=(PeFile const& lhs, PeFile const& rhs) BOOST_NOEXCEPT;

std::ostream& operator<<(std::ostream& lhs, PeFile const& rhs);

std::wostream& operator<<(std::wostream& lhs, PeFile const& rhs);

}
