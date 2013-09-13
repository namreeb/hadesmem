// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <memory>
#include <utility>
#include <iterator>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/optional.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <windows.h>

#include <hadesmem/read.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/pelib/import_dir.hpp>

// TODO: Distinguish between IAT and ILT in import enumeration.

namespace hadesmem
{

// ImportDirIterator satisfies the requirements of an input iterator 
// (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
template <typename ImportDirT>
class ImportDirIterator : public std::iterator<std::input_iterator_tag, 
  ImportDirT>
{
public:
  typedef std::iterator<std::input_iterator_tag, ImportDirT> BaseIteratorT;
  typedef typename BaseIteratorT::value_type value_type;
  typedef typename BaseIteratorT::difference_type difference_type;
  typedef typename BaseIteratorT::pointer pointer;
  typedef typename BaseIteratorT::reference reference;
  typedef typename BaseIteratorT::iterator_category iterator_category;

  ImportDirIterator() HADESMEM_DETAIL_NOEXCEPT
    : impl_()
  { }
  
  explicit ImportDirIterator(Process const& process, PeFile const& pe_file)
    : impl_(std::make_shared<Impl>(process, pe_file))
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

  ImportDirIterator(ImportDirIterator const& other) HADESMEM_DETAIL_NOEXCEPT
    : impl_(other.impl_)
  { }

  ImportDirIterator& operator=(ImportDirIterator const& other) 
    HADESMEM_DETAIL_NOEXCEPT
  {
    impl_ = other.impl_;

    return *this;
  }

  ImportDirIterator(ImportDirIterator&& other) HADESMEM_DETAIL_NOEXCEPT
    : impl_(std::move(other.impl_))
  { }

  ImportDirIterator& operator=(ImportDirIterator&& other) HADESMEM_DETAIL_NOEXCEPT
  {
    impl_ = std::move(other.impl_);

    return *this;
  }

  reference operator*() const HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return *impl_->import_dir_;
  }
  
  pointer operator->() const HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return &*impl_->import_dir_;
  }
  
  ImportDirIterator& operator++()
  {
    try
    {
      HADESMEM_DETAIL_ASSERT(impl_.get());

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
  
  ImportDirIterator operator++(int)
  {
    ImportDirIterator iter(*this);
    ++*this;
    return iter;
  }
  
  bool operator==(ImportDirIterator const& other) const HADESMEM_DETAIL_NOEXCEPT
  {
    return impl_ == other.impl_;
  }
  
  bool operator!=(ImportDirIterator const& other) const HADESMEM_DETAIL_NOEXCEPT
  {
    return !(*this == other);
  }
  
private:
  struct Impl
  {
    explicit Impl(Process const& process, PeFile const& pe_file) 
      HADESMEM_DETAIL_NOEXCEPT
      : process_(&process), 
      pe_file_(&pe_file), 
      import_dir_()
    { }

    Process const* process_;
    PeFile const* pe_file_;
    boost::optional<ImportDir> import_dir_;
  };

  // Using a shared_ptr to provide shallow copy semantics, as 
  // required by InputIterator.
  std::shared_ptr<Impl> impl_;
};

class ImportDirList
{
public:
  typedef ImportDir value_type;
  typedef ImportDirIterator<ImportDir> iterator;
  typedef ImportDirIterator<ImportDir const> const_iterator;
  
  explicit ImportDirList(Process const& process, PeFile const& pe_file)
    : process_(&process), 
    pe_file_(&pe_file)
  { }

  iterator begin()
  {
    return ImportDirList::iterator(*process_, *pe_file_);
  }
  
  const_iterator begin() const
  {
    return ImportDirList::const_iterator(*process_, *pe_file_);
  }
  
  const_iterator cbegin() const
  {
    return ImportDirList::const_iterator(*process_, *pe_file_);
  }
  
  iterator end() HADESMEM_DETAIL_NOEXCEPT
  {
    return ImportDirList::iterator();
  }
  
  const_iterator end() const HADESMEM_DETAIL_NOEXCEPT
  {
    return ImportDirList::const_iterator();
  }
  
  const_iterator cend() const HADESMEM_DETAIL_NOEXCEPT
  {
    return ImportDirList::const_iterator();
  }
  
private:
  Process const* process_;
  PeFile const* pe_file_;
};

}
