// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/pelib/section_list.hpp"

#include <cstddef>
#include <utility>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include <boost/optional.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <windows.h>
#include <winnt.h>

#include "hadesmem/error.hpp"
#include "hadesmem/config.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/pelib/section.hpp"
#include "hadesmem/pelib/nt_headers.hpp"

namespace hadesmem
{

struct SectionIterator::Impl
{
  explicit Impl(Process const& process, PeFile const& pe_file) 
    HADESMEM_NOEXCEPT
    : process_(&process), 
    pe_file_(&pe_file), 
    section_()
  { }

  Process const* process_;
  PeFile const* pe_file_;
  boost::optional<Section> section_;
};

SectionIterator::SectionIterator() HADESMEM_NOEXCEPT
  : impl_()
{ }

SectionIterator::SectionIterator(Process const& process, PeFile const& pe_file)
  : impl_(new Impl(process, pe_file))
{
  NtHeaders const nt_headers(process, pe_file);
  if (!nt_headers.GetNumberOfSections())
  {
    impl_.reset();
    return;
  }

  PBYTE base = static_cast<PBYTE>(nt_headers.GetBase()) + 
    offsetof(IMAGE_NT_HEADERS, OptionalHeader) + 
    nt_headers.GetSizeOfOptionalHeader();

  impl_->section_ = Section(process, pe_file, 0, base);
}

SectionIterator::SectionIterator(SectionIterator const& other) 
  HADESMEM_NOEXCEPT
  : impl_(other.impl_)
{ }

SectionIterator& SectionIterator::operator=(SectionIterator const& other) 
  HADESMEM_NOEXCEPT
{
  impl_ = other.impl_;

  return *this;
}

SectionIterator::SectionIterator(SectionIterator&& other) HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

SectionIterator& SectionIterator::operator=(SectionIterator&& other) 
  HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);

  return *this;
}

SectionIterator::~SectionIterator()
{ }

SectionIterator::reference SectionIterator::operator*() const HADESMEM_NOEXCEPT
{
  BOOST_ASSERT(impl_.get());
  return *impl_->section_;
}

SectionIterator::pointer SectionIterator::operator->() const HADESMEM_NOEXCEPT
{
  BOOST_ASSERT(impl_.get());
  return &*impl_->section_;
}

SectionIterator& SectionIterator::operator++()
{
  BOOST_ASSERT(impl_.get());

  WORD const number = impl_->section_->GetNumber();

  NtHeaders const nt_headers(*impl_->process_, *impl_->pe_file_);
  if (number + 1 >= nt_headers.GetNumberOfSections())
  {
    impl_.reset();
    return *this;
  }

  PIMAGE_SECTION_HEADER new_base = reinterpret_cast<PIMAGE_SECTION_HEADER>(
    impl_->section_->GetBase()) + 1;
  impl_->section_ = Section(*impl_->process_, *impl_->pe_file_, number + 1, 
    new_base);
  
  return *this;
}

SectionIterator SectionIterator::operator++(int)
{
  SectionIterator iter(*this);
  ++*this;
  return iter;
}

bool SectionIterator::operator==(SectionIterator const& other) const 
  HADESMEM_NOEXCEPT
{
  return impl_ == other.impl_;
}

bool SectionIterator::operator!=(SectionIterator const& other) const 
  HADESMEM_NOEXCEPT
{
  return !(*this == other);
}

struct SectionList::Impl
{
  Impl(Process const& process, PeFile const& pe_file)
    : process_(&process), 
    pe_file_(&pe_file)
  { }

  Process const* process_;
  PeFile const* pe_file_;
};

SectionList::SectionList(Process const& process, PeFile const& pe_file)
  : impl_(new Impl(process, pe_file))
{ }

SectionList::SectionList(SectionList const& other)
  : impl_(new Impl(*other.impl_))
{ }

SectionList& SectionList::operator=(SectionList const& other)
{
  impl_ = std::unique_ptr<Impl>(new Impl(*other.impl_));

  return *this;
}

SectionList::SectionList(SectionList&& other) HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

SectionList& SectionList::operator=(SectionList&& other) HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);

  return *this;
}

SectionList::~SectionList()
{ }

SectionList::iterator SectionList::begin()
{
  return SectionList::iterator(*impl_->process_, *impl_->pe_file_);
}

SectionList::const_iterator SectionList::begin() const
{
  return SectionList::iterator(*impl_->process_, *impl_->pe_file_);
}

SectionList::iterator SectionList::end() HADESMEM_NOEXCEPT
{
  return SectionList::iterator();
}

SectionList::const_iterator SectionList::end() const HADESMEM_NOEXCEPT
{
  return SectionList::iterator();
}

}
