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
#include <hadesmem/pelib/import_thunk.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>

namespace hadesmem
{

    // ImportThunkIterator satisfies the requirements of an input iterator 
    // (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
    template <typename ImportThunkT>
    class ImportThunkIterator : public std::iterator<
        std::input_iterator_tag,
        ImportThunkT>
    {
    public:
        typedef std::iterator<std::input_iterator_tag, ImportThunkT> 
            BaseIteratorT;
        typedef typename BaseIteratorT::value_type value_type;
        typedef typename BaseIteratorT::difference_type difference_type;
        typedef typename BaseIteratorT::pointer pointer;
        typedef typename BaseIteratorT::reference reference;
        typedef typename BaseIteratorT::iterator_category iterator_category;

        HADESMEM_DETAIL_CONSTEXPR ImportThunkIterator() 
            HADESMEM_DETAIL_NOEXCEPT
            : impl_()
        { }

        explicit ImportThunkIterator(
            Process const& process, 
            PeFile const& pe_file,
            DWORD first_thunk)
            : impl_(std::make_shared<Impl>(process, pe_file))
        {
            try
            {
                auto const thunk_ptr = reinterpret_cast<PIMAGE_THUNK_DATA>(
                    RvaToVa(
                    process, 
                    pe_file, 
                    first_thunk));
                impl_->import_thunk_ = ImportThunk(
                    process, 
                    pe_file, 
                    thunk_ptr);
                if (!impl_->import_thunk_->GetAddressOfData())
                {
                    impl_.reset();
                }
            }
            catch (std::exception const& /*e*/)
            {
                impl_.reset();
            }
        }

        ImportThunkIterator(ImportThunkIterator const& other) 
            HADESMEM_DETAIL_NOEXCEPT
            : impl_(other.impl_)
        { }

        ImportThunkIterator& operator=(ImportThunkIterator const& other)
            HADESMEM_DETAIL_NOEXCEPT
        {
            impl_ = other.impl_;

            return *this;
        }

        ImportThunkIterator(ImportThunkIterator&& other) 
            HADESMEM_DETAIL_NOEXCEPT
            : impl_(std::move(other.impl_))
        { }

        ImportThunkIterator& operator=(ImportThunkIterator&& other)
            HADESMEM_DETAIL_NOEXCEPT
        {
            impl_ = std::move(other.impl_);

            return *this;
        }

        reference operator*() const HADESMEM_DETAIL_NOEXCEPT
        {
            HADESMEM_DETAIL_ASSERT(impl_.get());
            return *impl_->import_thunk_;
        }

        pointer operator->() const HADESMEM_DETAIL_NOEXCEPT
        {
            HADESMEM_DETAIL_ASSERT(impl_.get());
            return &*impl_->import_thunk_;
        }

        ImportThunkIterator& operator++()
        {
            try
            {
                HADESMEM_DETAIL_ASSERT(impl_.get());

                auto const cur_base = reinterpret_cast<PIMAGE_THUNK_DATA>(
                    impl_->import_thunk_->GetBase());
                impl_->import_thunk_ = ImportThunk(
                    *impl_->process_, 
                    *impl_->pe_file_,
                    cur_base + 1);

                if (!impl_->import_thunk_->GetAddressOfData())
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

        ImportThunkIterator operator++(int)
        {
            ImportThunkIterator const iter(*this);
            ++*this;
            return iter;
        }

        bool operator==(ImportThunkIterator const& other) const 
            HADESMEM_DETAIL_NOEXCEPT
        {
            return impl_ == other.impl_;
        }

        bool operator!=(ImportThunkIterator const& other) const 
            HADESMEM_DETAIL_NOEXCEPT
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
            import_thunk_()
            { }

            Process const* process_;
            PeFile const* pe_file_;
            hadesmem::detail::Optional<ImportThunk> import_thunk_;
        };

        // Using a shared_ptr to provide shallow copy semantics, as 
        // required by InputIterator.
        std::shared_ptr<Impl> impl_;
    };

    class ImportThunkList
    {
    public:
        typedef ImportThunk value_type;
        typedef ImportThunkIterator<ImportThunk> iterator;
        typedef ImportThunkIterator<ImportThunk const> const_iterator;

        explicit ImportThunkList(
            Process const& process, 
            PeFile const& pe_file,
            DWORD first_thunk)
            : process_(&process),
            pe_file_(&pe_file),
            first_thunk_(first_thunk)
        { }

        iterator begin()
        {
            return ImportThunkList::iterator(
                *process_, 
                *pe_file_, 
                first_thunk_);
        }

        const_iterator begin() const
        {
            return ImportThunkList::const_iterator(
                *process_, 
                *pe_file_,
                first_thunk_);
        }

        const_iterator cbegin() const
        {
            return ImportThunkList::const_iterator(
                *process_, 
                *pe_file_,
                first_thunk_);
        }

        iterator end() HADESMEM_DETAIL_NOEXCEPT
        {
            return ImportThunkList::iterator();
        }

        const_iterator end() const HADESMEM_DETAIL_NOEXCEPT
        {
            return ImportThunkList::const_iterator();
        }

        const_iterator cend() const HADESMEM_DETAIL_NOEXCEPT
        {
            return ImportThunkList::const_iterator();
        }

    private:
        Process const* process_;
        PeFile const* pe_file_;
        DWORD first_thunk_;
    };

}
