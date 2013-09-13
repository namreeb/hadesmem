// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <memory>
#include <utility>
#include <iterator>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/optional.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <windows.h>
#include <tlhelp32.h>

#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/toolhelp.hpp>
#include <hadesmem/detail/smart_handle.hpp>

namespace hadesmem
{

// ModuleIterator satisfies the requirements of an input iterator 
// (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
template <typename ModuleT>
class ModuleIterator : public std::iterator<std::input_iterator_tag, ModuleT>
{
public:
  typedef std::iterator<std::input_iterator_tag, ModuleT> BaseIteratorT;
  typedef typename BaseIteratorT::value_type value_type;
  typedef typename BaseIteratorT::difference_type difference_type;
  typedef typename BaseIteratorT::pointer pointer;
  typedef typename BaseIteratorT::reference reference;
  typedef typename BaseIteratorT::iterator_category iterator_category;

  ModuleIterator() HADESMEM_DETAIL_NOEXCEPT
    : impl_()
  { }
  
  explicit ModuleIterator(Process const& process)
    : impl_(std::make_shared<Impl>())
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
  
    impl_->process_ = &process;
  
    impl_->snap_ = detail::CreateToolhelp32Snapshot(
      TH32CS_SNAPMODULE, 
      impl_->process_->GetId());
    
    boost::optional<MODULEENTRY32> entry = 
      detail::Module32First(impl_->snap_.GetHandle());
    if (!entry)
    {
      impl_.reset();
      return;
    }
  
    impl_->module_ = Module(*impl_->process_, *entry);
  }

  ModuleIterator(ModuleIterator const& other) HADESMEM_DETAIL_NOEXCEPT
    : impl_(other.impl_)
  { }

  ModuleIterator& operator=(ModuleIterator const& other) HADESMEM_DETAIL_NOEXCEPT
  {
    impl_ = other.impl_;

    return *this;
  }

  ModuleIterator(ModuleIterator&& other) HADESMEM_DETAIL_NOEXCEPT
    : impl_(std::move(other.impl_))
  { }

  ModuleIterator& operator=(ModuleIterator&& other) HADESMEM_DETAIL_NOEXCEPT
  {
    impl_ = std::move(other.impl_);

    return *this;
  }

  reference operator*() const HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return *impl_->module_;
  }

  pointer operator->() const HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return &*impl_->module_;
  }

  ModuleIterator& operator++()
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    
    boost::optional<MODULEENTRY32> entry = 
      detail::Module32Next(impl_->snap_.GetHandle());
    if (!entry)
    {
      impl_.reset();
      return *this;
    }
  
    impl_->module_ = Module(*impl_->process_, *entry);
  
    return *this;
  }
  
  ModuleIterator operator++(int)
  {
    ModuleIterator iter(*this);
    ++*this;
    return iter;
  }
  
  bool operator==(ModuleIterator const& other) const HADESMEM_DETAIL_NOEXCEPT
  {
    return impl_ == other.impl_;
  }
  
  bool operator!=(ModuleIterator const& other) const HADESMEM_DETAIL_NOEXCEPT
  {
    return impl_ != other.impl_;
  }
  
private:
  struct Impl
  {
    Impl() HADESMEM_DETAIL_NOEXCEPT
      : process_(nullptr), 
      snap_(INVALID_HANDLE_VALUE), 
      module_()
    { }

    Process const* process_;
    detail::SmartSnapHandle snap_;
    boost::optional<Module> module_;
  };

  // Using a shared_ptr to provide shallow copy semantics, as 
  // required by InputIterator.
  std::shared_ptr<Impl> impl_;
};

class ModuleList
{
public:
  typedef Module value_type;
  typedef ModuleIterator<Module> iterator;
  typedef ModuleIterator<Module const> const_iterator;
  
  explicit HADESMEM_DETAIL_CONSTEXPR ModuleList(Process const& process) 
    HADESMEM_DETAIL_NOEXCEPT
    : process_(&process)
  { }

  HADESMEM_DETAIL_CONSTEXPR ModuleList(ModuleList const& other) HADESMEM_DETAIL_NOEXCEPT
    : process_(other.process_)
  { }

  ModuleList& operator=(ModuleList const& other) HADESMEM_DETAIL_NOEXCEPT
  {
    process_ = other.process_;

    return *this;
  }

  HADESMEM_DETAIL_CONSTEXPR ModuleList(ModuleList&& other) HADESMEM_DETAIL_NOEXCEPT
    : process_(other.process_)
  { }

  ModuleList& operator=(ModuleList&& other) HADESMEM_DETAIL_NOEXCEPT
  {
    process_ = other.process_;

    return *this;
  }

  ~ModuleList() HADESMEM_DETAIL_NOEXCEPT
  { }
  
  iterator begin()
  {
    return iterator(*process_);
  }
  
  const_iterator begin() const
  {
    return const_iterator(*process_);
  }
  
  const_iterator cbegin() const
  {
    return const_iterator(*process_);
  }
  
  iterator end() HADESMEM_DETAIL_NOEXCEPT
  {
    return iterator();
  }
  
  const_iterator end() const HADESMEM_DETAIL_NOEXCEPT
  {
    return const_iterator();
  }
  
  const_iterator cend() const HADESMEM_DETAIL_NOEXCEPT
  {
    return const_iterator();
  }
  
private:
  Process const* process_;
};

}
