// Copyright (C) 2010-2015 Joshua Boyce
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

  constexpr SectionIterator() noexcept
  {
  }

  explicit SectionIterator(Process const& process, PeFile const& pe_file)
  {
    NtHeaders const nt_headers(process, pe_file);
    if (auto const num_sections = nt_headers.GetNumberOfSections())
    {
      Section const section(process, pe_file, nullptr);
      impl_ = std::make_shared<Impl>(process, pe_file, section, num_sections);
    }
  }

  explicit SectionIterator(Process const&& process, PeFile const& pe_file) = delete;

  explicit SectionIterator(Process const& process, PeFile&& pe_file) = delete;

  explicit SectionIterator(Process const&& process, PeFile&& pe_file) = delete;

  reference operator*() const noexcept
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return *impl_->section_;
  }

  pointer operator->() const noexcept
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return &*impl_->section_;
  }

  SectionIterator& operator++()
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());

    if (++impl_->cur_section_ >= impl_->num_sections_)
    {
      impl_.reset();
      return *this;
    }

    auto const new_base =
      static_cast<PIMAGE_SECTION_HEADER>(impl_->section_->GetBase()) + 1U;
    impl_->section_ = Section{*impl_->process_, *impl_->pe_file_, new_base};

    return *this;
  }

  SectionIterator operator++(int)
  {
    SectionIterator const iter{*this};
    ++*this;
    return iter;
  }

  bool operator==(SectionIterator const& other) const noexcept
  {
    return impl_ == other.impl_;
  }

  bool operator!=(SectionIterator const& other) const noexcept
  {
    return !(*this == other);
  }

private:
  struct Impl
  {
    explicit Impl(Process const& process,
                  PeFile const& pe_file,
                  Section const& section,
                  WORD num_sections) noexcept
      : process_{&process},
        pe_file_{&pe_file},
        section_{section},
        num_sections_{num_sections}
    {
    }

    Process const* process_;
    PeFile const* pe_file_;
    hadesmem::detail::Optional<Section> section_;
    WORD num_sections_;
    WORD cur_section_{};
  };

  // Shallow copy semantics, as required by InputIterator.
  std::shared_ptr<Impl> impl_;
};

class SectionList
{
public:
  using value_type = Section;
  using iterator = SectionIterator<Section>;
  using const_iterator = SectionIterator<Section const>;

  explicit SectionList(Process const& process, PeFile const& pe_file)
    : process_{&process}, pe_file_{&pe_file}
  {
  }

  explicit SectionList(Process const&& process, PeFile const& pe_file) = delete;

  explicit SectionList(Process const& process, PeFile&& pe_file) = delete;

  explicit SectionList(Process const&& process, PeFile&& pe_file) = delete;

  iterator begin()
  {
    return iterator{*process_, *pe_file_};
  }

  const_iterator begin() const
  {
    return const_iterator{*process_, *pe_file_};
  }

  const_iterator cbegin() const
  {
    return const_iterator{*process_, *pe_file_};
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
};
}
