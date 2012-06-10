// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <memory>
#include <iterator>

#include <windows.h>
#include <tlhelp32.h>

namespace hadesmem
{

class Process;

class Module;

class ModuleList;

class ModuleIter : public std::iterator<std::input_iterator_tag, Module>
{
public:
  ModuleIter();
  
  ModuleIter(Process const& process, MODULEENTRY32 const& entry, 
    ModuleList* back);
  
  ModuleIter(ModuleIter const& other);
  
  ModuleIter& operator=(ModuleIter const& other);
  
  Module const& operator*() const;
  
  Module const* operator->() const;
  
  ModuleIter& operator++();
  
  ModuleIter operator++(int);
  
  bool equal(ModuleIter const& other) const;
  
private:
  Process const* process_;
  ModuleList* back_;
  std::unique_ptr<Module> module_;
};

bool operator==(ModuleIter const& lhs, ModuleIter const& rhs);

bool operator!=(ModuleIter const& lhs, ModuleIter const& rhs);

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
  
  std::unique_ptr<MODULEENTRY32> Next();
  
  Process const* process_;
  HANDLE snap_;
};

}
