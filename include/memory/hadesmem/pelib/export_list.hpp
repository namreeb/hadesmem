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
#include <hadesmem/pelib/export.hpp>
#include <hadesmem/pelib/export_dir.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>

namespace hadesmem
{

// ExportIterator satisfies the requirements of an input iterator 
// (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
template <typename ExportT>
class ExportIterator : public std::iterator<std::input_iterator_tag, ExportT>
{
public:
  typedef std::iterator<std::input_iterator_tag, ExportT> BaseIteratorT;
  typedef typename BaseIteratorT::value_type value_type;
  typedef typename BaseIteratorT::difference_type difference_type;
  typedef typename BaseIteratorT::pointer pointer;
  typedef typename BaseIteratorT::reference reference;
  typedef typename BaseIteratorT::iterator_category iterator_category;

  HADESMEM_DETAIL_CONSTEXPR ExportIterator() HADESMEM_DETAIL_NOEXCEPT
    : impl_()
  { }
  
  explicit ExportIterator(Process const& process, PeFile const& pe_file)
    : impl_(std::make_shared<Impl>(process, pe_file))
  {
    try
    {
      ExportDir const export_dir(process, pe_file);
      impl_->export_ = Export(process, pe_file, static_cast<WORD>(
        export_dir.GetOrdinalBase()));
    }
    catch (std::exception const& /*e*/)
    {
      impl_.reset();
    }
  }

  ExportIterator(ExportIterator const& other) HADESMEM_DETAIL_NOEXCEPT
    : impl_(other.impl_)
  { }

  ExportIterator& operator=(ExportIterator const& other) HADESMEM_DETAIL_NOEXCEPT
  {
    impl_ = other.impl_;

    return *this;
  }

  ExportIterator(ExportIterator&& other) HADESMEM_DETAIL_NOEXCEPT
    : impl_(std::move(other.impl_))
  { }

  ExportIterator& operator=(ExportIterator&& other) HADESMEM_DETAIL_NOEXCEPT
  {
    impl_ = std::move(other.impl_);

    return *this;
  }

  reference operator*() const HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return *impl_->export_;
  }
  
  pointer operator->() const HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return &*impl_->export_;
  }
  
  ExportIterator& operator++()
  {
    try
    {
      HADESMEM_DETAIL_ASSERT(impl_.get());
    
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
        HADESMEM_DETAIL_THROW_EXCEPTION(Error() << 
          ErrorString("Invalid export number."));
      }

      WORD const new_ordinal = static_cast<WORD>(offset + 
        export_dir.GetOrdinalBase());

      impl_->export_ = Export(*impl_->process_, *impl_->pe_file_, new_ordinal);
    }
    catch (std::exception const& /*e*/)
    {
      impl_.reset();
    }
  
    return *this;
  }
  
  ExportIterator operator++(int)
  {
    ExportIterator const iter(*this);
    ++*this;
    return iter;
  }
  
  bool operator==(ExportIterator const& other) const HADESMEM_DETAIL_NOEXCEPT
  {
    return impl_ == other.impl_;
  }
  
  bool operator!=(ExportIterator const& other) const HADESMEM_DETAIL_NOEXCEPT
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
      export_()
    { }

    Process const* process_;
    PeFile const* pe_file_;
    hadesmem::detail::Optional<Export> export_;
  };

  // Using a shared_ptr to provide shallow copy semantics, as 
  // required by InputIterator.
  std::shared_ptr<Impl> impl_;
};

class ExportList
{
public:
  typedef Export value_type;
  typedef ExportIterator<Export> iterator;
  typedef ExportIterator<Export const> const_iterator;
  
  explicit ExportList(Process const& process, PeFile const& pe_file)
    : process_(&process), 
    pe_file_(&pe_file)
  { }

  iterator begin()
  {
    return ExportList::iterator(*process_, *pe_file_);
  }
  
  const_iterator begin() const
  {
    return ExportList::const_iterator(*process_, *pe_file_);
  }
  
  const_iterator cbegin() const
  {
    return ExportList::const_iterator(*process_, *pe_file_);
  }
  
  iterator end() HADESMEM_DETAIL_NOEXCEPT
  {
    return ExportList::iterator();
  }
  
  const_iterator end() const HADESMEM_DETAIL_NOEXCEPT
  {
    return ExportList::const_iterator();
  }
  
  const_iterator cend() const HADESMEM_DETAIL_NOEXCEPT
  {
    return ExportList::const_iterator();
  }
  
private:
  Process const* process_;
  PeFile const* pe_file_;
};

}
