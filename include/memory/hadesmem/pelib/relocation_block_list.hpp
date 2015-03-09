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
#include <hadesmem/pelib/nt_headers.hpp>
#include <hadesmem/pelib/relocation_block.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>

namespace hadesmem
{
// RelocationBlockIterator satisfies the requirements of an input iterator
// (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
template <typename RelocationBlockT>
class RelocationBlockIterator
  : public std::iterator<std::input_iterator_tag, RelocationBlockT>
{
public:
  using BaseIteratorT =
    std::iterator<std::input_iterator_tag, RelocationBlockT>;
  using value_type = typename BaseIteratorT::value_type;
  using difference_type = typename BaseIteratorT::difference_type;
  using pointer = typename BaseIteratorT::pointer;
  using reference = typename BaseIteratorT::reference;
  using iterator_category = typename BaseIteratorT::iterator_category;

  HADESMEM_DETAIL_CONSTEXPR RelocationBlockIterator() HADESMEM_DETAIL_NOEXCEPT
  {
  }

  explicit RelocationBlockIterator(Process const& process,
                                   PeFile const& pe_file)
  {
    try
    {
      NtHeaders const nt_headers{process, pe_file};

      DWORD const data_dir_va =
        nt_headers.GetDataDirectoryVirtualAddress(PeDataDir::BaseReloc);
      DWORD const size = nt_headers.GetDataDirectorySize(PeDataDir::BaseReloc);
      if (!data_dir_va || !size)
      {
        return;
      }

      auto base =
        static_cast<std::uint8_t*>(RvaToVa(process, pe_file, data_dir_va));
      if (!base)
      {
        return;
      }

      // Cast to integer and back to avoid pointer overflow UB.
      auto const reloc_dir_end = reinterpret_cast<void const*>(
        reinterpret_cast<std::uintptr_t>(base) + size);
      auto const file_end =
        static_cast<std::uint8_t*>(pe_file.GetBase()) + pe_file.GetSize();
      // Sample: virtrelocXP.exe
      if (pe_file.GetType() == PeFileType::Data &&
          (reloc_dir_end < base || reloc_dir_end > file_end))
      {
        return;
      }

      RelocationBlock const relocation_block{
        process,
        pe_file,
        reinterpret_cast<IMAGE_BASE_RELOCATION*>(base),
        reloc_dir_end};
      if (relocation_block.IsInvalid())
      {
        return;
      }

      impl_ = std::make_shared<Impl>(
        process, pe_file, relocation_block, reloc_dir_end);
    }
    catch (std::exception const& /*e*/)
    {
      // Nothing to do here.
    }
  }

  explicit RelocationBlockIterator(Process&& process,
                                   PeFile const& pe_file) = delete;

  explicit RelocationBlockIterator(Process const& process,
                                   PeFile&& pe_file) = delete;

  explicit RelocationBlockIterator(Process&& process,
                                   PeFile&& pe_file) = delete;

#if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

  RelocationBlockIterator(RelocationBlockIterator const&) = default;

  RelocationBlockIterator& operator=(RelocationBlockIterator const&) = default;

  RelocationBlockIterator(RelocationBlockIterator&& other)
    HADESMEM_DETAIL_NOEXCEPT : impl_{std::move(other.impl_)}
  {
  }

  RelocationBlockIterator&
    operator=(RelocationBlockIterator&& other) HADESMEM_DETAIL_NOEXCEPT
  {
    impl_ = std::move(other.impl_);

    return *this;
  }

#endif // #if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

  reference operator*() const HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return *impl_->relocation_block_;
  }

  pointer operator->() const HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_ASSERT(impl_.get());
    return &*impl_->relocation_block_;
  }

  RelocationBlockIterator& operator++()
  {
    try
    {
      HADESMEM_DETAIL_ASSERT(impl_.get());

      auto const next_base = reinterpret_cast<PIMAGE_BASE_RELOCATION>(
        reinterpret_cast<std::uintptr_t>(
          impl_->relocation_block_->GetRelocationDataStart()) +
        (impl_->relocation_block_->GetNumberOfRelocations() * sizeof(WORD)));
      if (next_base < impl_->relocation_block_->GetBase() ||
          next_base >= impl_->reloc_dir_end_)
      {
        impl_.reset();
        return *this;
      }
      impl_->relocation_block_ = RelocationBlock{
        *impl_->process_, *impl_->pe_file_, next_base, impl_->reloc_dir_end_};
      if (impl_->relocation_block_->IsInvalid())
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

  RelocationBlockIterator operator++(int)
  {
    RelocationBlockIterator const iter{*this};
    ++*this;
    return iter;
  }

  bool operator==(RelocationBlockIterator const& other) const
    HADESMEM_DETAIL_NOEXCEPT
  {
    return impl_ == other.impl_;
  }

  bool operator!=(RelocationBlockIterator const& other) const
    HADESMEM_DETAIL_NOEXCEPT
  {
    return !(*this == other);
  }

private:
  struct Impl
  {
    explicit Impl(Process const& process,
                  PeFile const& pe_file,
                  RelocationBlock const& relocation_block,
                  void const* reloc_dir_end) HADESMEM_DETAIL_NOEXCEPT
      : process_(&process),
        pe_file_(&pe_file),
        relocation_block_(relocation_block),
        reloc_dir_end_(reloc_dir_end)
    {
    }

    Process const* process_;
    PeFile const* pe_file_;
    hadesmem::detail::Optional<RelocationBlock> relocation_block_;
    void const* reloc_dir_end_;
  };

  // Shallow copy semantics, as required by InputIterator.
  std::shared_ptr<Impl> impl_;
};

class RelocationBlockList
{
public:
  using value_type = RelocationBlock;
  using iterator = RelocationBlockIterator<RelocationBlock>;
  using const_iterator = RelocationBlockIterator<RelocationBlock const>;

  explicit RelocationBlockList(Process const& process, PeFile const& pe_file)
    : process_{&process}, pe_file_{&pe_file}
  {
  }

  explicit RelocationBlockList(Process&& process,
                               PeFile const& pe_file) = delete;

  explicit RelocationBlockList(Process const& process,
                               PeFile&& pe_file) = delete;

  explicit RelocationBlockList(Process&& process, PeFile&& pe_file) = delete;

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
