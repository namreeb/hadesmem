// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <type_traits>

#include <windows.h>
#include <tlhelp32.h>

#include <hadesmem/detail/optional.hpp>
#include <hadesmem/detail/smart_handle.hpp>
#include <hadesmem/detail/static_assert.hpp>
#include <hadesmem/error.hpp>

namespace hadesmem
{

namespace detail
{

inline detail::SmartSnapHandle CreateToolhelp32Snapshot(DWORD flags, DWORD pid)
{
  detail::SmartSnapHandle snap;
  do
  {
    snap = ::CreateToolhelp32Snapshot(flags, pid);
  } while (!snap.IsValid() && ::GetLastError() == ERROR_BAD_LENGTH);

  if (!snap.IsValid())
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error{} << ErrorString{"CreateToolhelp32Snapshot failed."}
              << ErrorCodeWinLast{last_error});
  }

  return snap;
}

template <typename Entry, typename Func>
hadesmem::detail::Optional<Entry>
  Toolhelp32Enum(Func func, HANDLE snap, std::string const& error)
{
  HADESMEM_DETAIL_STATIC_ASSERT(std::is_pod<Entry>::value);

  Entry entry;
  ::ZeroMemory(&entry, sizeof(entry));
  entry.dwSize = static_cast<DWORD>(sizeof(entry));
  if (!func(snap, &entry))
  {
    DWORD const last_error = ::GetLastError();
    if (last_error == ERROR_NO_MORE_FILES)
    {
      return hadesmem::detail::Optional<Entry>();
    }

    HADESMEM_DETAIL_THROW_EXCEPTION(Error{} << ErrorString{error.c_str()}
                                            << ErrorCodeWinLast{last_error});
  }

  return hadesmem::detail::Optional<Entry>(entry);
}

inline hadesmem::detail::Optional<MODULEENTRY32W> Module32First(HANDLE snap)
{
  return Toolhelp32Enum<MODULEENTRY32W, decltype(& ::Module32FirstW)>(
    &::Module32First, snap, "Module32First failed.");
}

inline hadesmem::detail::Optional<MODULEENTRY32W> Module32Next(HANDLE snap)
{
  return Toolhelp32Enum<MODULEENTRY32W, decltype(& ::Module32NextW)>(
    &::Module32NextW, snap, "Module32Next failed.");
}

inline hadesmem::detail::Optional<PROCESSENTRY32W> Process32First(HANDLE snap)
{
  return Toolhelp32Enum<PROCESSENTRY32W, decltype(& ::Process32FirstW)>(
    &::Process32FirstW, snap, "Process32First failed.");
}

inline hadesmem::detail::Optional<PROCESSENTRY32W> Process32Next(HANDLE snap)
{
  return Toolhelp32Enum<PROCESSENTRY32W, decltype(& ::Process32NextW)>(
    &::Process32NextW, snap, "Process32Next failed.");
}

inline hadesmem::detail::Optional<THREADENTRY32> Thread32First(HANDLE snap)
{
  return Toolhelp32Enum<THREADENTRY32, decltype(& ::Thread32First)>(
    &::Thread32First, snap, "Thread32First failed.");
}

inline hadesmem::detail::Optional<THREADENTRY32> Thread32Next(HANDLE snap)
{
  return Toolhelp32Enum<THREADENTRY32, decltype(& ::Thread32Next)>(
    &::Thread32Next, snap, "Thread32Next failed.");
}
}
}
