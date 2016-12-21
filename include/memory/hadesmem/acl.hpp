// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>
#include <aclapi.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/smart_handle.hpp>
#include <hadesmem/error.hpp>

namespace hadesmem
{
// TODO: Implement a less ghetto solution (use DeleteAce,
// AllocateAndInitializeSid, AddAccessAllowedAce, etc. to rebuild the DACL
// manually)?
void CloneDaclsToRemoteProcess(DWORD pid)
{
  HADESMEM_DETAIL_ASSERT(pid != 0);

  PACL dacl = nullptr;
  PSECURITY_DESCRIPTOR security_descriptor = nullptr;
  auto const get_sec_info_err = ::GetSecurityInfo(::GetCurrentProcess(),
                                                  SE_KERNEL_OBJECT,
                                                  DACL_SECURITY_INFORMATION,
                                                  nullptr,
                                                  nullptr,
                                                  &dacl,
                                                  nullptr,
                                                  &security_descriptor);
  if (get_sec_info_err != ERROR_SUCCESS)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"GetSecurityInfo failed."}
                        << hadesmem::ErrorCodeWinRet{get_sec_info_err});
  }
  hadesmem::detail::SmartLocalFreeHandle security_descriptor_cleanup(
    security_descriptor);

  hadesmem::detail::SmartHandle const proc(
    ::OpenProcess(WRITE_DAC, FALSE, pid));
  if (!proc.IsValid())
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"OpenProcess failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  auto const set_sec_info_err = ::SetSecurityInfo(proc.GetHandle(),
                                                  SE_KERNEL_OBJECT,
                                                  DACL_SECURITY_INFORMATION,
                                                  nullptr,
                                                  nullptr,
                                                  dacl,
                                                  nullptr);
  if (set_sec_info_err != ERROR_SUCCESS)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"SetSecurityInfo failed."}
                        << hadesmem::ErrorCodeWinRet{set_sec_info_err});
  }
}
}
