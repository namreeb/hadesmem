// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/region_iterator.hpp"

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <windows.h>

#include "hadesmem/error.hpp"
#include "hadesmem/region.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/protect.hpp"
#include "hadesmem/detail/query_region.hpp"
#include "hadesmem/detail/region_iterator_impl.hpp"

namespace hadesmem
{

RegionIterator::RegionIterator() BOOST_NOEXCEPT
  : impl_()
{ }

RegionIterator::RegionIterator(Process const* process)
  : impl_(new detail::RegionIteratorImpl)
{
  BOOST_ASSERT(impl_.get());
  BOOST_ASSERT(process != nullptr);
  
  impl_->process_ = process;
  
  MEMORY_BASIC_INFORMATION const mbi = detail::Query(*impl_->process_, 
    nullptr);
  impl_->region_ = Region(impl_->process_, mbi);
}

RegionIterator::RegionIterator(RegionIterator const& other) BOOST_NOEXCEPT
  : impl_(other.impl_)
{ }

RegionIterator& RegionIterator::operator=(RegionIterator const& other) 
  BOOST_NOEXCEPT
{
  impl_ = other.impl_;
  
  return *this;
}

RegionIterator::RegionIterator(RegionIterator&& other) BOOST_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

RegionIterator& RegionIterator::operator=(RegionIterator&& other) 
  BOOST_NOEXCEPT
{
  impl_ = std::move(other.impl_);
  
  return *this;
}

RegionIterator::~RegionIterator()
{ }

RegionIterator::reference RegionIterator::operator*() const BOOST_NOEXCEPT
{
  BOOST_ASSERT(impl_.get());
  return *impl_->region_;
}

RegionIterator::pointer RegionIterator::operator->() const BOOST_NOEXCEPT
{
  BOOST_ASSERT(impl_.get());
  return &*impl_->region_;
}

RegionIterator& RegionIterator::operator++()
{
  try
  {
    BOOST_ASSERT(impl_.get());
    
    void const* const base = impl_->region_->GetBase();
    SIZE_T const size = impl_->region_->GetSize();
    void const* const next = static_cast<char const* const>(base) + size;
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
  BOOST_NOEXCEPT
{
  return impl_ == other.impl_;
}

bool RegionIterator::operator!=(RegionIterator const& other) const 
  BOOST_NOEXCEPT
{
  return !(*this == other);
}

}
