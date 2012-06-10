// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <memory>
#include <iterator>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/config.hpp>
#include <boost/optional.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <windows.h>
#include <tlhelp32.h>

#include "hadesmem/module.hpp"

namespace hadesmem
{

class Process;
class ModuleList;

// TODO: Redesign to move most of the work and data to the ModuleList class.
// TODO: Redesign to avoid include dependency on hadesmem::Module and 
// boost::optional (via pimpl?).

// Inheriting from std::iterator causes the following warning under GCC:
// error: base class 'struct std::iterator<std::input_iterator_tag, 
// hadesmem::Module>' has a non-virtual destructor [-Werror=effc++]
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#endif // #if defined(HADESMEM_GCC)

class ModuleIter : public std::iterator<std::input_iterator_tag, Module>
{
public:
  ModuleIter() BOOST_NOEXCEPT;
  
  ModuleIter(Process const& process, MODULEENTRY32 const& entry, 
    ModuleList* back);
  
  ModuleIter(ModuleIter const& other);
  
  ModuleIter& operator=(ModuleIter const& other);
  
  Module const& operator*() const BOOST_NOEXCEPT;
  
  Module const* operator->() const BOOST_NOEXCEPT;
  
  ModuleIter& operator++();
  
  ModuleIter operator++(int);
  
  bool operator==(ModuleIter const& other) BOOST_NOEXCEPT;
  
  bool operator!=(ModuleIter const& other) BOOST_NOEXCEPT;
  
private:
  Process const* process_;
  ModuleList* back_;
  boost::optional<Module> module_;
};

#if defined(HADESMEM_GCC)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_GCC)

class ModuleList
{
public:
  typedef ModuleIter iterator;
  
  explicit ModuleList(Process const& process);
  
  ~ModuleList();
  
  iterator begin();
  
  iterator end();
  
private:
  ModuleList(ModuleList const&);
  ModuleList& operator=(ModuleList const&);
  
  friend class ModuleIter;
  
  boost::optional<MODULEENTRY32> Next();
  
  Process const* process_;
  HANDLE snap_;
};

}
