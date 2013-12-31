// Copyright (C) 2010-2013 Joshua Boyce.
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
#include <hadesmem/pelib/bound_import_dir.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>

// TODO: Add tests.

namespace hadesmem
{

// ImportDirIterator satisfies the requirements of an input iterator
// (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
template <typename BoundImportDirT>
class BoundImportDirIterator
  : public std::iterator<std::input_iterator_tag, BoundImportDirT>
{
public:
  using BaseIteratorT = std::iterator<std::input_iterator_tag, BoundImportDirT>;
  using value_type = typename BaseIteratorT::value_type;
  using difference_type = typename BaseIteratorT::difference_type;
  using pointer = typename BaseIteratorT::pointer;
  using reference = typename BaseIteratorT::reference;
  using iterator_category = typename BaseIteratorT::iterator_category;

  HADESMEM_DETAIL_CONSTEXPR BoundImportDirIterator() HADESMEM_DETAIL_NOEXCEPT
    : impl_()
  {
  }

  explicit BoundImportDirIterator(Process const& process, PeFile const& pe_file)
    : impl_(std::make_shared<Impl>(process, pe_file))
  {
    try
    {
      impl_->bound_import_dir_ =
        BoundImportDir(process, pe_file, nullptr, nullptr);
      if (IsTerminator())
      {
        impl_.reset();
      }
      else
      {
        impl_->start_bound_import_dir_ = impl_->bound_import_dir_;
      }
    }
    catch (std::exception const& /*e*/)
    {
      impl_.reset();
    }
  }

#if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

  BoundImportDirIterator(BoundImportDirIterator const&) = default;

  BoundImportDirIterator& operator=(BoundImportDirIterator const&) = default;

  BoundImportDirIterator(BoundImportDirIterator&& other)
HADESMEM_DETAIL_NOEXCEPT:
  impl_(std::move(other.impl_))
  {
  }

  BoundImportDirIterator& operator=(BoundImportDirIterator&& other)
    HADESMEM_DETAIL_NOEXCEPT
  {
    impl_ = std::move(other.impl_);

    return *this;
  }

#endif // #if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

  reference operator*() const HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return *impl_->bound_import_dir_;
  }

  pointer operator->() const HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return &*impl_->bound_import_dir_;
  }

  BoundImportDirIterator& operator++()
  {
    try
    {
      HADESMEM_DETAIL_ASSERT(impl_.get());

      auto const cur_base = reinterpret_cast<PIMAGE_BOUND_IMPORT_DESCRIPTOR>(
        impl_->bound_import_dir_->GetBase());
      auto const new_base = reinterpret_cast<PIMAGE_BOUND_IMPORT_DESCRIPTOR>(
        reinterpret_cast<PIMAGE_BOUND_FORWARDER_REF>(cur_base + 1) +
        impl_->bound_import_dir_->GetNumberOfModuleForwarderRefs());
      auto const start_base = reinterpret_cast<PIMAGE_BOUND_IMPORT_DESCRIPTOR>(
        impl_->start_bound_import_dir_->GetBase());
      impl_->bound_import_dir_ = BoundImportDir(
        *impl_->process_, *impl_->pe_file_, start_base, new_base);

      if (IsTerminator())
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

  BoundImportDirIterator operator++(int)
  {
    BoundImportDirIterator const iter(*this);
    ++*this;
    return iter;
  }

  bool operator==(BoundImportDirIterator const& other) const
    HADESMEM_DETAIL_NOEXCEPT
  {
    return impl_ == other.impl_;
  }

  bool operator!=(BoundImportDirIterator const& other) const
    HADESMEM_DETAIL_NOEXCEPT
  {
    return !(*this == other);
  }

private:
  bool IsTerminator() const
  {
    // Apparently all three fields are supposed to be zero, but it seems that
    // may not be the case when it comes to the actual loader implementation.
    // TODO: Use 00030855940c3b6c50789d203a9ba01d8d4cc0dc to verify this (will
    // require patching a module that's loaded so that the corrupted timestamp
    // matches).
    // TODO: Verify this is correct.
    return !impl_->bound_import_dir_->GetTimeDateStamp() ||
           !impl_->bound_import_dir_->GetOffsetModuleName();
  }

  struct Impl
  {
    explicit Impl(Process const& process,
                  PeFile const& pe_file) HADESMEM_DETAIL_NOEXCEPT
      : process_(&process),
        pe_file_(&pe_file),
        bound_import_dir_(),
        start_bound_import_dir_()
    {
    }

    Process const* process_;
    PeFile const* pe_file_;
    hadesmem::detail::Optional<BoundImportDir> bound_import_dir_;
    hadesmem::detail::Optional<BoundImportDir> start_bound_import_dir_;
  };

  // Using a shared_ptr to provide shallow copy semantics, as
  // required by InputIterator.
  std::shared_ptr<Impl> impl_;
};

class BoundImportDirList
{
public:
  using value_type = BoundImportDir;
  using iterator = BoundImportDirIterator<BoundImportDir>;
  using const_iterator = BoundImportDirIterator<BoundImportDir const>;

  explicit BoundImportDirList(Process const& process, PeFile const& pe_file)
    : process_(&process), pe_file_(&pe_file)
  {
  }

  iterator begin()
  {
    return BoundImportDirList::iterator(*process_, *pe_file_);
  }

  const_iterator begin() const
  {
    return BoundImportDirList::const_iterator(*process_, *pe_file_);
  }

  const_iterator cbegin() const
  {
    return BoundImportDirList::const_iterator(*process_, *pe_file_);
  }

  iterator end() HADESMEM_DETAIL_NOEXCEPT
  {
    return BoundImportDirList::iterator();
  }

  const_iterator end() const HADESMEM_DETAIL_NOEXCEPT
  {
    return BoundImportDirList::const_iterator();
  }

  const_iterator cend() const HADESMEM_DETAIL_NOEXCEPT
  {
    return BoundImportDirList::const_iterator();
  }

private:
  Process const* process_;
  PeFile const* pe_file_;
};
}
