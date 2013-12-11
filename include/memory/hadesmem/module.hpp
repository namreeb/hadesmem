// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstring>
#include <functional>
#include <ostream>
#include <string>
#include <utility>

#include <windows.h>
#include <tlhelp32.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/filesystem.hpp>
#include <hadesmem/detail/smart_handle.hpp>
#include <hadesmem/detail/toolhelp.hpp>
#include <hadesmem/detail/to_upper_ordinal.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/pelib/export.hpp>
#include <hadesmem/pelib/export_list.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>

// TODO: Remove the constructor that takes a name/path because it is
// functionality that belongs in (and already exists in) ModuleList. Perhaps
// provide a factory instead?

// TODO: When finding a module by name, allow disambiguation by checking the
// headers (like the Windows loader does). Given that the name/path
// functinoality will be 'removed' this will probably need to be implemented
// as a free func designed to be used with ModuleList. Needs more thought...

namespace hadesmem
{

class Module
{
public:
  explicit Module(Process const& process, HMODULE handle)
    : process_(&process), handle_(nullptr), size_(0), name_(), path_()
  {
    Initialize(handle);
  }

  explicit Module(Process const& process, std::wstring const& path)
    : process_(&process), handle_(nullptr), size_(0), name_(), path_()
  {
    Initialize(path);
  }

#if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

  Module(Module const&) = default;

  Module& operator=(Module const&) = default;

  Module(Module&& other)
    : process_(other.process_),
      handle_(other.handle_),
      size_(other.size_),
      name_(std::move(other.name_)),
      path_(std::move(other.path_))
  {
  }

  Module& operator=(Module&& other)
  {
    process_ = other.process_;
    handle_ = other.handle_;
    size_ = other.size_;
    name_ = std::move(other.name_);
    path_ = std::move(other.path_);

    return *this;
  }

#endif // #if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

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
  template <typename ModuleT> friend class ModuleIterator;

  using EntryCallback = std::function<bool(MODULEENTRY32 const&)>;

  explicit Module(Process const& process, MODULEENTRY32 const& entry)
    : process_(&process), handle_(nullptr), size_(0), name_(), path_()
  {
    Initialize(entry);
  }

  void Initialize(HMODULE handle)
  {
    auto const handle_check = [&](MODULEENTRY32 const & entry)->bool
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

    auto const path_check = [&](MODULEENTRY32 const & entry)->bool
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
    detail::SmartSnapHandle const snap(
      detail::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, process_->GetId()));

    hadesmem::detail::Optional<MODULEENTRY32> entry;
    for (entry = detail::Module32First(snap.GetHandle()); entry;
         entry = detail::Module32Next(snap.GetHandle()))
    {
      if (check_func(*entry))
      {
        Initialize(*entry);
        return;
      }
    }

    HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                    << ErrorString("Could not find module."));
  }

  Process const* process_;
  HMODULE handle_;
  DWORD size_;
  std::wstring name_;
  std::wstring path_;
};

inline bool operator==(Module const& lhs, Module const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetHandle() == rhs.GetHandle();
}

inline bool operator!=(Module const& lhs, Module const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(Module const& lhs, Module const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetHandle() < rhs.GetHandle();
}

inline bool operator<=(Module const& lhs, Module const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetHandle() <= rhs.GetHandle();
}

inline bool operator>(Module const& lhs, Module const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetHandle() > rhs.GetHandle();
}

inline bool operator>=(Module const& lhs, Module const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetHandle() >= rhs.GetHandle();
}

inline std::ostream& operator<<(std::ostream& lhs, Module const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << static_cast<void*>(rhs.GetHandle());
  lhs.imbue(old);
  return lhs;
}

inline std::wostream& operator<<(std::wostream& lhs, Module const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << static_cast<void*>(rhs.GetHandle());
  lhs.imbue(old);
  return lhs;
}
}
