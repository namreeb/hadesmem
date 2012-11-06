// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <string>
#include <functional>

#include <boost/config.hpp>

#include <windows.h>
#include <tlhelp32.h>

namespace hadesmem
{

class Process;

class Module
{
public:
  Module(Process const* process, HMODULE handle);
  
  Module(Process const* process, std::wstring const& path);
  
  Module(Process const* process, MODULEENTRY32 const& entry);
  
  Module(Module const& other);
  
  Module& operator=(Module const& other);
  
  Module(Module&& other) BOOST_NOEXCEPT;
  
  Module& operator=(Module&& other) BOOST_NOEXCEPT;
  
  HMODULE GetHandle() const BOOST_NOEXCEPT;
  
  DWORD GetSize() const BOOST_NOEXCEPT;
  
  std::wstring GetName() const;
  
  std::wstring GetPath() const;
  
private:
  void Initialize(HMODULE handle);
  
  void Initialize(std::wstring const& path);
  
  void Initialize(MODULEENTRY32 const& entry);
  
  typedef std::function<bool (MODULEENTRY32 const&)> EntryCallback;
  void InitializeIf(EntryCallback const& check_func);
  
  Process const* process_;
  HMODULE handle_;
  DWORD size_;
  std::wstring name_;
  std::wstring path_;
};

bool operator==(Module const& lhs, Module const& rhs) BOOST_NOEXCEPT;

bool operator!=(Module const& lhs, Module const& rhs) BOOST_NOEXCEPT;

bool operator<(Module const& lhs, Module const& rhs) BOOST_NOEXCEPT;

bool operator<=(Module const& lhs, Module const& rhs) BOOST_NOEXCEPT;

bool operator>(Module const& lhs, Module const& rhs) BOOST_NOEXCEPT;

bool operator>=(Module const& lhs, Module const& rhs) BOOST_NOEXCEPT;

std::ostream& operator<<(std::ostream& lhs, Module const& rhs);

std::wostream& operator<<(std::wostream& lhs, Module const& rhs);

FARPROC FindProcedure(Module const& module, std::string const& name);

FARPROC FindProcedure(Module const& module, WORD ordinal);

}
