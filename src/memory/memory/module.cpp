// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/module.hpp"

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <boost/scope_exit.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/detail/to_upper_ordinal.hpp"
#include "hadesmem/detail/module_find_procedure.hpp"

namespace hadesmem
{

Module::Module(Process const* process, HMODULE handle)
  : process_(process), 
  handle_(nullptr), 
  size_(0), 
  name_(), 
  path_()
{
  Initialize(handle);
}

Module::Module(Process const* process, std::wstring const& path)
  : process_(process), 
  handle_(nullptr), 
  size_(0), 
  name_(), 
  path_()
{
  Initialize(path);
}

Module::Module(Process const* process, MODULEENTRY32 const& entry)
  : process_(process), 
  handle_(nullptr), 
  size_(0), 
  name_(), 
  path_()
{
  Initialize(entry);
}

Module::Module(Module const& other)
  : process_(other.process_), 
  handle_(other.handle_), 
  size_(other.size_), 
  name_(other.name_), 
  path_(other.path_)
{ }

Module& Module::operator=(Module const& other)
{
  process_ = other.process_;
  handle_ = other.handle_;
  size_ = other.size_;
  name_ = other.name_;
  path_ = other.path_;
  
  return *this;
}

Module::Module(Module&& other) BOOST_NOEXCEPT
  : process_(other.process_), 
  handle_(other.handle_), 
  size_(other.size_), 
  name_(std::move(other.name_)), 
  path_(std::move(other.path_))
{
  other.process_ = nullptr;
  other.handle_ = nullptr;
  other.size_ = 0;
}

Module& Module::operator=(Module&& other) BOOST_NOEXCEPT
{
  process_ = other.process_;
  handle_ = other.handle_;
  size_ = other.size_;
  name_ = std::move(other.name_);
  path_ = std::move(other.path_);
  
  other.handle_ = nullptr;
  other.process_ = nullptr;
  other.size_ = 0;
  
  return *this;
}

Module::~Module()
{ }

Process const* Module::GetProcess() const BOOST_NOEXCEPT
{
  return process_;
}

HMODULE Module::GetHandle() const BOOST_NOEXCEPT
{
  return handle_;
}

DWORD Module::GetSize() const BOOST_NOEXCEPT
{
  return size_;
}

std::wstring Module::GetName() const
{
  return name_;
}

std::wstring Module::GetPath() const
{
  return path_;
}

void Module::Initialize(HMODULE handle)
{
  auto handle_check = 
    [&] (MODULEENTRY32 const& entry) -> bool
    {
      if (entry.hModule == handle || !handle)
      {
        return true;
      }
      
      return false;
    };
  
  InitializeIf(handle_check);
}

void Module::Initialize(std::wstring const& path)
{
  bool const is_path = (path.find(L'\\') != std::wstring::npos) || 
    (path.find(L'/') != std::wstring::npos);
  
  std::wstring const path_upper = detail::ToUpperOrdinal(path);
  
  auto path_check = 
    [&] (MODULEENTRY32 const& entry) -> bool
    {
      // TODO: Investigate whether 'equivalent' is the right API to use here. 
      // Not sure whether it matches the behavior of Windows in terms of 
      // following symlinks/hardlinks/etc.
      if (is_path && boost::filesystem::equivalent(path, entry.szExePath))
      {
        return true;
      }
      
      if (!is_path && path_upper == detail::ToUpperOrdinal(entry.szModule))
      {
        return true;
      }
      
      return false;
    };
  
  InitializeIf(path_check);
}

void Module::Initialize(MODULEENTRY32 const& entry)
{
  handle_ = entry.hModule;
  size_ = entry.modBaseSize;
  name_ = entry.szModule;
  path_ = entry.szExePath;
}

void Module::InitializeIf(EntryCallback const& check_func)
{
  HANDLE const snap = CreateToolhelp32Snapshot(
    TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process_->GetId());
  if (snap == INVALID_HANDLE_VALUE)
  {
    // TODO: Improve handling of ERROR_BAD_LENGTH.
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("CreateToolhelp32Snapshot failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  BOOST_SCOPE_EXIT_ALL(&)
  {
    // WARNING: Handle is leaked if CloseHandle fails.
    BOOST_VERIFY(::CloseHandle(snap));
  };
  
  MODULEENTRY32 entry;
  ::ZeroMemory(&entry, sizeof(entry));
  entry.dwSize = sizeof(entry);
  
  for (BOOL more_mods = ::Module32First(snap, &entry); more_mods; 
    more_mods = ::Module32Next(snap, &entry)) 
  {
    if (check_func(entry))
    {
      Initialize(entry);      
      return;
    }
  }
  
  // TODO: Improve error handling when the error code returned by the module 
  // enumeration APIs is something other than ERROR_NO_MORE_FILES.
  DWORD const last_error = ::GetLastError();
  BOOST_THROW_EXCEPTION(HadesMemError() << 
    ErrorString("Could not find module.") << 
    ErrorCodeWinLast(last_error));
}

bool operator==(Module const& lhs, Module const& rhs) BOOST_NOEXCEPT
{
  return lhs.GetHandle() == rhs.GetHandle();
}

bool operator!=(Module const& lhs, Module const& rhs) BOOST_NOEXCEPT
{
  return !(lhs == rhs);
}

bool operator<(Module const& lhs, Module const& rhs) BOOST_NOEXCEPT
{
  return lhs.GetHandle() < rhs.GetHandle();
}

bool operator<=(Module const& lhs, Module const& rhs) BOOST_NOEXCEPT
{
  return lhs.GetHandle() <= rhs.GetHandle();
}

bool operator>(Module const& lhs, Module const& rhs) BOOST_NOEXCEPT
{
  return lhs.GetHandle() > rhs.GetHandle();
}

bool operator>=(Module const& lhs, Module const& rhs) BOOST_NOEXCEPT
{
  return lhs.GetHandle() >= rhs.GetHandle();
}

std::ostream& operator<<(std::ostream& lhs, Module const& rhs)
{
  return (lhs << rhs.GetHandle());
}

std::wostream& operator<<(std::wostream& lhs, Module const& rhs)
{
  return (lhs << rhs.GetHandle());
}

FARPROC FindProcedure(Module const& module, std::string const& name)
{
  return detail::FindProcedureInternal(module, name.c_str());
}

FARPROC FindProcedure(Module const& module, WORD ordinal)
{
  return detail::FindProcedureInternal(module, MAKEINTRESOURCEA(ordinal));
}

}
