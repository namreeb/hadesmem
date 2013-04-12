// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/detail/smart_handle.hpp>

#include <hadesmem/error.hpp>
#include <hadesmem/detail/assert.hpp>

// TODO: PImpl this class?

namespace hadesmem
{

namespace detail
{

SmartHandle::SmartHandle() HADESMEM_NOEXCEPT
  : handle_(nullptr), 
  invalid_(nullptr)
{ }

SmartHandle::SmartHandle(HANDLE handle) HADESMEM_NOEXCEPT
  : handle_(handle), 
  invalid_(nullptr)
{ }

SmartHandle::SmartHandle(HANDLE handle, HANDLE invalid) HADESMEM_NOEXCEPT
  : handle_(handle), 
  invalid_(invalid)
{ }

SmartHandle& SmartHandle::operator=(HANDLE handle) HADESMEM_NOEXCEPT
{
  CleanupUnchecked();

  handle_ = handle;

  return *this;
}

SmartHandle::SmartHandle(SmartHandle&& other) HADESMEM_NOEXCEPT
  : handle_(other.handle_), 
  invalid_(other.invalid_)
{
  other.handle_ = nullptr;
}

SmartHandle& SmartHandle::operator=(SmartHandle&& other) HADESMEM_NOEXCEPT
{
  CleanupUnchecked();

  handle_ = other.handle_;
  other.handle_ = other.invalid_;

  invalid_ = other.invalid_;

  return *this;
}

SmartHandle::~SmartHandle()
{
  CleanupUnchecked();
}

HANDLE SmartHandle::GetHandle() const HADESMEM_NOEXCEPT
{
  return handle_;
}

HANDLE SmartHandle::GetInvalid() const HADESMEM_NOEXCEPT
{
  return invalid_;
}

bool SmartHandle::IsValid() const HADESMEM_NOEXCEPT
{
  return GetHandle() != GetInvalid();
}

void SmartHandle::Cleanup()
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

void SmartHandle::CleanupUnchecked() HADESMEM_NOEXCEPT
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

}

}
