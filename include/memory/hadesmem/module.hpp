// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <string>
#include <utility>
#include <iostream>
#include <functional>

#include <windows.h>
#include <tlhelp32.h>

#include <hadesmem/config.hpp>

namespace hadesmem
{

class Process;

class Module
{
public:
  explicit Module(Process const& process, HMODULE handle)
    : process_(&process), 
    handle_(nullptr), 
    size_(0), 
    name_(), 
    path_()
  {
    Initialize(handle);
  }
  
  explicit Module(Process const& process, std::wstring const& path)
    : process_(&process), 
    handle_(nullptr), 
    size_(0), 
    name_(), 
    path_()
  {
    Initialize(path);
  }
  
  Module(Module const& other)
    : process_(other.process_), 
    handle_(other.handle_), 
    size_(other.size_), 
    name_(other.name_), 
    path_(other.path_)
  { }

  Module& operator=(Module const& other)
{
  process_ = other.process_;
  handle_ = other.handle_;
  size_ = other.size_;
  name_ = other.name_;
  path_ = other.path_;

  return *this;
}

  Module(Module&& other) HADESMEM_NOEXCEPT
    : process_(other.process_), 
    handle_(other.handle_), 
    size_(other.size_), 
    name_(std::move(other.name_)), 
    path_(std::move(other.path_))
  { }

  Module& operator=(Module&& other) HADESMEM_NOEXCEPT
  {
    process_ = other.process_;
    handle_ = other.handle_;
    size_ = other.size_;
    name_ = std::move(other.name_);
    path_ = std::move(other.path_);

    return *this;
  }

  ~Module() HADESMEM_NOEXCEPT
  { }

  HMODULE GetHandle() const HADESMEM_NOEXCEPT
  {
    return handle_;
  }
  
  DWORD GetSize() const HADESMEM_NOEXCEPT
  {
    return size_;
  }
  
  std::wstring GetName() const
  {
    return name_;
  }
  
  std::wstring GetPath() const
  {
    return path_;
  }
  
private:
  friend class ModuleIterator;

  typedef std::function<bool (MODULEENTRY32 const&)> EntryCallback;

  explicit Module(Process const& process, MODULEENTRY32 const& entry)
  : process_(&process), 
  handle_(nullptr), 
  size_(0), 
  name_(), 
  path_()
{
  Initialize(entry);
}
  
  void Initialize(HMODULE handle);
  
  void Initialize(std::wstring const& path);
  
  void Initialize(MODULEENTRY32 const& entry)
  {
    handle_ = entry.hModule;
    size_ = entry.modBaseSize;
    name_ = entry.szModule;
    path_ = entry.szExePath;
  }

  void InitializeIf(EntryCallback const& check_func);

  Process const* process_;
  HMODULE handle_;
  DWORD size_;
  std::wstring name_;
  std::wstring path_;
};

inline bool operator==(Module const& lhs, Module const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetHandle() == rhs.GetHandle();
}

inline bool operator!=(Module const& lhs, Module const& rhs) HADESMEM_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(Module const& lhs, Module const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetHandle() < rhs.GetHandle();
}

inline bool operator<=(Module const& lhs, Module const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetHandle() <= rhs.GetHandle();
}

inline bool operator>(Module const& lhs, Module const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetHandle() > rhs.GetHandle();
}

inline bool operator>=(Module const& lhs, Module const& rhs) HADESMEM_NOEXCEPT
{
  return lhs.GetHandle() >= rhs.GetHandle();
}

inline std::ostream& operator<<(std::ostream& lhs, Module const& rhs)
{
  return (lhs << rhs.GetHandle());
}

inline std::wostream& operator<<(std::wostream& lhs, Module const& rhs)
{
  return (lhs << rhs.GetHandle());
}

FARPROC FindProcedure(Module const& module, std::string const& name);

FARPROC FindProcedure(Module const& module, WORD ordinal);

}
