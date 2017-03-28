// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <iterator>
#include <memory>
#include <utility>

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/detail/vectored_handler.hpp>

namespace hadesmem
{
namespace detail
{
// VectoredHandlerIterator satisfies the requirements of a mutable input
// iterator (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
template <typename VectoredHandlerEntryT>
class VectoredHandlerIterator
  : public std::iterator<std::input_iterator_tag, VectoredHandlerEntryT>
{
public:
  using BaseIteratorT =
    std::iterator<std::input_iterator_tag, VectoredHandlerEntryT>;
  using value_type = typename BaseIteratorT::value_type;
  using difference_type = typename BaseIteratorT::difference_type;
  using pointer = typename BaseIteratorT::pointer;
  using reference = typename BaseIteratorT::reference;
  using iterator_category = typename BaseIteratorT::iterator_category;

  constexpr VectoredHandlerIterator() noexcept
  {
  }

  VectoredHandlerIterator(Process const& process)
    : impl_{std::make_shared<Impl>()}
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());

    if (process.GetId() != ::GetCurrentProcessId())
    {
      // Don't know whether to throw or not so I
      // just decided to invalidate the pointer.
      impl_.reset();
    }
    else
    {
      impl_->base_ = GetVectoredEhPointer(process);
      impl_->iter_ = impl_->base_;
    }
  }

  reference operator*() const noexcept
  {
    return *impl_->iter_;
  }

  pointer operator->() const noexcept
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return &*impl_->iter_;
  }

  VectoredHandlerIterator& operator++()
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());

    if (impl_->iter_->Next == impl_->base_)
    {
      // Invalidate the pointer since we have
      // already reached the end of the link.
      impl_.reset();
    }
    else
    {
      impl_->iter_ = impl_->iter_->Next;
    }

    return *this;
  }

  VectoredHandlerIterator operator++(int)
  {
    VectoredHandlerIterator old{*this};
    ++*this;
    return old;
  }

  bool operator==(VectoredHandlerIterator const& other) const noexcept
  {
    return impl_ == other.impl_;
  }

  bool operator!=(VectoredHandlerIterator const& other) const noexcept
  {
    return !(*this == other);
  }

private:
  struct Impl
  {
    pointer base_{};
    pointer iter_{};
  };

  // Shallow copy semantics, as required by InputIterator.
  std::shared_ptr<Impl> impl_;
};

class VectoredHandlerList
{
public:
  using value_type = winternl::VECTORED_HANDLER_ENTRY;
  using iterator = VectoredHandlerIterator<winternl::VECTORED_HANDLER_ENTRY>;
  using const_iterator =
    VectoredHandlerIterator<winternl::VECTORED_HANDLER_ENTRY const>;

  VectoredHandlerList(Process const& process) : process_{process}
  {
  }

  iterator begin()
  {
    return iterator(process_);
  }

  const_iterator begin() const
  {
    return const_iterator(process_);
  }

  const_iterator cbegin() const
  {
    return const_iterator(process_);
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
  Process process_;
};
}
}