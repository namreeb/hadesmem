// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/detail/assert.hpp>

namespace hadesmem
{

namespace detail
{

// TODO: Turn this into a template like the old EnsureCleanup class template?
class SmartHandle
{
public:
  HADESMEM_CONSTEXPR SmartHandle() HADESMEM_NOEXCEPT
    : handle_(nullptr), 
    invalid_(nullptr)
  { }
  
  explicit HADESMEM_CONSTEXPR SmartHandle(HANDLE handle) HADESMEM_NOEXCEPT
    : handle_(handle), 
    invalid_(nullptr)
  { }

  explicit HADESMEM_CONSTEXPR SmartHandle(HANDLE handle, HANDLE invalid) HADESMEM_NOEXCEPT
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

  ~SmartHandle() HADESMEM_NOEXCEPT
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
      HADESMEM_THROW_EXCEPTION(Error() << 
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
      HADESMEM_ASSERT(boost::diagnostic_information(e).c_str() && false);

      handle_ = invalid_;
    }
  }

  HANDLE handle_;
  HANDLE invalid_;
};

}

}
