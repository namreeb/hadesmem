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

Module::~Module() BOOST_NOEXCEPT
{ }

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

FARPROC Module::FindProcedure(std::string const& name) const
{
  return FindProcedureInternal(name.c_str());
}

FARPROC Module::FindProcedure(WORD ordinal) const
{
  return FindProcedureInternal(MAKEINTRESOURCEA(ordinal));
}

bool Module::operator==(Module const& other) const BOOST_NOEXCEPT
{
  return this->handle_ == other.handle_;
}

bool Module::operator!=(Module const& other) const BOOST_NOEXCEPT
{
  return !(*this == other);
}
  
bool Module::operator<(Module const& other) const BOOST_NOEXCEPT
{
  return this->handle_ < other.handle_;
}

bool Module::operator<=(Module const& other) const BOOST_NOEXCEPT
{
  return this->handle_ <= other.handle_;
}

bool Module::operator>(Module const& other) const BOOST_NOEXCEPT
{
  return this->handle_ > other.handle_;
}

bool Module::operator>=(Module const& other) const BOOST_NOEXCEPT
{
  return this->handle_ >= other.handle_;
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

void Module::InitializeIf(std::function<bool (MODULEENTRY32 const& entry)> 
  const& check_func)
{
  HANDLE const snap = CreateToolhelp32Snapshot(
    TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process_->GetId());
  if (snap == INVALID_HANDLE_VALUE)
  {
    DWORD const last_error = GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("CreateToolhelp32Snapshot failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  BOOST_SCOPE_EXIT_ALL(&)
  {
    // WARNING: Handle is leaked if CloseHandle fails.
    BOOST_VERIFY(CloseHandle(snap));
  };
  
  MODULEENTRY32 entry;
  ZeroMemory(&entry, sizeof(entry));
  entry.dwSize = sizeof(entry);
  
  for (BOOL more_mods = Module32First(snap, &entry); more_mods; 
    more_mods = Module32Next(snap, &entry)) 
  {
    if (check_func(entry))
    {
      Initialize(entry);      
      return;
    }
  }
  
  DWORD const last_error = GetLastError();
  BOOST_THROW_EXCEPTION(HadesMemError() << 
    ErrorString("Could not find module.") << 
    ErrorCodeWinLast(last_error));
}

FARPROC Module::FindProcedureInternal(LPCSTR name) const
{
  HMODULE const local_module(LoadLibraryEx(path_.c_str(), nullptr, 
    DONT_RESOLVE_DLL_REFERENCES));
  if (!local_module)
  {
    DWORD const last_error = GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Could not load module locally.") << 
      ErrorCodeWinLast(last_error));
  }
  
  BOOST_SCOPE_EXIT_ALL(&)
  {
    // WARNING: Handle is leaked if FreeLibrary fails.
    BOOST_VERIFY(FreeLibrary(local_module));
  };
  
  FARPROC const local_func = GetProcAddress(local_module, name);
  if (!local_func)
  {
    DWORD const last_error = GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Could not find target function.") << 
      ErrorCodeWinLast(last_error));
  }
  
  LONG_PTR const func_delta = reinterpret_cast<DWORD_PTR>(local_func) - 
    reinterpret_cast<DWORD_PTR>(static_cast<HMODULE>(local_module));
  
  FARPROC const remote_func = reinterpret_cast<FARPROC>(
    reinterpret_cast<DWORD_PTR>(handle_) + func_delta);
  
  return remote_func;
}

}
