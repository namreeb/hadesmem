// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <functional>

#include <windows.h>

#include <hadesmem/config.hpp>

namespace hadesmem
{
namespace cerberus
{
typedef void
  OnCreateProcessInternalWCallback(HANDLE token,
                                   LPCWSTR application_name,
                                   LPWSTR command_line,
                                   LPSECURITY_ATTRIBUTES process_attributes,
                                   LPSECURITY_ATTRIBUTES thread_attributes,
                                   BOOL inherit_handles,
                                   DWORD creation_flags,
                                   LPVOID environment,
                                   LPCWSTR current_directory,
                                   LPSTARTUPINFOW startup_info,
                                   LPPROCESS_INFORMATION process_info,
                                   PHANDLE new_token,
                                   bool* handled,
                                   BOOL* retval);

class ProcessInterface
{
public:
  virtual ~ProcessInterface()
  {
  }

  virtual std::size_t RegisterOnCreateProcessInternalW(
    std::function<OnCreateProcessInternalWCallback> const& callback) = 0;

  virtual void UnregisterOnCreateProcessInternalW(std::size_t id) = 0;
};

ProcessInterface& GetProcessInterface() HADESMEM_DETAIL_NOEXCEPT;

void InitializeProcess();

void DetourKernelBaseForProcess(HMODULE base);

void UndetourKernelBaseForProcess(bool remove);
}
}
