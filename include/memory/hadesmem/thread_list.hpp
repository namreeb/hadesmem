// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <memory>
#include <utility>
#include <iterator>

#include <windows.h>
#include <tlhelp32.h>

#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/thread_entry.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/optional.hpp>
#include <hadesmem/detail/toolhelp.hpp>
#include <hadesmem/detail/smart_handle.hpp>

namespace hadesmem
{
// ThreadIterator satisfies the requirements of an input iterator
// (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
template <typename ThreadEntryT>
class ThreadIterator
  : public std::iterator<std::input_iterator_tag, ThreadEntryT>
{
public:
  using BaseIteratorT = std::iterator<std::input_iterator_tag, ThreadEntryT>;
  using value_type = typename BaseIteratorT::value_type;
  using difference_type = typename BaseIteratorT::difference_type;
  using pointer = typename BaseIteratorT::pointer;
  using reference = typename BaseIteratorT::reference;
  using iterator_category = typename BaseIteratorT::iterator_category;

  constexpr ThreadIterator() noexcept
  {
  }

  ThreadIterator(DWORD pid) noexcept : impl_(std::make_shared<Impl>()),
                                       pid_(pid)
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());

    impl_->snap_ = detail::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

    hadesmem::detail::Optional<THREADENTRY32> const entry =
      detail::Thread32First(impl_->snap_.GetHandle());
    if (!entry)
    {
      impl_.reset();
      return;
    }

    if (IsTargetThread(entry->th32OwnerProcessID))
    {
      impl_->thread_ = ThreadEntry{*entry};
    }
    else
    {
      Advance();
    }
  }

  reference operator*() const noexcept
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return *impl_->thread_;
  }

  pointer operator->() const noexcept
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return &*impl_->thread_;
  }

  ThreadIterator& operator++()
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());

    Advance();

    return *this;
  }

  ThreadIterator operator++(int)
  {
    ThreadIterator const iter{*this};
    ++*this;
    return iter;
  }

  bool operator==(ThreadIterator const& other) const noexcept
  {
    return impl_ == other.impl_;
  }

  bool operator!=(ThreadIterator const& other) const noexcept
  {
    return !(*this == other);
  }

private:
  void Advance()
  {
    for (;;)
    {
      hadesmem::detail::Optional<THREADENTRY32> const entry =
        detail::Thread32Next(impl_->snap_.GetHandle());
      if (!entry)
      {
        impl_.reset();
        break;
      }

      if (IsTargetThread(entry->th32OwnerProcessID))
      {
        impl_->thread_ = ThreadEntry{*entry};
        break;
      }
    }
  }

  bool IsTargetThread(DWORD owner_id) noexcept
  {
    return pid_ == static_cast<DWORD>(-1) || pid_ == owner_id;
  }

  struct Impl
  {
    detail::SmartSnapHandle snap_;
    hadesmem::detail::Optional<ThreadEntry> thread_;
  };

  // Shallow copy semantics, as required by InputIterator.
  std::shared_ptr<Impl> impl_;
  DWORD pid_{0};
};

class ThreadList
{
public:
  using value_type = ThreadEntry;
  using iterator = ThreadIterator<ThreadEntry>;
  using const_iterator = ThreadIterator<ThreadEntry const>;

  constexpr ThreadList() noexcept
  {
  }

  // TODO: Change this to take a Process object once we use dynamic access
  // rights. Ensure we won't lose any functionality though in the case of
  // special system processes etc. which may be difficult to get a handle to.
  constexpr ThreadList(DWORD pid) noexcept : pid_(pid)
  {
  }

  iterator begin()
  {
    return iterator(pid_);
  }

  const_iterator begin() const
  {
    return const_iterator(pid_);
  }

  const_iterator cbegin() const
  {
    return const_iterator(pid_);
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
  DWORD pid_{static_cast<DWORD>(-1)};
};
}
