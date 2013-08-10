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

template <typename Policy>
class SmartHandleImpl
{
public:
  typedef typename Policy::HandleT HandleT;

  HADESMEM_CONSTEXPR SmartHandleImpl() HADESMEM_NOEXCEPT
    : handle_(nullptr)
  { }
  
  explicit HADESMEM_CONSTEXPR SmartHandleImpl(HandleT handle) HADESMEM_NOEXCEPT
    : handle_(handle)
  { }

  SmartHandleImpl& operator=(HandleT handle) HADESMEM_NOEXCEPT
  {
    CleanupUnchecked();

    handle_ = handle;

    return *this;
  }

  SmartHandleImpl(SmartHandleImpl&& other) HADESMEM_NOEXCEPT
    : handle_(other.handle_)
  {
    other.handle_ = GetInvalid();
  }

  SmartHandleImpl& operator=(SmartHandleImpl&& other) HADESMEM_NOEXCEPT
  {
    CleanupUnchecked();

    handle_ = other.handle_;
    other.handle_ = other.GetInvalid();

    return *this;
  }

  ~SmartHandleImpl() HADESMEM_NOEXCEPT
  {
    CleanupUnchecked();
  }

  HandleT GetHandle() const HADESMEM_NOEXCEPT
  {
    return handle_;
  }

  HADESMEM_CONSTEXPR HandleT GetInvalid() const HADESMEM_NOEXCEPT
  {
    return Policy::GetInvalid();
  }

  bool IsValid() const HADESMEM_NOEXCEPT
  {
    return GetHandle() != GetInvalid();
  }

  void Cleanup()
  {
    if (handle_ == GetInvalid())
    {
      return;
    }

    if (!Policy::Cleanup(handle_))
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_THROW_EXCEPTION(Error() << 
        ErrorString("SmartHandle cleanup failed.") << 
        ErrorCodeWinLast(last_error));
    }

    handle_ = GetInvalid();
  }

private:
  SmartHandleImpl(SmartHandleImpl const& other) HADESMEM_DELETED_FUNCTION;
  SmartHandleImpl& operator=(SmartHandleImpl const& other) 
    HADESMEM_DELETED_FUNCTION;

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

      handle_ = GetInvalid();
    }
  }

  HandleT handle_;
  Policy policy_;
};

struct HandlePolicy
{
  typedef HANDLE HandleT;

  static HADESMEM_CONSTEXPR HandleT GetInvalid() HADESMEM_NOEXCEPT
  {
    return nullptr;
  }

  static bool Cleanup(HandleT handle)
  {
    return ::CloseHandle(handle) != 0;
  }
};

typedef SmartHandleImpl<HandlePolicy> SmartHandle;

struct SnapPolicy
{
  typedef HANDLE HandleT;

  static HADESMEM_CONSTEXPR HandleT GetInvalid() HADESMEM_NOEXCEPT
  {
    return INVALID_HANDLE_VALUE;
  }

  static bool Cleanup(HandleT handle)
  {
    return ::CloseHandle(handle) != 0;
  }
};

typedef SmartHandleImpl<SnapPolicy> SmartSnapHandle;

struct LibraryPolicy
{
  typedef HMODULE HandleT;

  static HADESMEM_CONSTEXPR HandleT GetInvalid() HADESMEM_NOEXCEPT
  {
    return nullptr;
  }

  static bool Cleanup(HandleT handle)
  {
    return ::FreeLibrary(handle) != 0;
  }
};

typedef SmartHandleImpl<LibraryPolicy> SmartModuleHandle;

}

}
