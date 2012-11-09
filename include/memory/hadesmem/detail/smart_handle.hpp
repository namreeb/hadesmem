// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <cassert>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/config.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <windows.h>

#include "hadesmem/error.hpp"

namespace hadesmem
{

namespace detail
{

class SmartHandle
{
public:
  explicit SmartHandle(HANDLE handle = nullptr, HANDLE invalid = nullptr) 
    BOOST_NOEXCEPT
    : handle_(handle), 
    invalid_(invalid)
  { }

  SmartHandle& operator=(HANDLE handle) BOOST_NOEXCEPT
  {
    CleanupUnchecked();

    handle_ = handle;

    return *this;
  }

  SmartHandle(SmartHandle&& other) BOOST_NOEXCEPT
    : handle_(other.handle_), 
    invalid_(other.invalid_)
  {
    other.handle_ = nullptr;
  }

  SmartHandle& operator=(SmartHandle&& other) BOOST_NOEXCEPT
  {
    CleanupUnchecked();

    handle_ = other.handle_;
    other.handle_ = other.invalid_;

    invalid_ = other.invalid_;

    return *this;
  }

  ~SmartHandle() BOOST_NOEXCEPT
  {
    CleanupUnchecked();
  }

  HANDLE GetHandle() const
  {
    return handle_;
  }

  HANDLE GetInvalid() const
  {
    return invalid_;
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
  SmartHandle(SmartHandle const& other);
  SmartHandle& operator=(SmartHandle const& other);

  void CleanupUnchecked() BOOST_NOEXCEPT
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
