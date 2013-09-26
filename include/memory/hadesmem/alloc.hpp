// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <ostream>
#include <utility>

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>

namespace hadesmem
{
  
inline PVOID Alloc(Process const& process, SIZE_T size)
{
  PVOID const address = ::VirtualAllocEx(process.GetHandle(), nullptr, 
    size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
  if (!address)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
      ErrorString("VirtualAllocEx failed.") << 
      ErrorCodeWinLast(last_error));
  }

  return address;
}

inline PVOID Alloc(Process const& process, PVOID base, SIZE_T size)
{
  PVOID const address = ::VirtualAllocEx(process.GetHandle(), base, 
    size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
  if (!address)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
      ErrorString("VirtualAllocEx failed.") << 
      ErrorCodeWinLast(last_error));
  }

  return address;
}

inline void Free(Process const& process, LPVOID address)
{
  if (!::VirtualFreeEx(process.GetHandle(), address, 0, MEM_RELEASE))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
      ErrorString("VirtualFreeEx failed.") << 
      ErrorCodeWinLast(last_error));
  }
}

class Allocator
{
public:
  explicit Allocator(Process const& process, SIZE_T size)
    : process_(&process), 
    base_(Alloc(process, size)), 
    size_(size)
  {
    HADESMEM_DETAIL_ASSERT(process_ != 0);
    HADESMEM_DETAIL_ASSERT(base_ != 0);
    HADESMEM_DETAIL_ASSERT(size_ != 0);
  }
  
  explicit Allocator(Process const& process, PVOID base, SIZE_T size)
    : process_(&process), 
    base_(Alloc(process, base, size)), 
    size_(size)
  {
    HADESMEM_DETAIL_ASSERT(process_ != 0);
    HADESMEM_DETAIL_ASSERT(base_ != 0);
    HADESMEM_DETAIL_ASSERT(size_ != 0);
  }
  
  Allocator(Allocator&& other) HADESMEM_DETAIL_NOEXCEPT
    : process_(other.process_), 
    base_(other.base_), 
    size_(other.size_)
  {
    other.process_ = nullptr;
    other.base_ = nullptr;
    other.size_ = 0;
  }
  
  Allocator& operator=(Allocator&& other) HADESMEM_DETAIL_NOEXCEPT
  {
    FreeUnchecked();

    process_ = other.process_;
    other.process_ = nullptr;

    base_ = other.base_;
    other.base_ = nullptr;
  
    size_ = other.size_;
    other.size_ = 0;
  
    return *this;
  }
  
  ~Allocator() HADESMEM_DETAIL_NOEXCEPT
  {
    FreeUnchecked();
  }
  
  void Free()
  {
    if (!process_)
    {
      return;
    }

    HADESMEM_DETAIL_ASSERT(base_ != nullptr);
    HADESMEM_DETAIL_ASSERT(size_ != 0);

    ::hadesmem::Free(*process_, base_);

    process_ = nullptr;
    base_ = nullptr;
    size_ = 0;
  }
  
  PVOID GetBase() const HADESMEM_DETAIL_NOEXCEPT
  {
    return base_;
  }

  SIZE_T GetSize() const HADESMEM_DETAIL_NOEXCEPT
  {
    return size_;
  }

private:
  Allocator(Allocator const& other) HADESMEM_DETAIL_DELETED_FUNCTION;
  Allocator& operator=(Allocator const& other) HADESMEM_DETAIL_DELETED_FUNCTION;

  void FreeUnchecked() HADESMEM_DETAIL_NOEXCEPT
  {
    try
    {
      Free();
    }
    catch (...)
    {
      // WARNING: Memory in remote process is leaked if 'Free' fails
      HADESMEM_DETAIL_TRACE_A(
        boost::current_exception_diagnostic_information().c_str());
      HADESMEM_DETAIL_ASSERT(false);

      process_ = nullptr;
      base_ = nullptr;
      size_ = 0;
    }
  }
  
  Process const* process_;
  PVOID base_;
  SIZE_T size_;
};

inline bool operator==(Allocator const& lhs, Allocator const& rhs) 
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() == rhs.GetBase();
}

inline bool operator!=(Allocator const& lhs, Allocator const& rhs) 
  HADESMEM_DETAIL_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(Allocator const& lhs, Allocator const& rhs) 
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() < rhs.GetBase();
}

inline bool operator<=(Allocator const& lhs, Allocator const& rhs) 
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() <= rhs.GetBase();
}

inline bool operator>(Allocator const& lhs, Allocator const& rhs) 
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() > rhs.GetBase();
}

inline bool operator>=(Allocator const& lhs, Allocator const& rhs) 
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetBase() >= rhs.GetBase();
}

inline std::ostream& operator<<(std::ostream& lhs, Allocator const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << static_cast<void*>(rhs.GetBase());
  lhs.imbue(old);
  return lhs;
}

inline std::wostream& operator<<(std::wostream& lhs, Allocator const& rhs)
{
  std::locale const old = lhs.imbue(std::locale::classic());
  lhs << static_cast<void*>(rhs.GetBase());
  lhs.imbue(old);
  return lhs;
}

}
