// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>

#include <windows.h>
#include <winnt.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/optional.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/pelib/nt_headers.hpp>
#include <hadesmem/pelib/section.hpp>
#include <hadesmem/process.hpp>

namespace hadesmem
{

// SectionIterator satisfies the requirements of an input iterator
// (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
template <typename SectionT>
class SectionIterator : public std::iterator<std::input_iterator_tag, SectionT>
{
public:
  using BaseIteratorT = std::iterator<std::input_iterator_tag, SectionT>;
  using value_type = typename BaseIteratorT::value_type;
  using difference_type = typename BaseIteratorT::difference_type;
  using pointer = typename BaseIteratorT::pointer;
  using reference = typename BaseIteratorT::reference;
  using iterator_category = typename BaseIteratorT::iterator_category;

  HADESMEM_DETAIL_CONSTEXPR SectionIterator() HADESMEM_DETAIL_NOEXCEPT : impl_()
  {
  }

  explicit SectionIterator(Process const& process, PeFile const& pe_file)
    : impl_(std::make_shared<Impl>(process, pe_file))
  {
    NtHeaders const nt_headers(process, pe_file);
    impl_->num_sections_ = nt_headers.GetNumberOfSections();
    if (!impl_->num_sections_)
    {
      impl_.reset();
      return;
    }

    impl_->section_ = Section(process, pe_file, nullptr);
    impl_->cur_section_ = 0U;
  }

#if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

  SectionIterator(SectionIterator const&) = default;

  SectionIterator& operator=(SectionIterator const&) = default;

  SectionIterator(SectionIterator&& other) HADESMEM_DETAIL_NOEXCEPT
    : impl_(std::move(other.impl_))
  {
  }

  SectionIterator& operator=(SectionIterator&& other) HADESMEM_DETAIL_NOEXCEPT
  {
    impl_ = std::move(other.impl_);

    return *this;
  }

#endif // #if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

  reference operator*() const HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return *impl_->section_;
  }

  pointer operator->() const HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return &*impl_->section_;
  }

  SectionIterator& operator++()
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());

    // TODO: Handle overflow here for cases where we have the maximum number of
    // sections?
    if (++impl_->cur_section_ >= impl_->num_sections_)
    {
      impl_.reset();
      return *this;
    }

    PIMAGE_SECTION_HEADER new_base =
      reinterpret_cast<PIMAGE_SECTION_HEADER>(impl_->section_->GetBase()) + 1U;
    impl_->section_ = Section(*impl_->process_, *impl_->pe_file_, new_base);

    return *this;
  }

  SectionIterator operator++(int)
  {
    SectionIterator const iter(*this);
    ++*this;
    return iter;
  }

  bool operator==(SectionIterator const& other) const HADESMEM_DETAIL_NOEXCEPT
  {
    return impl_ == other.impl_;
  }

  bool operator!=(SectionIterator const& other) const HADESMEM_DETAIL_NOEXCEPT
  {
    return !(*this == other);
  }

private:
  struct Impl
  {
    explicit Impl(Process const& process,
                  PeFile const& pe_file) HADESMEM_DETAIL_NOEXCEPT
      : process_(&process),
        pe_file_(&pe_file),
        section_()
    {
    }

    Process const* process_;
    PeFile const* pe_file_;
    hadesmem::detail::Optional<Section> section_;
    WORD num_sections_;
    WORD cur_section_;
  };

  // Using a shared_ptr to provide shallow copy semantics, as
  // required by InputIterator.
  std::shared_ptr<Impl> impl_;
};

class SectionList
{
public:
  using value_type = Section;
  using iterator = SectionIterator<Section>;
  using const_iterator = SectionIterator<Section const>;

  explicit SectionList(Process const& process, PeFile const& pe_file)
    : process_(&process), pe_file_(&pe_file)
  {
  }

  iterator begin()
  {
    return SectionList::iterator(*process_, *pe_file_);
  }

  const_iterator begin() const
  {
    return SectionList::const_iterator(*process_, *pe_file_);
  }

  const_iterator cbegin() const
  {
    return SectionList::const_iterator(*process_, *pe_file_);
  }

  iterator end() HADESMEM_DETAIL_NOEXCEPT
  {
    return SectionList::iterator();
  }

  const_iterator end() const HADESMEM_DETAIL_NOEXCEPT
  {
    return SectionList::const_iterator();
  }

  const_iterator cend() const HADESMEM_DETAIL_NOEXCEPT
  {
    return SectionList::const_iterator();
  }

private:
  Process const* process_;
  PeFile const* pe_file_;
};
}
