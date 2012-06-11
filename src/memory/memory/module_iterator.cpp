// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/module_iterator.hpp"

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/none.hpp>
#include <boost/assert.hpp>
#include <boost/optional.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/module.hpp"
#include "hadesmem/process.hpp"

namespace hadesmem
{

namespace detail
{
  struct ModuleIteratorImpl
  {
    ~ModuleIteratorImpl() BOOST_NOEXCEPT
    {
      BOOST_VERIFY(CloseHandle(snap_));
    }
    
    Process const* process_;
    HANDLE snap_;
    boost::optional<Module> module_;
  };
}

ModuleIterator::ModuleIterator() BOOST_NOEXCEPT
  : impl_()
{ }

ModuleIterator::ModuleIterator(Process const& process)
  : impl_(new detail::ModuleIteratorImpl)
{
  impl_->process_ = &process;
  
  // TODO: Attempt to call this function at least twice on ERROR_BAD_LENGTH.
  // Potentially call until success if it can be determined whether or not 
  // the process started suspended (as per MSDN).
  impl_->snap_ = CreateToolhelp32Snapshot(
    TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, 
    impl_->process_->GetId());
  if (impl_->snap_ == INVALID_HANDLE_VALUE)
  {
    DWORD const last_error = GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("CreateToolhelp32Snapshot failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  MODULEENTRY32 entry;
  ZeroMemory(&entry, sizeof(entry));
  entry.dwSize = sizeof(entry);
  if (!Module32First(impl_->snap_, &entry))
  {
    // TODO: More gracefully handle failure when GetLastError returns 
    // ERROR_NO_MORE_FILES. It seems that we can just treat that as an EOL, 
    // however I first want to understand the circumstances under which that 
    // error can occur for the first module in the list (other than an invalid 
    // snapshot type).
    DWORD const last_error = GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Module32First failed.") << 
      ErrorCodeWinLast(last_error));
  }
  
  impl_->module_ = Module(*impl_->process_, entry);
}

ModuleIterator::ModuleIteratorFacade::reference ModuleIterator::dereference() const
{
  BOOST_ASSERT(impl_.get());
  return *impl_->module_;
}

void ModuleIterator::increment()
{
  MODULEENTRY32 entry;
  ZeroMemory(&entry, sizeof(entry));
  entry.dwSize = sizeof(entry);
  if (!Module32Next(impl_->snap_, &entry))
  {
    if (GetLastError() == ERROR_NO_MORE_FILES)
    {
      impl_.reset();
      return;
    }
    else
    {
      DWORD const last_error = GetLastError();
      BOOST_THROW_EXCEPTION(HadesMemError() << 
        ErrorString("Module32Next failed.") << 
        ErrorCodeWinLast(last_error));
    }
  }
  
  impl_->module_ = Module(*impl_->process_, entry);
}

bool ModuleIterator::equal(ModuleIterator const& other) const BOOST_NOEXCEPT
{
  return impl_ == other.impl_;
}

}
