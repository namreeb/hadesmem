// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <string>
#include <ostream>
#include <utility>
#include <functional>
#include <ostream>
#include <utility>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/filesystem.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <windows.h>
#include <tlhelp32.h>

#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/detail/assert.hpp>
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

  Module(Module&& other) HADESMEM_NOEXCEPT
    : process_(other.process_), 
    handle_(other.handle_), 
    size_(other.size_), 
    name_(std::move(other.name_)), 
    path_(std::move(other.path_))
  { }

  Module& operator=(Module&& other) HADESMEM_NOEXCEPT
  {
    process_ = other.process_;
    handle_ = other.handle_;
    size_ = other.size_;
    name_ = std::move(other.name_);
    path_ = std::move(other.path_);

    return *this;
  }

  ~Module() HADESMEM_NOEXCEPT
  { }

  HMODULE GetHandle() const HADESMEM_NOEXCEPT
  {
    return handle_;
  }
  
  DWORD GetSize() const HADESMEM_NOEXCEPT
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
    bool const is_path = (path.find(L'\\') != std::wstring::npos) || 
      (path.find(L'/') != std::wstring::npos);

    std::wstring const path_upper = detail::ToUpperOrdinal(path);

    auto path_check = 
      [&] (MODULEENTRY32 const& entry) -> bool
    {
      if (is_path)
      {
        if (boost::filesystem::equivalent(path, entry.szExePath))
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
          HADESMEM_THROW_EXCEPTION(Error() << 
            ErrorString("CreateToolhelp32Snapshot failed.") << 
            ErrorCodeWinLast(last_error));
        }
      }
      else
      {
        DWORD const last_error = ::GetLastError();
        HADESMEM_THROW_EXCEPTION(Error() << 
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
      HADESMEM_THROW_EXCEPTION(Error() << 
        ErrorString("Could not find module.") << 
        ErrorCodeWinLast(last_error));
    }
    else
    {
      HADESMEM_THROW_EXCEPTION(Error() << 
        ErrorString("Module enumeration failed.") << 
        ErrorCodeWinLast(last_error));
    }
  }

  Process const* process_;
  HMODULE handle_;
  DWORD size_;
  std::wstring name_;
  std::wstring path_;
};

inline bool operator==(Module const& lhs, Module const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetHandle() == rhs.GetHandle();
}

inline bool operator!=(Module const& lhs, Module const& rhs) HADESMEM_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(Module const& lhs, Module const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetHandle() < rhs.GetHandle();
}

inline bool operator<=(Module const& lhs, Module const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetHandle() <= rhs.GetHandle();
}

inline bool operator>(Module const& lhs, Module const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetHandle() > rhs.GetHandle();
}

inline bool operator>=(Module const& lhs, Module const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetHandle() >= rhs.GetHandle();
}

inline std::ostream& operator<<(std::ostream& lhs, Module const& rhs)
{
  return (lhs << rhs.GetHandle());
}

inline std::wostream& operator<<(std::wostream& lhs, Module const& rhs)
{
  return (lhs << rhs.GetHandle());
}

// TODO: Split this into a different file.
namespace detail
{

inline FARPROC FindProcedureInternal(Module const& module, LPCSTR name)
{
  HADESMEM_ASSERT(name != nullptr);

  // Do not continue if Shim Engine is enabled for local process, 
  // otherwise it could interfere with the address resolution.
  // TODO: Work around this with 'manual' export lookup or similar, as we 
  // need this to work in cases where we can't control whether we're shimmed 
  // or not.
  HMODULE const shim_eng_mod = ::GetModuleHandle(L"ShimEng.dll");
  if (shim_eng_mod)
  {
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Shims enabled for local process."));
  }

  SmartModuleHandle const local_module(::LoadLibraryEx(
    module.GetPath().c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES));
  if (!local_module.GetHandle())
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("LoadLibraryEx failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  FARPROC const local_func = ::GetProcAddress(local_module.GetHandle(), name);
  if (!local_func)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("GetProcAddress failed.") << 
      ErrorCodeWinLast(last_error));
  }

  HADESMEM_ASSERT(reinterpret_cast<DWORD_PTR>(local_func) > 
    reinterpret_cast<DWORD_PTR>(local_module.GetHandle()));
  
  auto const func_delta = reinterpret_cast<DWORD_PTR>(local_func) - 
    reinterpret_cast<DWORD_PTR>(local_module.GetHandle());

  HADESMEM_ASSERT(module.GetSize() > func_delta);

  auto const remote_func = reinterpret_cast<FARPROC>(
    reinterpret_cast<DWORD_PTR>(module.GetHandle()) + func_delta);
  
  return remote_func;
}

}

inline FARPROC FindProcedure(Module const& module, std::string const& name)
{
  return detail::FindProcedureInternal(module, name.c_str());
}

inline FARPROC FindProcedure(Module const& module, WORD ordinal)
{
  return detail::FindProcedureInternal(module, MAKEINTRESOURCEA(ordinal));
}

}
