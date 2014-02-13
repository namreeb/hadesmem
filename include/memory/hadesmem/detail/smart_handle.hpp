// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/error.hpp>

namespace hadesmem
{

namespace detail
{

template <typename Policy> class SmartHandleImpl
{
public:
  using HandleT = typename Policy::HandleT;

  HADESMEM_DETAIL_CONSTEXPR SmartHandleImpl() HADESMEM_DETAIL_NOEXCEPT
    : handle_(GetInvalid())
  {
  }

  explicit HADESMEM_DETAIL_CONSTEXPR
    SmartHandleImpl(HandleT handle) HADESMEM_DETAIL_NOEXCEPT : handle_(handle)
  {
  }

  SmartHandleImpl(SmartHandleImpl const& other) = delete;

  SmartHandleImpl& operator=(SmartHandleImpl const& other) = delete;

  SmartHandleImpl& operator=(HandleT handle) HADESMEM_DETAIL_NOEXCEPT
  {
    CleanupUnchecked();

    handle_ = handle;

    return *this;
  }

  SmartHandleImpl(SmartHandleImpl&& other) HADESMEM_DETAIL_NOEXCEPT
    : handle_(other.handle_)
  {
    other.handle_ = GetInvalid();
  }

  SmartHandleImpl& operator=(SmartHandleImpl&& other) HADESMEM_DETAIL_NOEXCEPT
  {
    CleanupUnchecked();

    handle_ = other.handle_;
    other.handle_ = other.GetInvalid();

    return *this;
  }

  ~SmartHandleImpl()
  {
    CleanupUnchecked();
  }

  HandleT GetHandle() const HADESMEM_DETAIL_NOEXCEPT
  {
    return handle_;
  }

  HandleT GetInvalid() const HADESMEM_DETAIL_NOEXCEPT
  {
    return Policy::GetInvalid();
  }

  bool IsValid() const HADESMEM_DETAIL_NOEXCEPT
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
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("SmartHandle cleanup failed.")
                << ErrorCodeWinLast(last_error));
    }

    handle_ = GetInvalid();
  }

  // WARNING: This detaches the handle from the smart handle. The
  // caller is responsible for managing the lifetime of the handle
  // after this point.
  HandleT Detach()
  {
    HandleT const handle = handle_;
    handle_ = GetInvalid();
    return handle;
  }

private:
  void CleanupUnchecked() HADESMEM_DETAIL_NOEXCEPT
  {
    try
    {
      Cleanup();
    }
    catch (...)
    {
      // WARNING: Handle is leaked if 'Cleanup' fails.
      HADESMEM_DETAIL_TRACE_A(
        boost::current_exception_diagnostic_information().c_str());
      HADESMEM_DETAIL_ASSERT(false);

      handle_ = GetInvalid();
    }
  }

  HandleT handle_;
};

struct HandlePolicy
{
  using HandleT = HANDLE;

  static HADESMEM_DETAIL_CONSTEXPR HandleT GetInvalid() HADESMEM_DETAIL_NOEXCEPT
  {
    return nullptr;
  }

  static bool Cleanup(HandleT handle)
  {
    return ::CloseHandle(handle) != 0;
  }
};

using SmartHandle = SmartHandleImpl<HandlePolicy>;

struct SnapPolicy
{
  using HandleT = HANDLE;

  static HandleT GetInvalid() HADESMEM_DETAIL_NOEXCEPT
  {
    return INVALID_HANDLE_VALUE;
  }

  static bool Cleanup(HandleT handle)
  {
    return ::CloseHandle(handle) != 0;
  }
};

using SmartSnapHandle = SmartHandleImpl<SnapPolicy>;

struct LibraryPolicy
{
  typedef HMODULE HandleT;

  static HADESMEM_DETAIL_CONSTEXPR HandleT GetInvalid() HADESMEM_DETAIL_NOEXCEPT
  {
    return nullptr;
  }

  static bool Cleanup(HandleT handle)
  {
    return ::FreeLibrary(handle) != 0;
  }
};

using SmartModuleHandle = SmartHandleImpl<LibraryPolicy>;

struct FilePolicy
{
  using HandleT = HANDLE;

  static HandleT GetInvalid() HADESMEM_DETAIL_NOEXCEPT
  {
    return INVALID_HANDLE_VALUE;
  }

  static bool Cleanup(HandleT handle)
  {
    return ::CloseHandle(handle) != 0;
  }
};

using SmartFileHandle = SmartHandleImpl<FilePolicy>;

struct FindPolicy
{
  using HandleT = HANDLE;

  static HandleT GetInvalid() HADESMEM_DETAIL_NOEXCEPT
  {
    return INVALID_HANDLE_VALUE;
  }

  static bool Cleanup(HandleT handle)
  {
    return ::FindClose(handle) != 0;
  }
};

using SmartFindHandle = SmartHandleImpl<FindPolicy>;
}
}
