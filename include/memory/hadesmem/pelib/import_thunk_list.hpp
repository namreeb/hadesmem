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
#include <hadesmem/pelib/import_thunk.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>

namespace hadesmem
{
// ImportThunkIterator satisfies the requirements of an input iterator
// (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
template <typename ImportThunkT>
class ImportThunkIterator
  : public std::iterator<std::input_iterator_tag, ImportThunkT>
{
public:
  using BaseIteratorT = std::iterator<std::input_iterator_tag, ImportThunkT>;
  using value_type = typename BaseIteratorT::value_type;
  using difference_type = typename BaseIteratorT::difference_type;
  using pointer = typename BaseIteratorT::pointer;
  using reference = typename BaseIteratorT::reference;
  using iterator_category = typename BaseIteratorT::iterator_category;

  constexpr ImportThunkIterator() noexcept
  {
  }

  explicit ImportThunkIterator(Process const& process,
                               PeFile const& pe_file,
                               DWORD first_thunk)
  {
    try
    {
      auto const thunk_ptr = RvaToVa(process, pe_file, first_thunk);
      if (!thunk_ptr)
      {
        return;
      }

      ImportThunk thunk{process, pe_file, thunk_ptr};
      if (!thunk.GetAddressOfData())
      {
        return;
      }

      impl_ = std::make_shared<Impl>(process, pe_file, thunk);
    }
    catch (std::exception const& /*e*/)
    {
      // Nothing to do here.
    }
  }

  explicit ImportThunkIterator(Process const&& process,
                               PeFile const& pe_file,
                               DWORD first_thunk) = delete;

  explicit ImportThunkIterator(Process const& process,
                               PeFile&& pe_file,
                               DWORD first_thunk) = delete;

  explicit ImportThunkIterator(Process const&& process,
                               PeFile&& pe_file,
                               DWORD first_thunk) = delete;

  reference operator*() const noexcept
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return *impl_->import_thunk_;
  }

  pointer operator->() const noexcept
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return &*impl_->import_thunk_;
  }

  ImportThunkIterator& operator++()
  {
    try
    {
      HADESMEM_DETAIL_ASSERT(impl_.get());

      auto const cur_base =
        static_cast<std::uint8_t*>(impl_->import_thunk_->GetBase());
      void* const next = impl_->pe_file_->Is64()
                           ? cur_base + sizeof(IMAGE_THUNK_DATA64)
                           : cur_base + sizeof(IMAGE_THUNK_DATA32);
      impl_->import_thunk_ =
        ImportThunk{*impl_->process_, *impl_->pe_file_, next};

      if (!impl_->import_thunk_->GetAddressOfData())
      {
        impl_.reset();
        return *this;
      }
    }
    catch (std::exception const& /*e*/)
    {
      impl_.reset();
    }

    return *this;
  }

  ImportThunkIterator operator++(int)
  {
    ImportThunkIterator const iter{*this};
    ++*this;
    return iter;
  }

  bool
    operator==(ImportThunkIterator const& other) const noexcept
  {
    return impl_ == other.impl_;
  }

  bool
    operator!=(ImportThunkIterator const& other) const noexcept
  {
    return !(*this == other);
  }

private:
  struct Impl
  {
    explicit Impl(Process const& process,
                  PeFile const& pe_file,
                  ImportThunk const& thunk) noexcept
      : process_{&process},
        pe_file_{&pe_file},
        import_thunk_{thunk}
    {
    }

    Process const* process_;
    PeFile const* pe_file_;
    hadesmem::detail::Optional<ImportThunk> import_thunk_;
  };

  // Shallow copy semantics, as required by InputIterator.
  std::shared_ptr<Impl> impl_;
};

class ImportThunkList
{
public:
  using value_type = ImportThunk;
  using iterator = ImportThunkIterator<ImportThunk>;
  using const_iterator = ImportThunkIterator<ImportThunk const>;

  explicit ImportThunkList(Process const& process,
                           PeFile const& pe_file,
                           DWORD first_thunk)
    : process_{&process}, pe_file_{&pe_file}, first_thunk_{first_thunk}
  {
  }

  explicit ImportThunkList(Process const&& process,
                           PeFile const& pe_file,
                           DWORD first_thunk) = delete;

  explicit ImportThunkList(Process const& process,
                           PeFile&& pe_file,
                           DWORD first_thunk) = delete;

  explicit ImportThunkList(Process const&& process,
                           PeFile&& pe_file,
                           DWORD first_thunk) = delete;

  iterator begin()
  {
    return iterator{*process_, *pe_file_, first_thunk_};
  }

  const_iterator begin() const
  {
    return const_iterator{*process_, *pe_file_, first_thunk_};
  }

  const_iterator cbegin() const
  {
    return const_iterator{*process_, *pe_file_, first_thunk_};
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
  DWORD first_thunk_;
};
}
