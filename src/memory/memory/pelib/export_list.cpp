// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/pelib/export_list.hpp"

#include <utility>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/assert.hpp>
#include <boost/optional.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include <windows.h>

#include "hadesmem/read.hpp"
#include "hadesmem/error.hpp"
#include "hadesmem/config.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/pelib/export.hpp"
#include "hadesmem/pelib/pe_file.hpp"
#include "hadesmem/pelib/export_dir.hpp"

namespace hadesmem
{

struct ExportIterator::Impl
{
  explicit Impl(Process const& process, PeFile const& pe_file) 
    HADESMEM_NOEXCEPT
    : process_(&process), 
    pe_file_(&pe_file), 
    export_()
  { }

  Process const* process_;
  PeFile const* pe_file_;
  boost::optional<Export> export_;
};

ExportIterator::ExportIterator() HADESMEM_NOEXCEPT
  : impl_()
{ }

ExportIterator::ExportIterator(Process const& process, PeFile const& pe_file)
  : impl_(new Impl(process, pe_file))
{
  try
  {
    ExportDir const export_dir(process, pe_file);
    impl_->export_ = Export(process, pe_file, static_cast<WORD>(
      export_dir.GetOrdinalBase()));
  }
  catch (std::exception const& /*e*/)
  {
    // TODO: Check whether this is the right thing to do. We should only 
    // flag as the 'end' once we've actually reached the end of the list. If 
    // the iteration fails we should throw an exception.
    impl_.reset();
  }
}

ExportIterator::ExportIterator(ExportIterator const& other) HADESMEM_NOEXCEPT
  : impl_(other.impl_)
{ }

ExportIterator& ExportIterator::operator=(ExportIterator const& other) 
  HADESMEM_NOEXCEPT
{
  impl_ = other.impl_;

  return *this;
}

ExportIterator::ExportIterator(ExportIterator&& other) HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

ExportIterator& ExportIterator::operator=(ExportIterator&& other) 
  HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);

  return *this;
}

ExportIterator::~ExportIterator()
{ }

ExportIterator::reference ExportIterator::operator*() const HADESMEM_NOEXCEPT
{
  BOOST_ASSERT(impl_.get());
  return *impl_->export_;
}

ExportIterator::pointer ExportIterator::operator->() const HADESMEM_NOEXCEPT
{
  BOOST_ASSERT(impl_.get());
  return &*impl_->export_;
}

ExportIterator& ExportIterator::operator++()
{
  try
  {
    BOOST_ASSERT(impl_.get());
    
    ExportDir const export_dir(*impl_->process_, *impl_->pe_file_);
    DWORD* ptr_functions = static_cast<DWORD*>(RvaToVa(*impl_->process_, 
      *impl_->pe_file_, export_dir.GetAddressOfFunctions()));

    WORD offset = static_cast<WORD>(impl_->export_->GetOrdinal() - 
      export_dir.GetOrdinalBase() + 1);
    for (; !Read<DWORD>(*impl_->process_, ptr_functions + offset) && 
      offset < export_dir.GetNumberOfFunctions(); ++offset)
      ;

    if (offset >= export_dir.GetNumberOfFunctions())
    {
      HADESMEM_THROW_EXCEPTION(Error() << 
        ErrorString("Invalid export number."));
    }

    WORD const new_ordinal = static_cast<WORD>(offset + 
      export_dir.GetOrdinalBase());

    impl_->export_ = Export(*impl_->process_, *impl_->pe_file_, new_ordinal);
  }
  catch (std::exception const& /*e*/)
  {
    // TODO: Check whether this is the right thing to do. We should only 
    // flag as the 'end' once we've actually reached the end of the list. If 
    // the iteration fails we should throw an exception.
    impl_.reset();
  }
  
  return *this;
}

ExportIterator ExportIterator::operator++(int)
{
  ExportIterator iter(*this);
  ++*this;
  return iter;
}

bool ExportIterator::operator==(ExportIterator const& other) const 
  HADESMEM_NOEXCEPT
{
  return impl_ == other.impl_;
}

bool ExportIterator::operator!=(ExportIterator const& other) const 
  HADESMEM_NOEXCEPT
{
  return !(*this == other);
}

struct ExportList::Impl
{
  Impl(Process const& process, PeFile const& pe_file)
    : process_(&process), 
    pe_file_(&pe_file)
  { }

  Process const* process_;
  PeFile const* pe_file_;
};

ExportList::ExportList(Process const& process, PeFile const& pe_file)
  : impl_(new Impl(process, pe_file))
{ }

ExportList::ExportList(ExportList const& other)
  : impl_(new Impl(*other.impl_))
{ }

ExportList& ExportList::operator=(ExportList const& other)
{
  impl_ = std::unique_ptr<Impl>(new Impl(*other.impl_));

  return *this;
}

ExportList::ExportList(ExportList&& other) HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

ExportList& ExportList::operator=(ExportList&& other) HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);

  return *this;
}

ExportList::~ExportList()
{ }

ExportList::iterator ExportList::begin()
{
  return ExportList::iterator(*impl_->process_, *impl_->pe_file_);
}

ExportList::const_iterator ExportList::begin() const
{
  return ExportList::iterator(*impl_->process_, *impl_->pe_file_);
}

ExportList::iterator ExportList::end() HADESMEM_NOEXCEPT
{
  return ExportList::iterator();
}

ExportList::const_iterator ExportList::end() const HADESMEM_NOEXCEPT
{
  return ExportList::iterator();
}

}
