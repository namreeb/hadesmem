// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/pelib/import_thunk_list.hpp>

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
#include <hadesmem/pelib/import_thunk.hpp>

namespace hadesmem
{

struct ImportThunkIterator::Impl
{
  explicit Impl(Process const& process, PeFile const& pe_file) 
    HADESMEM_NOEXCEPT
    : process_(&process), 
    pe_file_(&pe_file), 
    import_thunk_()
  { }

  Process const* process_;
  PeFile const* pe_file_;
  boost::optional<ImportThunk> import_thunk_;
};

ImportThunkIterator::ImportThunkIterator() HADESMEM_NOEXCEPT
  : impl_()
{ }

ImportThunkIterator::ImportThunkIterator(Process const& process, 
  PeFile const& pe_file, DWORD first_thunk)
  : impl_(new Impl(process, pe_file))
{
  try
  {
    auto const thunk_ptr = reinterpret_cast<PIMAGE_THUNK_DATA>(RvaToVa(
      process, pe_file, first_thunk));
    impl_->import_thunk_ = ImportThunk(process, pe_file, thunk_ptr);
    if (!impl_->import_thunk_->GetAddressOfData())
    {
      impl_.reset();
    }
  }
  catch (std::exception const& /*e*/)
  {
    // TODO: Check whether this is the right thing to do. We should only 
    // flag as the 'end' once we've actually reached the end of the list. If 
    // the iteration fails we should throw an exception.
    impl_.reset();
  }
}

ImportThunkIterator::ImportThunkIterator(ImportThunkIterator const& other) 
  HADESMEM_NOEXCEPT
  : impl_(other.impl_)
{ }

ImportThunkIterator& ImportThunkIterator::operator=(
  ImportThunkIterator const& other) HADESMEM_NOEXCEPT
{
  impl_ = other.impl_;

  return *this;
}

ImportThunkIterator::ImportThunkIterator(ImportThunkIterator&& other) 
  HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

ImportThunkIterator& ImportThunkIterator::operator=(
  ImportThunkIterator&& other) HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);

  return *this;
}

ImportThunkIterator::~ImportThunkIterator()
{ }

ImportThunkIterator::reference ImportThunkIterator::operator*() const 
  HADESMEM_NOEXCEPT
{
  BOOST_ASSERT(impl_.get());
  return *impl_->import_thunk_;
}

ImportThunkIterator::pointer ImportThunkIterator::operator->() const 
  HADESMEM_NOEXCEPT
{
  BOOST_ASSERT(impl_.get());
  return &*impl_->import_thunk_;
}

ImportThunkIterator& ImportThunkIterator::operator++()
{
  try
  {
    BOOST_ASSERT(impl_.get());

    auto const cur_base = reinterpret_cast<PIMAGE_THUNK_DATA>(
      impl_->import_thunk_->GetBase());
    impl_->import_thunk_ = ImportThunk(*impl_->process_, *impl_->pe_file_, 
      cur_base + 1);

    if (!impl_->import_thunk_->GetAddressOfData())
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

ImportThunkIterator ImportThunkIterator::operator++(int)
{
  ImportThunkIterator iter(*this);
  ++*this;
  return iter;
}

bool ImportThunkIterator::operator==(ImportThunkIterator const& other) const 
  HADESMEM_NOEXCEPT
{
  return impl_ == other.impl_;
}

bool ImportThunkIterator::operator!=(ImportThunkIterator const& other) const 
  HADESMEM_NOEXCEPT
{
  return !(*this == other);
}

struct ImportThunkList::Impl
{
  Impl(Process const& process, PeFile const& pe_file, DWORD first_thunk)
    : process_(&process), 
    pe_file_(&pe_file), 
    first_thunk_(first_thunk)
  { }

  Process const* process_;
  PeFile const* pe_file_;
  DWORD first_thunk_;
};

ImportThunkList::ImportThunkList(Process const& process, PeFile const& pe_file, 
  DWORD first_thunk)
  : impl_(new Impl(process, pe_file, first_thunk))
{ }

ImportThunkList::ImportThunkList(ImportThunkList const& other)
  : impl_(new Impl(*other.impl_))
{ }

ImportThunkList& ImportThunkList::operator=(ImportThunkList const& other)
{
  impl_ = std::unique_ptr<Impl>(new Impl(*other.impl_));

  return *this;
}

ImportThunkList::ImportThunkList(ImportThunkList&& other) HADESMEM_NOEXCEPT
  : impl_(std::move(other.impl_))
{ }

ImportThunkList& ImportThunkList::operator=(ImportThunkList&& other) 
  HADESMEM_NOEXCEPT
{
  impl_ = std::move(other.impl_);

  return *this;
}

ImportThunkList::~ImportThunkList()
{ }

ImportThunkList::iterator ImportThunkList::begin()
{
  return ImportThunkList::iterator(*impl_->process_, *impl_->pe_file_, 
    impl_->first_thunk_);
}

ImportThunkList::const_iterator ImportThunkList::begin() const
{
  return ImportThunkList::iterator(*impl_->process_, *impl_->pe_file_, 
    impl_->first_thunk_);
}

ImportThunkList::iterator ImportThunkList::end() HADESMEM_NOEXCEPT
{
  return ImportThunkList::iterator();
}

ImportThunkList::const_iterator ImportThunkList::end() const HADESMEM_NOEXCEPT
{
  return ImportThunkList::iterator();
}

}
