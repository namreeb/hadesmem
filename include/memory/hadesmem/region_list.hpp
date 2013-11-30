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
#include <hadesmem/detail/query_region.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/protect.hpp>
#include <hadesmem/region.hpp>

namespace hadesmem
{

    // RegionIterator satisfies the requirements of an input iterator 
    // (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
    template <typename RegionT>
    class RegionIterator : public std::iterator<
        std::input_iterator_tag, 
        RegionT>
    {
    public:
        using BaseIteratorT = std::iterator<std::input_iterator_tag, RegionT>;
        using value_type = typename BaseIteratorT::value_type;
        using difference_type = typename BaseIteratorT::difference_type;
        using pointer = typename BaseIteratorT::pointer;
        using reference = typename BaseIteratorT::reference;
        using iterator_category = typename BaseIteratorT::iterator_category;

        HADESMEM_DETAIL_CONSTEXPR RegionIterator() HADESMEM_DETAIL_NOEXCEPT
            : impl_()
        { }

        explicit RegionIterator(Process const& process)
            : impl_(std::make_shared<Impl>(process))
        { }

#if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

        RegionIterator(RegionIterator const&) = default;

        RegionIterator& operator=(RegionIterator const&) = default;

        RegionIterator(RegionIterator&& other) HADESMEM_DETAIL_NOEXCEPT
            : impl_(std::move(other.impl_))
        { }

        RegionIterator& operator=(RegionIterator&& other) 
            HADESMEM_DETAIL_NOEXCEPT
        {
            impl_ = std::move(other.impl_);

            return *this;
        }

#endif // #if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

        reference operator*() const HADESMEM_DETAIL_NOEXCEPT
        {
            HADESMEM_DETAIL_ASSERT(impl_.get());
            return *impl_->region_;
        }

        pointer operator->() const HADESMEM_DETAIL_NOEXCEPT
        {
            HADESMEM_DETAIL_ASSERT(impl_.get());
            return &*impl_->region_;
        }

        RegionIterator& operator++()
        {
            try
            {
                HADESMEM_DETAIL_ASSERT(impl_.get());

                void const* const base = impl_->region_->GetBase();
                SIZE_T const size = impl_->region_->GetSize();
                auto const next = static_cast<char const* const>(base)+size;
                MEMORY_BASIC_INFORMATION const mbi = detail::Query(
                    *impl_->process_,
                    next);
                impl_->region_ = Region(*impl_->process_, mbi);
            }
            catch (std::exception const& /*e*/)
            {
                impl_.reset();
            }

            return *this;
        }

        RegionIterator operator++(int)
        {
            RegionIterator const iter(*this);
            ++*this;
            return iter;
        }


        bool operator==(RegionIterator const& other) const 
            HADESMEM_DETAIL_NOEXCEPT
        {
            return impl_ == other.impl_;
        }

        bool operator!=(RegionIterator const& other) const 
            HADESMEM_DETAIL_NOEXCEPT
        {
            return !(*this == other);
        }

    private:
        struct Impl
        {
            explicit Impl(Process const& process) HADESMEM_DETAIL_NOEXCEPT
            : process_(&process),
            region_()
            {
                MEMORY_BASIC_INFORMATION const mbi = 
                    detail::Query(process, nullptr);
                region_ = Region(process, mbi);
            }

            Process const* process_;
            hadesmem::detail::Optional<Region> region_;
        };

        // Using a shared_ptr to provide shallow copy semantics, as 
        // required by InputIterator.
        std::shared_ptr<Impl> impl_;
    };

    class RegionList
    {
    public:
        using value_type = Region;
        using iterator = RegionIterator<Region>;
        using const_iterator = RegionIterator<Region const>;

        explicit RegionList(Process const& process)
            : process_(&process)
        { }

        iterator begin()
        {
            return iterator(*process_);
        }

        const_iterator begin() const
        {
            return const_iterator(*process_);
        }

        const_iterator cbegin() const
        {
            return const_iterator(*process_);
        }

        iterator end() HADESMEM_DETAIL_NOEXCEPT
        {
            return iterator();
        }

        const_iterator end() const HADESMEM_DETAIL_NOEXCEPT
        {
            return const_iterator();
        }

        const_iterator cend() const HADESMEM_DETAIL_NOEXCEPT
        {
            return const_iterator();
        }

    private:
        Process const* process_;
    };

}
