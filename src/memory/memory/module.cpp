// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/module.hpp"

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/detail/smart_handle.hpp"
#include "hadesmem/detail/to_upper_ordinal.hpp"

namespace hadesmem
{

namespace
{

struct EnsureFreeLibrary
{
  explicit EnsureFreeLibrary(HMODULE module)
    : module_(module)
  { }

  ~EnsureFreeLibrary()
  {
    BOOST_VERIFY(::FreeLibrary(module_));
  }

  HMODULE module_;
};

FARPROC FindProcedureInternal(Module const& module, LPCSTR name)
{
  BOOST_ASSERT(name != nullptr);
  
  HMODULE const local_module = ::LoadLibraryEx(module.GetPath().c_str(), 
    nullptr, DONT_RESOLVE_DLL_REFERENCES);
  if (!local_module)
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(Error() << 
      ErrorString("LoadLibraryEx failed.") << 
      ErrorCodeWinLast(last_error));
  }
  EnsureFreeLibrary const local_module_free(local_module);
  
  FARPROC const local_func = ::GetProcAddress(local_module, name);
  if (!local_func)
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(Error() << 
      ErrorString("GetProcAddress failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  auto const func_delta = reinterpret_cast<DWORD_PTR>(local_func) - 
    reinterpret_cast<DWORD_PTR>(local_module);
  
  auto const remote_func = reinterpret_cast<FARPROC>(
    reinterpret_cast<DWORD_PTR>(module.GetHandle()) + func_delta);
  
  return remote_func;
}

HANDLE GetFileHandleForQuery(std::wstring const& path)
{
  HANDLE const handle = ::CreateFile(path.c_str(), 
    0, 
    FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, 
    nullptr, 
    OPEN_EXISTING, 
    FILE_FLAG_BACKUP_SEMANTICS, 
    nullptr);
  if (handle == INVALID_HANDLE_VALUE)
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(Error() << 
      ErrorString("CreateFile failed.") << 
      ErrorCodeWinLast(last_error));
  }

  return handle;
}

BY_HANDLE_FILE_INFORMATION GetFileInformationByHandle(HANDLE handle)
{
  BY_HANDLE_FILE_INFORMATION info;
  ::ZeroMemory(&info, sizeof(info));
  if (!GetFileInformationByHandle(handle, &info))
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(Error() << 
      ErrorString("GetFileInformationByHandle failed.") << 
      ErrorCodeWinLast(last_error));
  }

  return info;
}

bool IsPathEquivalent(std::wstring const& path1, std::wstring const& path2)
{
  detail::SmartHandle const handle1(GetFileHandleForQuery(path1), 
    INVALID_HANDLE_VALUE);
  detail::SmartHandle const handle2(GetFileHandleForQuery(path2), 
    INVALID_HANDLE_VALUE);

  BY_HANDLE_FILE_INFORMATION const info1 = GetFileInformationByHandle(
    handle1.GetHandle());
  BY_HANDLE_FILE_INFORMATION const info2 = GetFileInformationByHandle(
    handle2.GetHandle());

  return info1.dwVolumeSerialNumber == info2.dwVolumeSerialNumber
    && info1.nFileIndexHigh == info2.nFileIndexHigh
    && info1.nFileIndexLow == info2.nFileIndexLow
    && info1.nFileSizeHigh == info2.nFileSizeHigh
    && info1.nFileSizeLow == info2.nFileSizeLow
    && info1.ftLastWriteTime.dwLowDateTime == 
    info2.ftLastWriteTime.dwLowDateTime
    && info1.ftLastWriteTime.dwHighDateTime == 
    info2.ftLastWriteTime.dwHighDateTime;
}

}

Module::Module(Process const* process, HMODULE handle)
  : process_(process), 
  handle_(nullptr), 
  size_(0), 
  name_(), 
  path_()
{
  BOOST_ASSERT(process != nullptr);
  
  Initialize(handle);
}

Module::Module(Process const* process, std::wstring const& path)
  : process_(process), 
  handle_(nullptr), 
  size_(0), 
  name_(), 
  path_()
{
  BOOST_ASSERT(process != nullptr);
  
  Initialize(path);
}

Module::Module(Process const* process, MODULEENTRY32 const& entry)
  : process_(process), 
  handle_(nullptr), 
  size_(0), 
  name_(), 
  path_()
{
  BOOST_ASSERT(process != nullptr);
  
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
      if (is_path)
      {
        if (IsPathEquivalent(path, entry.szExePath))
        {
          return true;
        }
      }
      else
      {
        if (path_upper == detail::ToUpperOrdinal(entry.szModule))
        {
          return true;
        }
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
  detail::SmartHandle snap(::CreateToolhelp32Snapshot(
    TH32CS_SNAPMODULE, process_->GetId()), INVALID_HANDLE_VALUE);
  if (snap.GetHandle() == snap.GetInvalid())
  {
    if (GetLastError() == ERROR_BAD_LENGTH)
    {
      snap = ::CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE, process_->GetId());
      if (snap.GetHandle() == snap.GetInvalid())
      {
        DWORD const last_error = ::GetLastError();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorString("CreateToolhelp32Snapshot failed.") << 
          ErrorCodeWinLast(last_error));
      }
    }
    else
    {
      DWORD const last_error = ::GetLastError();
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorString("CreateToolhelp32Snapshot failed.") << 
        ErrorCodeWinLast(last_error));
    }
  }
  
  MODULEENTRY32 entry;
  ::ZeroMemory(&entry, sizeof(entry));
  entry.dwSize = sizeof(entry);
  
  for (BOOL more_mods = ::Module32First(snap.GetHandle(), &entry); more_mods; 
    more_mods = ::Module32Next(snap.GetHandle(), &entry)) 
  {
    if (check_func(entry))
    {
      Initialize(entry);      
      return;
    }
  }
  
  DWORD const last_error = ::GetLastError();
  if (last_error == ERROR_NO_MORE_FILES)
  {
    BOOST_THROW_EXCEPTION(Error() << 
      ErrorString("Could not find module.") << 
      ErrorCodeWinLast(last_error));
  }
  else
  {
    BOOST_THROW_EXCEPTION(Error() << 
      ErrorString("Module enumeration failed.") << 
      ErrorCodeWinLast(last_error));
  }
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
  return FindProcedureInternal(module, name.c_str());
}

FARPROC FindProcedure(Module const& module, WORD ordinal)
{
  return FindProcedureInternal(module, MAKEINTRESOURCEA(ordinal));
}

}
