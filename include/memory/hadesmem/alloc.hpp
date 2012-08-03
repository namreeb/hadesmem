// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <boost/config.hpp>

#include <windows.h>

namespace hadesmem
{

class Process;

PVOID Alloc(Process const& process, SIZE_T size);

void Free(Process const& process, LPVOID address);

// TODO: Equality and inequality operator overloads for Allocator.
// TODO: Relational operator overloads for Allocator.
// TODO: Stream operator overloads for Allocator.

class Allocator
{
public:
  Allocator(Process const* process, SIZE_T size);
  
  Allocator(Allocator&& other) BOOST_NOEXCEPT;
  
  Allocator& operator=(Allocator&& other) BOOST_NOEXCEPT;
  
  ~Allocator();
  
  void Free();
  
  PVOID GetBase() const BOOST_NOEXCEPT;
  
  SIZE_T GetSize() const BOOST_NOEXCEPT;
  
private:
  Allocator(Allocator const& other);
  Allocator& operator=(Allocator const& other);
  
  void FreeUnchecked() BOOST_NOEXCEPT;
  
  Process const* process_;
  PVOID base_;
  SIZE_T size_;
};

}
