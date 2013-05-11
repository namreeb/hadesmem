// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstddef>

#include <windows.h>

#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/protect.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/type_traits.hpp>
#include <hadesmem/detail/query_region.hpp>
#include <hadesmem/detail/protect_guard.hpp>

namespace hadesmem
{

namespace detail
{

inline void ReadUnchecked(Process const& process, LPVOID address, 
  LPVOID data, std::size_t len)
{
  HADESMEM_ASSERT(address != nullptr);
  HADESMEM_ASSERT(data != nullptr);
  HADESMEM_ASSERT(len != 0);
  
  SIZE_T bytes_read = 0;
  if (!::ReadProcessMemory(process.GetHandle(), address, data, len, 
    &bytes_read) || bytes_read != len)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Could not read process memory.") << 
      ErrorCodeWinLast(last_error));
  }
}

inline void Read(Process const& process, LPVOID address, LPVOID data, 
  std::size_t len)
{
  HADESMEM_ASSERT(address != nullptr);
  HADESMEM_ASSERT(data != nullptr);
  HADESMEM_ASSERT(len != 0);

  ProtectGuard protect_guard(process, address, ProtectGuardType::kRead);

  ReadUnchecked(process, address, data, len);

  protect_guard.Restore();
}

template <typename T>
T Read(Process const& process, PVOID address)
{
  HADESMEM_STATIC_ASSERT(detail::IsTriviallyCopyable<T>::value);
  HADESMEM_STATIC_ASSERT(detail::IsDefaultConstructible<T>::value);
  
  HADESMEM_ASSERT(address != nullptr);

  T data;
  Read(process, address, std::addressof(data), sizeof(data));
  return data;
}

}

}
