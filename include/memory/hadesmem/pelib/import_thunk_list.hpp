// Copyright (C) 2010-2014 Joshua Boyce.
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

  HADESMEM_DETAIL_CONSTEXPR ImportThunkIterator() HADESMEM_DETAIL_NOEXCEPT
  {
  }

  explicit ImportThunkIterator(Process const& process,
                               PeFile const& pe_file,
                               DWORD first_thunk)
  {
    try
    {
      auto const thunk_ptr = reinterpret_cast<PIMAGE_THUNK_DATA>(
        RvaToVa(process, pe_file, first_thunk));
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

  explicit ImportThunkIterator(Process&& process,
                               PeFile const& pe_file,
                               DWORD first_thunk) = delete;

  explicit ImportThunkIterator(Process const& process,
                               PeFile&& pe_file,
                               DWORD first_thunk) = delete;

  explicit ImportThunkIterator(Process&& process,
                               PeFile&& pe_file,
                               DWORD first_thunk) = delete;

#if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

  ImportThunkIterator(ImportThunkIterator const&) = default;

  ImportThunkIterator& operator=(ImportThunkIterator const&) = default;

  ImportThunkIterator(ImportThunkIterator&& other) HADESMEM_DETAIL_NOEXCEPT
    : impl_{std::move(other.impl_)}
  {
  }

  ImportThunkIterator& operator=(ImportThunkIterator&& other)
    HADESMEM_DETAIL_NOEXCEPT
  {
    impl_ = std::move(other.impl_);

    return *this;
  }

#endif // #if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

  reference operator*() const HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return *impl_->import_thunk_;
  }

  pointer operator->() const HADESMEM_DETAIL_NOEXCEPT
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
        reinterpret_cast<PIMAGE_THUNK_DATA>(impl_->import_thunk_->GetBase());
      impl_->import_thunk_ =
        ImportThunk{*impl_->process_, *impl_->pe_file_, cur_base + 1};

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

  bool operator==(ImportThunkIterator const& other) const
    HADESMEM_DETAIL_NOEXCEPT
  {
    return impl_ == other.impl_;
  }

  bool operator!=(ImportThunkIterator const& other) const
    HADESMEM_DETAIL_NOEXCEPT
  {
    return !(*this == other);
  }

private:
  struct Impl
  {
    explicit Impl(Process const& process,
                  PeFile const& pe_file,
                  ImportThunk const& thunk) HADESMEM_DETAIL_NOEXCEPT
      : process_{&process},
        pe_file_{&pe_file},
        import_thunk_{thunk}
    {
    }

    Process const* process_;
    PeFile const* pe_file_;
    hadesmem::detail::Optional<ImportThunk> import_thunk_;
  };

  // Using a shared_ptr to provide shallow copy semantics, as
  // required by InputIterator.
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

  explicit ImportThunkList(Process&& process,
                           PeFile const& pe_file,
                           DWORD first_thunk) = delete;

  explicit ImportThunkList(Process const& process,
                           PeFile&& pe_file,
                           DWORD first_thunk) = delete;

  explicit ImportThunkList(Process&& process,
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

  iterator end() HADESMEM_DETAIL_NOEXCEPT
  {
    return iterator{};
  }

  const_iterator end() const HADESMEM_DETAIL_NOEXCEPT
  {
    return const_iterator{};
  }

  const_iterator cend() const HADESMEM_DETAIL_NOEXCEPT
  {
    return const_iterator{};
  }

private:
  Process const* process_;
  PeFile const* pe_file_;
  DWORD first_thunk_;
};
}
