// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <cassert>

#include <windows.h>

#include "hadesmem/error.hpp"
#include "hadesmem/config.hpp"

namespace hadesmem
{

namespace detail
{

class SmartHandle
{
public:
  explicit SmartHandle(HANDLE handle = nullptr, HANDLE invalid = nullptr) 
    HADESMEM_NOEXCEPT
    : handle_(handle), 
    invalid_(invalid)
  { }

  SmartHandle& operator=(HANDLE handle) HADESMEM_NOEXCEPT
  {
    CleanupUnchecked();

    handle_ = handle;

    return *this;
  }

  SmartHandle(SmartHandle&& other) HADESMEM_NOEXCEPT
    : handle_(other.handle_), 
    invalid_(other.invalid_)
  {
    other.handle_ = nullptr;
  }

  SmartHandle& operator=(SmartHandle&& other) HADESMEM_NOEXCEPT
  {
    CleanupUnchecked();

    handle_ = other.handle_;
    other.handle_ = other.invalid_;

    invalid_ = other.invalid_;

    return *this;
  }

  ~SmartHandle()
  {
    CleanupUnchecked();
  }

  HANDLE GetHandle() const HADESMEM_NOEXCEPT
  {
    return handle_;
  }

  HANDLE GetInvalid() const HADESMEM_NOEXCEPT
  {
    return invalid_;
  }

  bool IsValid() const HADESMEM_NOEXCEPT
  {
    return GetHandle() != GetInvalid();
  }

  void Cleanup()
  {
    if (handle_ == invalid_)
    {
      return;
    }

    if (!::CloseHandle(handle_))
    {
      DWORD const last_error = ::GetLastError();
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorString("CloseHandle failed.") << 
        ErrorCodeWinLast(last_error));
    }

    handle_ = invalid_;
  }

private:
  SmartHandle(SmartHandle const& other) HADESMEM_DELETED_FUNCTION;
  SmartHandle& operator=(SmartHandle const& other) HADESMEM_DELETED_FUNCTION;

  void CleanupUnchecked() HADESMEM_NOEXCEPT
  {
    try
    {
      Cleanup();
    }
    catch (std::exception const& e)
    {
      (void)e;

      // WARNING: Handle is leaked if 'Cleanup' fails.
      assert(boost::diagnostic_information(e).c_str() && false);

      handle_ = invalid_;
    }
  }

  HANDLE handle_;
  HANDLE invalid_;
};

}

}
