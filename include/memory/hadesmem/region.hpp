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

// hadesmem::Region causes the following warning under GCC:
// error: 'class hadesmem::Module' has pointer data members 
// but does not override 'hadesmem::Module(const hadesmem::Module&)' 
// or 'operator=(const hadesmem::Module&)' [-Werror=effc++]
// This can be ignored because the pointer data members are non-owning 
// and shared pointers.
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#endif // #if defined(HADESMEM_GCC)

class Region
{
public:
  Region(Process const& process, LPCVOID address);
  
  Region(Process const& process, MEMORY_BASIC_INFORMATION const& mbi) BOOST_NOEXCEPT;
  
  PVOID GetBase() const BOOST_NOEXCEPT;
  
  PVOID GetAllocBase() const BOOST_NOEXCEPT;
  
  DWORD GetAllocProtect() const BOOST_NOEXCEPT;
  
  SIZE_T GetSize() const BOOST_NOEXCEPT;
  
  DWORD GetState() const BOOST_NOEXCEPT;
  
  DWORD GetProtect() const BOOST_NOEXCEPT;
  
  DWORD GetType() const BOOST_NOEXCEPT;
  
  bool operator==(Region const& other) const BOOST_NOEXCEPT;
  
  bool operator!=(Region const& other) const BOOST_NOEXCEPT;
  
  bool operator<(Region const& other) const BOOST_NOEXCEPT;
  
  bool operator<=(Region const& other) const BOOST_NOEXCEPT;
  
  bool operator>(Region const& other) const BOOST_NOEXCEPT;
  
  bool operator>=(Region const& other) const BOOST_NOEXCEPT;
  
private:
  Process const* process_;
  MEMORY_BASIC_INFORMATION mbi_;
};

#if defined(HADESMEM_GCC)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_GCC)

}
