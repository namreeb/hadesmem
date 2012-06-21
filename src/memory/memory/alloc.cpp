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
  
  BOOST_ASSERT(process_ == nullptr);
  BOOST_ASSERT(base_ == nullptr);
  BOOST_ASSERT(size_ == 0);
  
  process_ = other.process_;
  base_ = other.base_;
  size_ = other.size_;
  
  other.process_ = nullptr;
  other.base_ = nullptr;
  other.size_ = 0;
  
  return *this;
}

Allocator::~Allocator()
{
  FreeUnchecked();
}

void Allocator::Free()
{
  if (!process_)
  {
    return;
  }
  
  BOOST_ASSERT(base_);
  BOOST_ASSERT(size_);
  
  ::hadesmem::Free(*process_, base_);
  
  process_ = nullptr;
  base_ = nullptr;
  size_ = 0;
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
  try
  {
    Free();
  }
  catch (std::exception const& e)
  {
    (void)e;
    
    // WARNING: Memory in remote process is leaked if 'Free' fails, but 
    // unfortunately there's nothing that can be done about it...
    BOOST_ASSERT_MSG(false, boost::diagnostic_information(e).c_str());
    
    process_ = nullptr;
    base_ = nullptr;
    size_ = 0;
  }
}

}
