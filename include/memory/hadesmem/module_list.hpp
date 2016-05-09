// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <iterator>
#include <memory>
#include <utility>

#include <windows.h>
#include <tlhelp32.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/optional.hpp>
#include <hadesmem/detail/smart_handle.hpp>
#include <hadesmem/detail/toolhelp.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/process.hpp>

namespace hadesmem
{
// ModuleIterator satisfies the requirements of an input iterator
// (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
template <typename ModuleT>
class ModuleIterator : public std::iterator<std::input_iterator_tag, ModuleT>
{
public:
  using BaseIteratorT = std::iterator<std::input_iterator_tag, ModuleT>;
  using value_type = typename BaseIteratorT::value_type;
  using difference_type = typename BaseIteratorT::difference_type;
  using pointer = typename BaseIteratorT::pointer;
  using reference = typename BaseIteratorT::reference;
  using iterator_category = typename BaseIteratorT::iterator_category;

  constexpr ModuleIterator() noexcept
  {
  }

  explicit ModuleIterator(Process const& process)
    : impl_{std::make_shared<Impl>()}
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());

    impl_->process_ = &process;

    // CreateToolhelp32Snapshot can fail with ERROR_PARTIAL_COPY for 'zombie'
    // processes.
    try
    {
      impl_->snap_ = detail::CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, impl_->process_->GetId());
    }
    catch (std::exception const&)
    {
      impl_.reset();
      return;
    }

    hadesmem::detail::Optional<MODULEENTRY32> const entry =
      detail::Module32First(impl_->snap_.GetHandle());
    if (!entry)
    {
      impl_.reset();
      return;
    }

    impl_->module_ = Module{*impl_->process_, *entry};
  }

  explicit ModuleIterator(Process&& process) = delete;

  reference operator*() const noexcept
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return *impl_->module_;
  }

  pointer operator->() const noexcept
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return &*impl_->module_;
  }

  ModuleIterator& operator++()
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());

    hadesmem::detail::Optional<MODULEENTRY32> const entry =
      detail::Module32Next(impl_->snap_.GetHandle());
    if (!entry)
    {
      impl_.reset();
      return *this;
    }

    impl_->module_ = Module{*impl_->process_, *entry};

    return *this;
  }

  ModuleIterator operator++(int)
  {
    ModuleIterator const iter{*this};
    ++*this;
    return iter;
  }

  bool operator==(ModuleIterator const& other) const noexcept
  {
    return impl_ == other.impl_;
  }

  bool operator!=(ModuleIterator const& other) const noexcept
  {
    return impl_ != other.impl_;
  }

private:
  struct Impl
  {
    Process const* process_{nullptr};
    detail::SmartSnapHandle snap_{};
    hadesmem::detail::Optional<Module> module_{};
  };

  // Shallow copy semantics, as required by InputIterator.
  std::shared_ptr<Impl> impl_;
};

class ModuleList
{
public:
  using value_type = Module;
  using iterator = ModuleIterator<Module>;
  using const_iterator = ModuleIterator<Module const>;

  constexpr explicit ModuleList(Process const& process) noexcept
    : process_{&process}
  {
  }

  constexpr explicit ModuleList(Process&& process) noexcept = delete;

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

  iterator end() noexcept
  {
    return iterator();
  }

  const_iterator end() const noexcept
  {
    return const_iterator();
  }

  const_iterator cend() const noexcept
  {
    return const_iterator();
  }

private:
  Process const* process_;
};
}
