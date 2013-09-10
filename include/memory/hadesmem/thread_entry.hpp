// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>
#include <tlhelp32.h>

#include <hadesmem/config.hpp>

namespace hadesmem
{

// TODO: Add tests for ThreadEntry.
class ThreadEntry
{
public:
  explicit ThreadEntry(THREADENTRY32 const& entry) HADESMEM_DETAIL_NOEXCEPT
    : usage_(entry.cntUsage), 
    thread_id_(entry.th32ThreadID), 
    owner_process_id_(entry.th32OwnerProcessID), 
    base_priority_(entry.tpBasePri), 
    delta_priority_(entry.tpDeltaPri), 
    flags_(entry.dwFlags)
  { }

  ThreadEntry(ThreadEntry const& other) HADESMEM_DETAIL_NOEXCEPT
    : usage_(other.usage_), 
    thread_id_(other.thread_id_), 
    owner_process_id_(other.owner_process_id_), 
    base_priority_(other.base_priority_), 
    delta_priority_(other.delta_priority_), 
    flags_(other.flags_)
  { }

  ThreadEntry& operator=(ThreadEntry const& other) HADESMEM_DETAIL_NOEXCEPT
  {
    usage_ = other.usage_;
    thread_id_ = other.thread_id_;
    owner_process_id_ = other.owner_process_id_;
    base_priority_ = other.base_priority_;
    delta_priority_ = other.delta_priority_;
    flags_ = other.flags_;

    return *this;
  }

  ThreadEntry(ThreadEntry&& other) HADESMEM_DETAIL_NOEXCEPT
    : usage_(other.usage_), 
    thread_id_(other.thread_id_), 
    owner_process_id_(other.owner_process_id_), 
    base_priority_(other.base_priority_), 
    delta_priority_(other.delta_priority_), 
    flags_(other.flags_)
  { }

  ThreadEntry& operator=(ThreadEntry&& other) HADESMEM_DETAIL_NOEXCEPT
  {
    usage_ = other.usage_;
    thread_id_ = other.thread_id_;
    owner_process_id_ = other.owner_process_id_;
    base_priority_ = other.base_priority_;
    delta_priority_ = other.delta_priority_;
    flags_ = other.flags_;

    return *this;
  }

  ~ThreadEntry() HADESMEM_DETAIL_NOEXCEPT
  { }

  DWORD GetUsage() const HADESMEM_DETAIL_NOEXCEPT
  {
    return usage_;
  }

  DWORD GetId() const HADESMEM_DETAIL_NOEXCEPT
  {
    return thread_id_;
  }

  DWORD GetOwnerId() const HADESMEM_DETAIL_NOEXCEPT
  {
    return owner_process_id_;
  }

  LONG GetBasePriority() const HADESMEM_DETAIL_NOEXCEPT
  {
    return base_priority_;
  }

  LONG GetDeltaPriority() const HADESMEM_DETAIL_NOEXCEPT
  {
    return delta_priority_;
  }

  DWORD GetFlags() const HADESMEM_DETAIL_NOEXCEPT
  {
    return flags_;
  }
  
private:
  DWORD usage_;
  DWORD thread_id_;
  DWORD owner_process_id_;
  LONG base_priority_;
  LONG delta_priority_;
  DWORD flags_;
};

}
