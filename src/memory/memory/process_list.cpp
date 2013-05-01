// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/process_list.hpp>

#include <windows.h>
#include <tlhelp32.h>

#include <hadesmem/error.hpp>

namespace hadesmem
{

// TOOD: Clean this up.
ProcessIterator::ProcessIterator(int /*dummy*/)
  : impl_(new Impl())
{
  HADESMEM_ASSERT(impl_.get());
  
  impl_->snap_ = detail::SmartHandle(::CreateToolhelp32Snapshot(
    TH32CS_SNAPPROCESS, 0), INVALID_HANDLE_VALUE);
  if (!impl_->snap_.IsValid())
  {
    if (::GetLastError() == ERROR_BAD_LENGTH)
    {
      impl_->snap_ = detail::SmartHandle(::CreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS, 0), INVALID_HANDLE_VALUE);
      if (!impl_->snap_.IsValid())
      {
        DWORD const last_error = ::GetLastError();
        HADESMEM_THROW_EXCEPTION(Error() << 
          ErrorString("CreateToolhelp32Snapshot failed.") << 
          ErrorCodeWinLast(last_error));
      }
    }
    else
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_THROW_EXCEPTION(Error() << 
        ErrorString("CreateToolhelp32Snapshot failed.") << 
        ErrorCodeWinLast(last_error));
    }
  }
  
  PROCESSENTRY32 entry;
  ::ZeroMemory(&entry, sizeof(entry));
  entry.dwSize = sizeof(entry);
  if (!::Process32First(impl_->snap_.GetHandle(), &entry))
  {
    DWORD const last_error = ::GetLastError();
    
    if (last_error == ERROR_NO_MORE_FILES)
    {
      impl_.reset();
      return;
    }
    
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Process32First failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  impl_->process_ = ProcessEntry(entry);
}

ProcessIterator& ProcessIterator::operator++()
{
  HADESMEM_ASSERT(impl_.get());
  
  PROCESSENTRY32 entry;
  ::ZeroMemory(&entry, sizeof(entry));
  entry.dwSize = sizeof(entry);
  if (!::Process32Next(impl_->snap_.GetHandle(), &entry))
  {
    if (::GetLastError() == ERROR_NO_MORE_FILES)
    {
      impl_.reset();
      return *this;
    }
    else
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_THROW_EXCEPTION(Error() << 
        ErrorString("Module32Next failed.") << 
        ErrorCodeWinLast(last_error));
    }
  }
  
  impl_->process_ = ProcessEntry(entry);
  
  return *this;
}

}
