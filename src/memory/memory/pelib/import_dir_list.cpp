// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/pelib/import_dir_list.hpp>

#include <utility>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/assert.hpp>
#include <boost/optional.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <windows.h>

#include <hadesmem/read.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/pelib/import_dir.hpp>

namespace hadesmem
{

struct ImportDirIterator::Impl
{
  explicit Impl(Process const& process, PeFile const& pe_file) 
    HADESMEM_NOEXCEPT
    : process_(&process), 
    pe_file_(&pe_file), 
    import_dir_()
  { }

  Process const* process_;
  PeFile const* pe_file_;
  boost::optional<ImportDir> import_dir_;
};

ImportDirIterator::ImportDirIterator() HADESMEM_NOEXCEPT
  : impl_()
{ }

ImportDirIterator::ImportDirIterator(Process const& process, 
  PeFile const& pe_file)
  : impl_(new Impl(process, pe_file))
{
  try
  {
    impl_->import_dir_ = ImportDir(process, pe_file, nullptr);
  }
  catch (std::exception const& /*e*/)
  {
    // TODO: Check whether this is the right thing to do. We should only 
    // flag as the 'end' once we've actually reached the end of the list. If 
    // the iteration fails we should throw an exception.
    impl_.reset();
  }
}

ImportDirIterator::ImportDirIterator(ImportDirIterator const& other) 
  HADESMEM_NOEXCEPT
  : impl_(other.impl_)
{ }

ImportDirIterator& ImportDirIterator::operator=(ImportDirIterator const& other) 
  HADESMEM_NOEXCEPT
{
  impl_ = other.impl_;

  return *this;
}

ImportDirIterator::ImportDirIterator(ImportDirIterator&& other) 
  HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

ImportDirIterator& ImportDirIterator::operator=(ImportDirIterator&& other) 
  HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);

  return *this;
}

ImportDirIterator::~ImportDirIterator()
{ }

ImportDirIterator::reference ImportDirIterator::operator*() const 
  HADESMEM_NOEXCEPT
{
  BOOST_ASSERT(impl_.get());
  return *impl_->import_dir_;
}

ImportDirIterator::pointer ImportDirIterator::operator->() const 
  HADESMEM_NOEXCEPT
{
  BOOST_ASSERT(impl_.get());
  return &*impl_->import_dir_;
}

ImportDirIterator& ImportDirIterator::operator++()
{
  try
  {
    BOOST_ASSERT(impl_.get());

    auto const cur_base = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(
      impl_->import_dir_->GetBase());
    impl_->import_dir_ = ImportDir(*impl_->process_, *impl_->pe_file_, 
      cur_base + 1);
    
    // If the Name is NULL then the other fields can be non-NULL but the 
    // entire entry will still be skipped by the Windows loader.
    bool const has_name = impl_->import_dir_->GetNameRaw() != 0;
    bool const has_ilt_or_iat = impl_->import_dir_->GetOriginalFirstThunk() || 
      impl_->import_dir_->GetFirstThunk();
    if (!has_name || !has_ilt_or_iat)
    {
      impl_.reset();
      return *this;
    }
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

ImportDirIterator ImportDirIterator::operator++(int)
{
  ImportDirIterator iter(*this);
  ++*this;
  return iter;
}

bool ImportDirIterator::operator==(ImportDirIterator const& other) const 
  HADESMEM_NOEXCEPT
{
  return impl_ == other.impl_;
}

bool ImportDirIterator::operator!=(ImportDirIterator const& other) const 
  HADESMEM_NOEXCEPT
{
  return !(*this == other);
}

struct ImportDirList::Impl
{
  Impl(Process const& process, PeFile const& pe_file)
    : process_(&process), 
    pe_file_(&pe_file)
  { }

  Process const* process_;
  PeFile const* pe_file_;
};

ImportDirList::ImportDirList(Process const& process, PeFile const& pe_file)
  : impl_(new Impl(process, pe_file))
{ }

ImportDirList::ImportDirList(ImportDirList const& other)
  : impl_(new Impl(*other.impl_))
{ }

ImportDirList& ImportDirList::operator=(ImportDirList const& other)
{
  impl_ = std::unique_ptr<Impl>(new Impl(*other.impl_));

  return *this;
}

ImportDirList::ImportDirList(ImportDirList&& other) HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

ImportDirList& ImportDirList::operator=(ImportDirList&& other) HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);

  return *this;
}

ImportDirList::~ImportDirList()
{ }

ImportDirList::iterator ImportDirList::begin()
{
  return ImportDirList::iterator(*impl_->process_, *impl_->pe_file_);
}

ImportDirList::const_iterator ImportDirList::begin() const
{
  return ImportDirList::iterator(*impl_->process_, *impl_->pe_file_);
}

ImportDirList::iterator ImportDirList::end() HADESMEM_NOEXCEPT
{
  return ImportDirList::iterator();
}

ImportDirList::const_iterator ImportDirList::end() const HADESMEM_NOEXCEPT
{
  return ImportDirList::iterator();
}

}
