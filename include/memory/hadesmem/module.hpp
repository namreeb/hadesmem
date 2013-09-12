// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <string>
#include <cstring>
#include <ostream>
#include <utility>
#include <functional>

#include <windows.h>
#include <tlhelp32.h>

#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/pelib/export.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/detail/filesystem.hpp>
#include <hadesmem/pelib/export_list.hpp>
#include <hadesmem/detail/smart_handle.hpp>
#include <hadesmem/detail/to_upper_ordinal.hpp>

namespace hadesmem
{

class Module
{
public:
  explicit Module(Process const& process, HMODULE handle)
    : process_(&process), 
    handle_(nullptr), 
    size_(0), 
    name_(), 
    path_()
  {
    Initialize(handle);
  }
  
  explicit Module(Process const& process, std::wstring const& path)
    : process_(&process), 
    handle_(nullptr), 
    size_(0), 
    name_(), 
    path_()
  {
    Initialize(path);
  }

#if !defined(HADESMEM_DETAIL_NO_DEFAULTED_FUNCTIONS)

  Module(Module const&) HADESMEM_DETAIL_DEFAULTED_FUNCTION;

  Module& operator=(Module const&) HADESMEM_DETAIL_DEFAULTED_FUNCTION;

  Module(Module&&) HADESMEM_DETAIL_DEFAULTED_FUNCTION;

  Module& operator=(Module&&) HADESMEM_DETAIL_DEFAULTED_FUNCTION;

  ~Module() HADESMEM_DETAIL_DEFAULTED_FUNCTION;

#else // #if !defined(HADESMEM_DETAIL_NO_DEFAULTED_FUNCTIONS)

  Module(Module const& other)
    : process_(other.process_), 
    handle_(other.handle_), 
    size_(other.size_), 
    name_(other.name_), 
    path_(other.path_)
  { }

  Module& operator=(Module const& other)
  {
    process_ = other.process_;
    handle_ = other.handle_;
    size_ = other.size_;
    name_ = other.name_;
    path_ = other.path_;

    return *this;
  }

  Module(Module&& other) HADESMEM_DETAIL_NOEXCEPT
    : process_(other.process_), 
    handle_(other.handle_), 
    size_(other.size_), 
    name_(std::move(other.name_)), 
    path_(std::move(other.path_))
  { }

  Module& operator=(Module&& other) HADESMEM_DETAIL_NOEXCEPT
  {
    process_ = other.process_;
    handle_ = other.handle_;
    size_ = other.size_;
    name_ = std::move(other.name_);
    path_ = std::move(other.path_);

    return *this;
  }

  ~Module() HADESMEM_DETAIL_NOEXCEPT
  { }

#endif // #if !defined(HADESMEM_DETAIL_NO_DEFAULTED_FUNCTIONS)

  HMODULE GetHandle() const HADESMEM_DETAIL_NOEXCEPT
  {
    return handle_;
  }
  
  DWORD GetSize() const HADESMEM_DETAIL_NOEXCEPT
  {
    return size_;
  }
  
  std::wstring GetName() const
  {
    return name_;
  }
  
  std::wstring GetPath() const
  {
    return path_;
  }
  
private:
  template <typename ModuleT>
  friend class ModuleIterator;

  typedef std::function<bool (MODULEENTRY32 const&)> EntryCallback;

  explicit Module(Process const& process, MODULEENTRY32 const& entry)
    : process_(&process), 
    handle_(nullptr), 
    size_(0), 
    name_(), 
    path_()
  {
    Initialize(entry);
  }
  
  void Initialize(HMODULE handle)
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
  
  void Initialize(std::wstring const& path)
  {
    bool const is_path = (path.find_first_of(L"\\/") != std::wstring::npos);

    std::wstring const path_upper = detail::ToUpperOrdinal(path);

    auto path_check = 
      [&] (MODULEENTRY32 const& entry) -> bool
    {
      if (is_path)
      {
        if (detail::ArePathsEquivalent(path, entry.szExePath))
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
  
  void Initialize(MODULEENTRY32 const& entry)
  {
    handle_ = entry.hModule;
    size_ = entry.modBaseSize;
    name_ = entry.szModule;
    path_ = entry.szExePath;
  }

  void InitializeIf(EntryCallback const& check_func)
  {
    detail::SmartSnapHandle snap(::CreateToolhelp32Snapshot(
      TH32CS_SNAPMODULE, process_->GetId()));
    if (!snap.IsValid())
    {
      if (GetLastError() == ERROR_BAD_LENGTH)
      {
        snap = ::CreateToolhelp32Snapshot(
          TH32CS_SNAPMODULE, process_->GetId());
        if (!snap.IsValid())
        {
          DWORD const last_error = ::GetLastError();
          HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
            ErrorString("CreateToolhelp32Snapshot failed.") << 
            ErrorCodeWinLast(last_error));
        }
      }
      else
      {
        DWORD const last_error = ::GetLastError();
        HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
          ErrorString("CreateToolhelp32Snapshot failed.") << 
          ErrorCodeWinLast(last_error));
      }
    }

    MODULEENTRY32 entry;
    ::ZeroMemory(&entry, sizeof(entry));
    entry.dwSize = static_cast<DWORD>(sizeof(entry));

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
      HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
        ErrorString("Could not find module.") << 
        ErrorCodeWinLast(last_error));
    }
    else
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
        ErrorString("Module32First/Module32Next failed.") << 
        ErrorCodeWinLast(last_error));
    }
  }

  Process const* process_;
  HMODULE handle_;
  DWORD size_;
  std::wstring name_;
  std::wstring path_;
};

inline bool operator==(Module const& lhs, Module const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetHandle() == rhs.GetHandle();
}

inline bool operator!=(Module const& lhs, Module const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(Module const& lhs, Module const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetHandle() < rhs.GetHandle();
}

inline bool operator<=(Module const& lhs, Module const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetHandle() <= rhs.GetHandle();
}

inline bool operator>(Module const& lhs, Module const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetHandle() > rhs.GetHandle();
}

inline bool operator>=(Module const& lhs, Module const& rhs) HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetHandle() >= rhs.GetHandle();
}

inline std::ostream& operator<<(std::ostream& lhs, Module const& rhs)
{
  std::locale old = lhs.imbue(std::locale::classic());
  lhs << static_cast<void*>(rhs.GetHandle());
  lhs.imbue(old);
  return lhs;
}

inline std::wostream& operator<<(std::wostream& lhs, Module const& rhs)
{
  std::locale old = lhs.imbue(std::locale::classic());
  lhs << static_cast<void*>(rhs.GetHandle());
  lhs.imbue(old);
  return lhs;
}

}
