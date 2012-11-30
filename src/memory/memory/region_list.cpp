// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/region_list.hpp"

#include <windows.h>

#include "hadesmem/error.hpp"
#include "hadesmem/region.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/protect.hpp"
#include "hadesmem/detail/query_region.hpp"
#include "hadesmem/detail/region_iterator_impl.hpp"

namespace hadesmem
{

RegionIterator::RegionIterator() HADESMEM_NOEXCEPT
  : impl_()
{ }

RegionIterator::RegionIterator(Process const* process)
  : impl_(new detail::RegionIteratorImpl)
{
  assert(impl_.get());
  assert(process != nullptr);
  
  impl_->process_ = process;
  
  MEMORY_BASIC_INFORMATION const mbi = detail::Query(*impl_->process_, 
    nullptr);
  impl_->region_ = Region(impl_->process_, mbi);
}

RegionIterator::reference RegionIterator::operator*() const HADESMEM_NOEXCEPT
{
  assert(impl_.get());
  return *impl_->region_;
}

RegionIterator::pointer RegionIterator::operator->() const HADESMEM_NOEXCEPT
{
  assert(impl_.get());
  return &*impl_->region_;
}

RegionIterator& RegionIterator::operator++()
{
  try
  {
    assert(impl_.get());
    
    void const* const base = impl_->region_->GetBase();
    SIZE_T const size = impl_->region_->GetSize();
    auto const next = static_cast<char const* const>(base) + size;
    MEMORY_BASIC_INFORMATION const mbi = detail::Query(*impl_->process_, next);
    impl_->region_ = Region(impl_->process_, mbi);
  }
  catch (std::exception const& /*e*/)
  {
    impl_.reset();
  }
  
  return *this;
}

RegionIterator RegionIterator::operator++(int)
{
  RegionIterator iter(*this);
  ++*this;
  return iter;
}

bool RegionIterator::operator==(RegionIterator const& other) const 
  HADESMEM_NOEXCEPT
{
  return impl_ == other.impl_;
}

bool RegionIterator::operator!=(RegionIterator const& other) const 
  HADESMEM_NOEXCEPT
{
  return !(*this == other);
}

RegionList::RegionList(Process const* process) HADESMEM_NOEXCEPT
  : process_(process)
{ }

RegionList::iterator RegionList::begin()
{
  return RegionList::iterator(process_);
}

RegionList::const_iterator RegionList::begin() const
{
  return RegionList::iterator(process_);
}

RegionList::iterator RegionList::end() HADESMEM_NOEXCEPT
{
  return RegionList::iterator();
}

RegionList::const_iterator RegionList::end() const HADESMEM_NOEXCEPT
{
  return RegionList::iterator();
}

}
