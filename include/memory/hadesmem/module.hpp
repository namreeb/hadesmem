// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <string>
#include <cstring>
#include <ostream>
#include <utility>
#include <functional>


#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/locale.hpp>
#include <boost/filesystem.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <windows.h>
#include <tlhelp32.h>

#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/pelib/export.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/pelib/pe_file.hpp>
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

// TODO: Split this into a different file.
namespace detail
{

// TODO: Support exports by ordinal, add a new overload and change 
// FindProcedure to use it.
inline FARPROC GetProcAddressInternal(Process const& process, 
  HMODULE const& module, std::string const& export_name)
{
  HADESMEM_DETAIL_STATIC_ASSERT(sizeof(FARPROC) == sizeof(void*));

  PeFile const pe_file(process, module, PeFileType::Image);
  
  ExportList const exports(process, pe_file);
  for (auto const& e : exports)
  {
    if (e.ByName() && e.GetName() == export_name)
    {
      if (e.IsForwarded())
      {
        Module const forwarder_module(process, 
          boost::locale::conv::utf_to_utf<wchar_t>(e.GetForwarderModule()));
        return GetProcAddressInternal(process, forwarder_module.GetHandle(), 
          e.GetForwarderFunction());
      }

      void* va = e.GetVa();
      FARPROC pfn = nullptr;
      std::memcpy(&pfn, &va, sizeof(void*));

      return pfn;
    }
  }

  return nullptr;
}

inline FARPROC FindProcedureInternal(
  Process const& process, 
  HMODULE module, 
  LPCSTR name)
{
  HADESMEM_DETAIL_ASSERT(name != nullptr);

  FARPROC const remote_func = GetProcAddressInternal(
    process, module, name);
  if (!remote_func)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("GetProcAddressInternal failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  return remote_func;
}

}

inline FARPROC FindProcedure(
  Process const& process, 
  Module const& module, 
  std::string const& name)
{
  return detail::FindProcedureInternal(
    process, 
    module.GetHandle(), 
    name.c_str());
}

inline FARPROC FindProcedure(
  Process const& process, 
  Module const& module, 
  WORD ordinal)
{
  return detail::FindProcedureInternal(
    process, 
    module.GetHandle(), 
    MAKEINTRESOURCEA(ordinal));
}

}
