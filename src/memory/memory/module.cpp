// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/module.hpp>

#include <ostream>
#include <utility>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/filesystem.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/smart_handle.hpp>
#include <hadesmem/detail/to_upper_ordinal.hpp>

namespace hadesmem
{

namespace
{

// TODO: Replace this with something more generic and robust.
class Library
{
public:
  explicit HADESMEM_CONSTEXPR Library(HMODULE module) HADESMEM_NOEXCEPT
    : module_(module)
  { }

  ~Library() HADESMEM_NOEXCEPT
  {
    if (module_)
    {
        HADESMEM_VERIFY(::FreeLibrary(module_));
    }
  }

  HMODULE Get() const
  {
    return module_;
  }

private:
  Library(Library const& other);
  Library& operator=(Library const& other);

  HMODULE module_;
};

FARPROC FindProcedureInternal(Module const& module, LPCSTR name)
{
  HADESMEM_ASSERT(name != nullptr);

  // Do not continue if Shim Engine is enabled for local process, 
  // otherwise it could interfere with the address resolution.
  // TODO: Work around this with 'manual' export lookup or similar.
  HMODULE const shim_eng_mod = ::GetModuleHandle(L"ShimEng.dll");
  if (shim_eng_mod)
  {
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Shims enabled for local process."));
  }

  Library const local_module(::LoadLibraryEx(module.GetPath().c_str(), 
    nullptr, DONT_RESOLVE_DLL_REFERENCES));
  if (!local_module.Get())
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("LoadLibraryEx failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  FARPROC const local_func = ::GetProcAddress(local_module.Get(), name);
  if (!local_func)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("GetProcAddress failed.") << 
      ErrorCodeWinLast(last_error));
  }

  HADESMEM_ASSERT(reinterpret_cast<DWORD_PTR>(local_func) > 
    reinterpret_cast<DWORD_PTR>(local_module.Get()));
  
  auto const func_delta = reinterpret_cast<DWORD_PTR>(local_func) - 
    reinterpret_cast<DWORD_PTR>(local_module.Get());

  HADESMEM_ASSERT(module.GetSize() > func_delta);

  auto const remote_func = reinterpret_cast<FARPROC>(
    reinterpret_cast<DWORD_PTR>(module.GetHandle()) + func_delta);
  
  return remote_func;
}

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

void Module::InitializeIf(EntryCallback const& check_func)
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



FARPROC FindProcedure(Module const& module, std::string const& name)
{
  return FindProcedureInternal(module, name.c_str());
}

FARPROC FindProcedure(Module const& module, WORD ordinal)
{
  return FindProcedureInternal(module, MAKEINTRESOURCEA(ordinal));
}

}
