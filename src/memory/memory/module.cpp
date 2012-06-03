// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/module.hpp"

#if defined(HADESMEM_MSVC)
#pragma warning(push, 1)
#pragma warning(disable: 4996)
#endif // #if defined(HADESMEM_MSVC)
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wshadow"
#endif // #if defined(HADESMEM_GCC)
#include <boost/assert.hpp>
#include <boost/locale.hpp>
#include <boost/filesystem.hpp>
#include <boost/scope_exit.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#if defined(HADESMEM_MSVC)
#pragma warning(pop)
#endif // #if defined(HADESMEM_MSVC)
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_GCC)

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"

namespace hadesmem
{

Module::Module(Process const& process, HMODULE handle)
  : process_(&process), 
  handle_(nullptr), 
  size_(0), 
  name_(), 
  path_()
{
  Initialize(handle);
}

Module::Module(Process const& process, std::string const& path)
  : process_(&process), 
  handle_(nullptr), 
  size_(0), 
  name_(), 
  path_()
{
  Initialize(path);
}

HMODULE Module::GetHandle() const BOOST_NOEXCEPT
{
  return handle_;
}

DWORD Module::GetSize() const BOOST_NOEXCEPT
{
  return size_;
}

std::string Module::GetName() const BOOST_NOEXCEPT
{
  return name_;
}

std::string Module::GetPath() const BOOST_NOEXCEPT
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
  return this->process_ == other.process_ && 
    this->handle_ == other.handle_ && 
    this->size_ == other.size_ && 
    this->name_ == other.name_ && 
    this->path_ == other.path_;
}

bool Module::operator!=(Module const& other) const BOOST_NOEXCEPT
{
  return !(*this == other);
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
  
  Initialize(handle_check);
}

void Module::Initialize(std::string const& path)
{
  bool const is_path = (path.find('\\') != std::string::npos) || 
    (path.find('/') != std::string::npos);
  
  std::wstring const path_wide = 
    boost::locale::conv::utf_to_utf<wchar_t>(path);
  // FIXME: Fix the path comparison by more accurately matching the OS's 
  // rules on case insensitivity for paths.
  // http://goo.gl/y4wYF
  // http://goo.gl/Y2bFx
  std::wstring const path_wide_upper = boost::to_upper_copy(path_wide, 
    std::locale::classic());
  
  auto path_check = 
    [&] (MODULEENTRY32 const& entry) -> bool
    {
      if (is_path && boost::filesystem::equivalent(path, 
        entry.szExePath))
      {
        return true;
      }
      
      // FIXME: See note above about path comparisons.
      if (!is_path && boost::to_upper_copy(static_cast<std::wstring>(
        entry.szModule), std::locale::classic()) == path_wide_upper)
      {
        return true;
      }
      
      return false;
    };
  
  Initialize(path_check);
}

void Module::Initialize(std::function<bool (MODULEENTRY32 const& entry)> const& check_func)
{
  HANDLE const snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, 
    process_->GetId());
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
      handle_ = entry.hModule;
      size_ = entry.modBaseSize;
      name_ = boost::locale::conv::utf_to_utf<char>(entry.szModule);
      path_ = boost::locale::conv::utf_to_utf<char>(entry.szExePath);
      
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
  std::wstring const path_wide = boost::locale::conv::utf_to_utf<wchar_t>(path_);
  HMODULE const local_module(LoadLibraryEx(path_wide.c_str(), nullptr, 
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
