// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <iterator>
#include <memory>
#include <utility>

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/optional.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/pelib/relocation.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>

// TODO: Add tests.

namespace hadesmem
{
// RelocationIterator satisfies the requirements of an input iterator
// (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
template <typename RelocationT>
class RelocationIterator
  : public std::iterator<std::input_iterator_tag, RelocationT>
{
public:
  using BaseIteratorT = std::iterator<std::input_iterator_tag, RelocationT>;
  using value_type = typename BaseIteratorT::value_type;
  using difference_type = typename BaseIteratorT::difference_type;
  using pointer = typename BaseIteratorT::pointer;
  using reference = typename BaseIteratorT::reference;
  using iterator_category = typename BaseIteratorT::iterator_category;

  constexpr RelocationIterator() noexcept
  {
  }

  explicit RelocationIterator(Process const& process,
                              PeFile const& pe_file,
                              PWORD start,
                              DWORD count)
  {
    try
    {
      if (!count)
      {
        return;
      }

      Relocation const relocation(process, pe_file, start);
      impl_ = std::make_shared<Impl>(process, pe_file, relocation, count);
    }
    catch (std::exception const& /*e*/)
    {
      // Nothing to do here.
    }
  }

  explicit RelocationIterator(Process const&& process,
                              PeFile const& pe_file,
                              PWORD start,
                              DWORD count) = delete;

  explicit RelocationIterator(Process const& process,
                              PeFile&& pe_file,
                              PWORD start,
                              DWORD count) = delete;

  explicit RelocationIterator(Process const&& process,
                              PeFile&& pe_file,
                              PWORD start,
                              DWORD count) = delete;

  reference operator*() const noexcept
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return *impl_->relocation_;
  }

  pointer operator->() const noexcept
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return &*impl_->relocation_;
  }

  RelocationIterator& operator++()
  {
    try
    {
      HADESMEM_DETAIL_ASSERT(impl_.get());

      if (++impl_->cur_ >= impl_->count_)
      {
        impl_.reset();
        return *this;
      }

      auto const next_base =
        static_cast<PWORD>(impl_->relocation_->GetBase()) + 1U;
      impl_->relocation_ =
        Relocation{*impl_->process_, *impl_->pe_file_, next_base};
    }
    catch (std::exception const& /*e*/)
    {
      impl_.reset();
    }

    return *this;
  }

  RelocationIterator operator++(int)
  {
    RelocationIterator const iter{*this};
    ++*this;
    return iter;
  }

  bool operator==(RelocationIterator const& other) const noexcept
  {
    return impl_ == other.impl_;
  }

  bool operator!=(RelocationIterator const& other) const noexcept
  {
    return !(*this == other);
  }

private:
  struct Impl
  {
    explicit Impl(Process const& process,
                  PeFile const& pe_file,
                  Relocation const& relocation,
                  DWORD count) noexcept : process_{&process},
                                          pe_file_{&pe_file},
                                          relocation_{relocation},
                                          count_{count}
    {
    }

    Process const* process_;
    PeFile const* pe_file_;
    hadesmem::detail::Optional<Relocation> relocation_;
    DWORD count_;
    DWORD cur_{};
  };

  // Shallow copy semantics, as required by InputIterator.
  std::shared_ptr<Impl> impl_;
};

class RelocationList
{
public:
  using value_type = Relocation;
  using iterator = RelocationIterator<Relocation>;
  using const_iterator = RelocationIterator<Relocation const>;

  explicit RelocationList(Process const& process,
                          PeFile const& pe_file,
                          PWORD start,
                          DWORD count)
    : process_{&process}, pe_file_{&pe_file}, start_{start}, count_{count}
  {
  }

  explicit RelocationList(Process const&& process,
                          PeFile const& pe_file,
                          PWORD start,
                          DWORD count) = delete;

  explicit RelocationList(Process const& process,
                          PeFile&& pe_file,
                          PWORD start,
                          DWORD count) = delete;

  explicit RelocationList(Process const&& process,
                          PeFile&& pe_file,
                          PWORD start,
                          DWORD count) = delete;

  iterator begin()
  {
    return iterator{*process_, *pe_file_, start_, count_};
  }

  const_iterator begin() const
  {
    return const_iterator{*process_, *pe_file_, start_, count_};
  }

  const_iterator cbegin() const
  {
    return const_iterator{*process_, *pe_file_, start_, count_};
  }

  iterator end() noexcept
  {
    return iterator{};
  }

  const_iterator end() const noexcept
  {
    return const_iterator{};
  }

  const_iterator cend() const noexcept
  {
    return const_iterator{};
  }

private:
  Process const* process_;
  PeFile const* pe_file_;
  PWORD start_;
  DWORD count_;
};
}
