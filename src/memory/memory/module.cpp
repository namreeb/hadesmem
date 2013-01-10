// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/module.hpp"

#include <utility>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <boost/scope_exit.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/detail/smart_handle.hpp"
#include "hadesmem/detail/to_upper_ordinal.hpp"

namespace hadesmem
{

namespace
{

FARPROC FindProcedureInternal(Module const& module, LPCSTR name)
{
  BOOST_ASSERT(name != nullptr);

  // Do not continue if Shim Engine is enabled for local process, 
  // otherwise it could interfere with the address resolution.
  // TODO: Work around this with 'manual' export lookup or similar.
  HMODULE const shim_eng_mod = ::GetModuleHandle(L"ShimEng.dll");
  if (shim_eng_mod)
  {
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Shims enabled for local process."));
  }

  HMODULE const local_module = ::LoadLibraryEx(module.GetPath().c_str(), 
    nullptr, DONT_RESOLVE_DLL_REFERENCES);
  if (!local_module)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("LoadLibraryEx failed.") << 
      ErrorCodeWinLast(last_error));
  }

  BOOST_SCOPE_EXIT_ALL(&)
  {
    BOOST_VERIFY(::FreeLibrary(local_module));
  };
  
  FARPROC const local_func = ::GetProcAddress(local_module, name);
  if (!local_func)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("GetProcAddress failed.") << 
      ErrorCodeWinLast(last_error));
  }

  BOOST_ASSERT(reinterpret_cast<DWORD_PTR>(local_func) > 
    reinterpret_cast<DWORD_PTR>(local_module));
  
  auto const func_delta = reinterpret_cast<DWORD_PTR>(local_func) - 
    reinterpret_cast<DWORD_PTR>(local_module);

  BOOST_ASSERT(module.GetSize() > func_delta);

  auto const remote_func = reinterpret_cast<FARPROC>(
    reinterpret_cast<DWORD_PTR>(module.GetHandle()) + func_delta);
  
  return remote_func;
}

}

struct Module::Impl
{
  explicit Impl(Process const& process, HMODULE handle)
    : process_(&process), 
    handle_(nullptr), 
    size_(0), 
    name_(), 
    path_()
  {
    Initialize(handle);
  }

  explicit Impl(Process const& process, std::wstring const& path)
    : process_(&process), 
    handle_(nullptr), 
    size_(0), 
    name_(), 
    path_()
  {
    Initialize(path);
  }

  explicit Impl(Process const& process, MODULEENTRY32 const& entry)
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

  typedef std::function<bool (MODULEENTRY32 const&)> EntryCallback;
  void InitializeIf(EntryCallback const& check_func)
  {
    detail::SmartHandle snap(::CreateToolhelp32Snapshot(
      TH32CS_SNAPMODULE, process_->GetId()), INVALID_HANDLE_VALUE);
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

Module::Module(Process const& process, HMODULE handle)
  : impl_(new Impl(process, handle))
{ }

Module::Module(Process const& process, std::wstring const& path)
  : impl_(new Impl(process, path))
{ }

Module::Module(Process const& process, MODULEENTRY32 const& entry)
  : impl_(new Impl(process, entry))
{ }

Module::Module(Module const& other)
  : impl_(new Impl(*other.impl_))
{ }

Module& Module::operator=(Module const& other)
{
  impl_ = std::unique_ptr<Impl>(new Impl(*other.impl_));

  return *this;
}

Module::Module(Module&& other) HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

Module& Module::operator=(Module&& other) HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);

  return *this;
}

Module::~Module()
{ }

HMODULE Module::GetHandle() const HADESMEM_NOEXCEPT
{
  return impl_->handle_;
}

DWORD Module::GetSize() const HADESMEM_NOEXCEPT
{
  return impl_->size_;
}

std::wstring Module::GetName() const
{
  return impl_->name_;
}

std::wstring Module::GetPath() const
{
  return impl_->path_;
}

bool operator==(Module const& lhs, Module const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetHandle() == rhs.GetHandle();
}

bool operator!=(Module const& lhs, Module const& rhs) HADESMEM_NOEXCEPT
{
  return !(lhs == rhs);
}

bool operator<(Module const& lhs, Module const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetHandle() < rhs.GetHandle();
}

bool operator<=(Module const& lhs, Module const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetHandle() <= rhs.GetHandle();
}

bool operator>(Module const& lhs, Module const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetHandle() > rhs.GetHandle();
}

bool operator>=(Module const& lhs, Module const& rhs) HADESMEM_NOEXCEPT
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
