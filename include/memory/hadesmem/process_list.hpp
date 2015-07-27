// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <iterator>
#include <memory>
#include <utility>

#include <windows.h>
#include <tlhelp32.h>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/optional.hpp>
#include <hadesmem/detail/smart_handle.hpp>
#include <hadesmem/detail/toolhelp.hpp>
#include <hadesmem/process_entry.hpp>

namespace hadesmem
{

// ProcessIterator satisfies the requirements of an input iterator
// (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
template <typename ProcessEntryT>
class ProcessIterator
  : public std::iterator<std::input_iterator_tag, ProcessEntryT>
{
public:
  using BaseIteratorT = std::iterator<std::input_iterator_tag, ProcessEntryT>;
  using value_type = typename BaseIteratorT::value_type;
  using difference_type = typename BaseIteratorT::difference_type;
  using pointer = typename BaseIteratorT::pointer;
  using reference = typename BaseIteratorT::reference;
  using iterator_category = typename BaseIteratorT::iterator_category;

  constexpr ProcessIterator() noexcept
  {
  }

  ProcessIterator(std::int32_t /*dummy*/) : impl_{std::make_shared<Impl>()}
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());

    impl_->snap_ = detail::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    hadesmem::detail::Optional<PROCESSENTRY32W> const entry =
      detail::Process32First(impl_->snap_.GetHandle());
    if (!entry)
    {
      impl_.reset();
      return;
    }

    impl_->process_ = ProcessEntry{*entry};
  }

  reference operator*() const noexcept
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return *impl_->process_;
  }

  pointer operator->() const noexcept
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return &*impl_->process_;
  }

  ProcessIterator& operator++()
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());

    hadesmem::detail::Optional<PROCESSENTRY32> const entry =
      detail::Process32Next(impl_->snap_.GetHandle());
    if (!entry)
    {
      impl_.reset();
      return *this;
    }

    impl_->process_ = ProcessEntry{*entry};

    return *this;
  }

  ProcessIterator operator++(int)
  {
    ProcessIterator const iter{*this};
    ++*this;
    return iter;
  }

  bool operator==(ProcessIterator const& other) const noexcept
  {
    return impl_ == other.impl_;
  }

  bool operator!=(ProcessIterator const& other) const noexcept
  {
    return !(*this == other);
  }

private:
  struct Impl
  {
    detail::SmartSnapHandle snap_{};
    hadesmem::detail::Optional<ProcessEntry> process_{};
  };

  // Shallow copy semantics, as required by InputIterator.
  std::shared_ptr<Impl> impl_;
};

class ProcessList
{
public:
  using value_type = ProcessEntry;
  using iterator = ProcessIterator<ProcessEntry>;
  using const_iterator = ProcessIterator<ProcessEntry const>;

  iterator begin()
  {
    return iterator(0);
  }

  const_iterator begin() const
  {
    return const_iterator(0);
  }

  const_iterator cbegin() const
  {
    return const_iterator(0);
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
};
}
