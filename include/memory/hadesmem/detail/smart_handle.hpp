// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>
#include <setupapi.h>
#include <wincrypt.h>

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

  constexpr SmartHandleImpl() noexcept
  {
  }

  explicit constexpr SmartHandleImpl(HandleT handle) noexcept : handle_{handle}
  {
  }

  SmartHandleImpl(SmartHandleImpl const& other) = delete;

  SmartHandleImpl& operator=(SmartHandleImpl const& other) = delete;

  SmartHandleImpl& operator=(HandleT handle) noexcept
  {
    CleanupUnchecked();

    handle_ = handle;

    return *this;
  }

  SmartHandleImpl(SmartHandleImpl&& other) noexcept : handle_{other.handle_}
  {
    other.handle_ = GetInvalid();
  }

  SmartHandleImpl& operator=(SmartHandleImpl&& other) noexcept
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

  HandleT GetHandle() const noexcept
  {
    return handle_;
  }

  HandleT GetInvalid() const noexcept
  {
    return Policy::GetInvalid();
  }

  bool IsValid() const noexcept
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
        Error{} << ErrorString{"SmartHandle cleanup failed."}
                << ErrorCodeWinLast{last_error});
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
  void CleanupUnchecked() noexcept
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

  HandleT handle_{GetInvalid()};
};

struct HandlePolicy
{
  using HandleT = HANDLE;

  static constexpr HandleT GetInvalid() noexcept
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

  static HandleT GetInvalid() noexcept
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
  using HandleT = HMODULE;

  static constexpr HandleT GetInvalid() noexcept
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

  static HandleT GetInvalid() noexcept
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

  static HandleT GetInvalid() noexcept
  {
    return INVALID_HANDLE_VALUE;
  }

  static bool Cleanup(HandleT handle)
  {
    return ::FindClose(handle) != 0;
  }
};

using SmartFindHandle = SmartHandleImpl<FindPolicy>;

struct FindVolumePolicy
{
  using HandleT = HANDLE;

  static HandleT GetInvalid() noexcept
  {
    return INVALID_HANDLE_VALUE;
  }

  static bool Cleanup(HandleT handle)
  {
    return ::FindVolumeClose(handle) != 0;
  }
};

using SmartFindVolumeHandle = SmartHandleImpl<FindVolumePolicy>;

struct ComPolicy
{
  using HandleT = IUnknown*;

  static HandleT GetInvalid() noexcept
  {
    return nullptr;
  }

  static bool Cleanup(HandleT handle)
  {
    handle->Release();
    return true;
  }
};

using SmartComHandle = SmartHandleImpl<ComPolicy>;

struct GdiObjectPolicy
{
  using HandleT = HGDIOBJ;

  static HandleT GetInvalid() noexcept
  {
    return nullptr;
  }

  static bool Cleanup(HandleT handle)
  {
    return ::DeleteObject(handle) != 0;
  }
};

using SmartGdiObjectHandle = SmartHandleImpl<GdiObjectPolicy>;

struct DeleteDcPolicy
{
  using HandleT = HDC;

  static HandleT GetInvalid() noexcept
  {
    return nullptr;
  }

  static bool Cleanup(HandleT handle)
  {
    return ::DeleteDC(handle) != 0;
  }
};

using SmartDeleteDcHandle = SmartHandleImpl<DeleteDcPolicy>;

struct VectoredExceptionHandlerPolicy
{
  using HandleT = PVOID;

  static HandleT GetInvalid() noexcept
  {
    return nullptr;
  }

  static bool Cleanup(HandleT handle)
  {
    return ::RemoveVectoredExceptionHandler(handle) != 0;
  }
};

using SmartRemoveVectoredExceptionHandler =
  SmartHandleImpl<VectoredExceptionHandlerPolicy>;

struct CryptContextPolicy
{
  using HandleT = HCRYPTPROV;

  static HandleT GetInvalid() noexcept
  {
    return 0;
  }

  static bool Cleanup(HandleT handle)
  {
    return !!::CryptReleaseContext(handle, 0);
  }
};

using SmartCryptContextHandle = SmartHandleImpl<CryptContextPolicy>;

struct DestroyHashPolicy
{
  using HandleT = HCRYPTPROV;

  static HandleT GetInvalid() noexcept
  {
    return 0;
  }

  static bool Cleanup(HandleT handle)
  {
    return !!::CryptDestroyHash(handle);
  }
};

using SmartCryptHashHandle = SmartHandleImpl<DestroyHashPolicy>;

struct UnmapViewOfFilePolicy
{
  using HandleT = LPVOID;

  static HandleT GetInvalid() noexcept
  {
    return 0;
  }

  static bool Cleanup(HandleT handle)
  {
    return !!::UnmapViewOfFile(handle);
  }
};

using SmartMappedFileHandle = SmartHandleImpl<UnmapViewOfFilePolicy>;

struct SetupDiDestroyDeviceInfoListPolicy
{
  using HandleT = HDEVINFO;

  static HandleT GetInvalid() noexcept
  {
    return INVALID_HANDLE_VALUE;
  }

  static bool Cleanup(HandleT handle)
  {
    return !!::SetupDiDestroyDeviceInfoList(handle);
  }
};

using SmartDeviceInfoListHandle =
  SmartHandleImpl<SetupDiDestroyDeviceInfoListPolicy>;

struct RegKeyPolicy
{
  using HandleT = HKEY;

  static HandleT GetInvalid() noexcept
  {
    return nullptr;
  }

  static bool Cleanup(HandleT handle)
  {
    return ::RegCloseKey(handle) == ERROR_SUCCESS;
  }
};

using SmartRegKeyHandle = SmartHandleImpl<RegKeyPolicy>;

struct VirtualMemPolicy
{
  using HandleT = PVOID;

  static HandleT GetInvalid() noexcept
  {
    return nullptr;
  }

  static bool Cleanup(HandleT handle)
  {
    return !!::VirtualFree(handle, 0, MEM_RELEASE);
  }
};

using SmartVirtualMemHandle = SmartHandleImpl<VirtualMemPolicy>;

struct EventLogPolicy
{
  using HandleT = HANDLE;

  static HandleT GetInvalid() noexcept
  {
    return nullptr;
  }

  static bool Cleanup(HandleT handle)
  {
    return !!::CloseEventLog(handle);
  }
};

using SmartEventLogHandle = SmartHandleImpl<EventLogPolicy>;

struct LocalFreePolicy
{
  using HandleT = PVOID;

  static HandleT GetInvalid() noexcept
  {
    return nullptr;
  }

  static bool Cleanup(HandleT handle)
  {
    // LocalFree returns NULL on success.
    return !::LocalFree(handle);
  }
};

using SmartLocalFreeHandle = SmartHandleImpl<LocalFreePolicy>;
}
}
