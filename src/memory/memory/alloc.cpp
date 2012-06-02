// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/alloc.hpp"

#include <boost/assert.hpp>

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"

namespace hadesmem
{

PVOID Alloc(Process const& process, SIZE_T size)
{
  PVOID const address = ::VirtualAllocEx(process.GetHandle(), nullptr, 
    size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
  if (!address)
  {
    DWORD const last_error = GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("VirtualAllocEx failed.") << 
      ErrorCodeWinLast(last_error));
  }

  return address;
}

void Free(Process const& process, LPVOID address)
{
  if (!::VirtualFreeEx(process.GetHandle(), address, 0, MEM_RELEASE))
  {
    DWORD const last_error = GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("VirtualFreeEx failed.") << 
      ErrorCodeWinLast(last_error));
  }
}

Allocator::Allocator(Process const& process, SIZE_T size)
  : process_(&process), 
  base_(Alloc(process, size)), 
  size_(size)
{ }

Allocator::Allocator(Allocator&& other) BOOST_NOEXCEPT
  : process_(other.process_), 
  base_(other.base_), 
  size_(other.size_)
{
  other.process_ = nullptr;
  other.base_ = nullptr;
  other.size_ = 0;
}

Allocator& Allocator::operator=(Allocator&& other) BOOST_NOEXCEPT
{
  FreeUnchecked();
  
  std::swap(this->process_, other.process_);
  std::swap(this->base_, other.base_);
  std::swap(this->size_, other.size_);
  
  return *this;
}

Allocator::~Allocator() BOOST_NOEXCEPT
{
  FreeUnchecked();
}

void Allocator::Free()
{
  return ::hadesmem::Free(*process_, base_);
}

PVOID Allocator::GetBase() const BOOST_NOEXCEPT
{
  return base_;
}

SIZE_T Allocator::GetSize() const BOOST_NOEXCEPT
{
  return size_;

}

void Allocator::FreeUnchecked() BOOST_NOEXCEPT
{
  if (!process_)
  {
    return;
  }
  
  BOOST_VERIFY(::VirtualFreeEx(process_->GetHandle(), base_, 0, MEM_RELEASE));
  process_ = nullptr;
  base_ = nullptr;
  size_ = 0;
}

}
