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
  using BaseIteratorT = std::iterator<std::input_iterator_tag, ExportT>;
  using value_type = typename BaseIteratorT::value_type;
  using difference_type = typename BaseIteratorT::difference_type;
  using pointer = typename BaseIteratorT::pointer;
  using reference = typename BaseIteratorT::reference;
  using iterator_category = typename BaseIteratorT::iterator_category;

  HADESMEM_DETAIL_CONSTEXPR ExportIterator() HADESMEM_DETAIL_NOEXCEPT : impl_()
  {
  }

  explicit ExportIterator(Process const& process, PeFile const& pe_file)
  {
    try
    {
      ExportDir const export_dir{process, pe_file};
      Export const exp{
        process, pe_file, static_cast<WORD>(export_dir.GetOrdinalBase())};
      impl_ = std::make_shared<Impl>(process, pe_file, exp);
    }
    catch (std::exception const& /*e*/)
    {
      // Nothing to do here.
    }
  }

  explicit ExportIterator(Process&& process, PeFile const& pe_file) = delete;

  explicit ExportIterator(Process const& process, PeFile&& pe_file) = delete;

  explicit ExportIterator(Process&& process, PeFile&& pe_file) = delete;

#if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

  ExportIterator(ExportIterator const&) = default;

  ExportIterator& operator=(ExportIterator const&) = default;

  ExportIterator(ExportIterator&& other) HADESMEM_DETAIL_NOEXCEPT
    : impl_{std::move(other.impl_)}
  {
  }

  ExportIterator& operator=(ExportIterator&& other) HADESMEM_DETAIL_NOEXCEPT
  {
    impl_ = std::move(other.impl_);

    return *this;
  }

#endif // #if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

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

      ExportDir const export_dir{*impl_->process_, *impl_->pe_file_};

      DWORD* ptr_functions =
        static_cast<DWORD*>(RvaToVa(*impl_->process_,
                                    *impl_->pe_file_,
                                    export_dir.GetAddressOfFunctions()));

      DWORD const ordinal_base = export_dir.GetOrdinalBase();

      WORD const procedure_number = impl_->export_->GetProcedureNumber();

      WORD ordinal_number =
        static_cast<WORD>((procedure_number - ordinal_base) + 1);

      DWORD const num_funcs = export_dir.GetNumberOfFunctions();

      for (; ((ordinal_number + ordinal_base) >= ordinal_base) &&
               !Read<DWORD>(*impl_->process_, ptr_functions + ordinal_number) &&
               ordinal_number < num_funcs;
           ++ordinal_number)
      {
      }

      if ((ordinal_number + ordinal_base) < ordinal_base)
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(
          Error{} << ErrorString{"Ordinal number overflow."});
      }

      if (ordinal_number >= num_funcs)
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(
          Error{} << ErrorString{"Invalid export number."});
      }

      WORD const new_procedure_number =
        static_cast<WORD>(ordinal_number + ordinal_base);

      impl_->export_ =
        Export{*impl_->process_, *impl_->pe_file_, new_procedure_number};
    }
    catch (std::exception const& /*e*/)
    {
      impl_.reset();
    }

    return *this;
  }

  ExportIterator operator++(int)
  {
    ExportIterator const iter{*this};
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
    explicit Impl(Process const& process,
                  PeFile const& pe_file,
                  Export const& exp) HADESMEM_DETAIL_NOEXCEPT
      : process_{&process},
        pe_file_{&pe_file},
        export_(exp)
    {
    }

    Process const* process_;
    PeFile const* pe_file_;
    hadesmem::detail::Optional<Export> export_;
  };

  // Shallow copy semantics, as required by InputIterator.
  std::shared_ptr<Impl> impl_;
};

class ExportList
{
public:
  using value_type = Export;
  using iterator = ExportIterator<Export>;
  using const_iterator = ExportIterator<Export const>;

  explicit ExportList(Process const& process, PeFile const& pe_file)
    : process_{&process}, pe_file_{&pe_file}
  {
  }

  explicit ExportList(Process&& process, PeFile const& pe_file) = delete;

  explicit ExportList(Process const& process, PeFile&& pe_file) = delete;

  explicit ExportList(Process&& process, PeFile&& pe_file) = delete;

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
};
}
