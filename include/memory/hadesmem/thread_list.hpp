// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <memory>
#include <utility>
#include <iterator>

#include <windows.h>
#include <tlhelp32.h>

#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/thread_entry.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/optional.hpp>
#include <hadesmem/detail/toolhelp.hpp>
#include <hadesmem/detail/smart_handle.hpp>

namespace hadesmem
{

    // ThreadIterator satisfies the requirements of an input iterator 
    // (C++ Standard, 24.2.1, Input Iterators [input.iterators]).
    template <typename ThreadEntryT>
    class ThreadIterator : public std::iterator<
        std::input_iterator_tag,
        ThreadEntryT>
    {
    public:
        using BaseIteratorT = std::iterator<
            std::input_iterator_tag, 
            ThreadEntryT>;
        using value_type = typename BaseIteratorT::value_type;
        using difference_type = typename BaseIteratorT::difference_type;
        using pointer = typename BaseIteratorT::pointer;
        using reference = typename BaseIteratorT::reference;
        using iterator_category = typename BaseIteratorT::iterator_category;

        HADESMEM_DETAIL_CONSTEXPR ThreadIterator() HADESMEM_DETAIL_NOEXCEPT
            : impl_(),
            pid_(0)
        { }

        ThreadIterator(DWORD pid) HADESMEM_DETAIL_NOEXCEPT
            : impl_(std::make_shared<Impl>()),
            pid_(pid)
        {
            HADESMEM_DETAIL_ASSERT(impl_.get());

            impl_->snap_ = detail::CreateToolhelp32Snapshot(
                TH32CS_SNAPTHREAD,
                0);

            hadesmem::detail::Optional<THREADENTRY32> const entry =
                detail::Thread32First(impl_->snap_.GetHandle());
            if (!entry)
            {
                impl_.reset();
                return;
            }

            if (IsTargetThread(entry->th32OwnerProcessID))
            {
                impl_->thread_ = ThreadEntry(*entry);
            }
            else
            {
                Advance();
            }
        }

#if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

        ThreadIterator(ThreadIterator const&) = default;

        ThreadIterator& operator=(ThreadIterator const&) = default;

        ThreadIterator(ThreadIterator&& other) HADESMEM_DETAIL_NOEXCEPT
            : impl_(std::move(other.impl_)),
            pid_(other.pid_)
        { }

        ThreadIterator& operator=(ThreadIterator&& other)
            HADESMEM_DETAIL_NOEXCEPT
        {
            impl_ = std::move(other.impl_);
            pid_ = other.pid_;

            return *this;
        }

#endif // #if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

        reference operator*() const HADESMEM_DETAIL_NOEXCEPT
        {
            HADESMEM_DETAIL_ASSERT(impl_.get());
            return *impl_->thread_;
        }

        pointer operator->() const HADESMEM_DETAIL_NOEXCEPT
        {
            HADESMEM_DETAIL_ASSERT(impl_.get());
            return &*impl_->thread_;
        }

        ThreadIterator& operator++()
        {
            HADESMEM_DETAIL_ASSERT(impl_.get());

            Advance();

            return *this;
        }

        ThreadIterator operator++(int)
        {
            ThreadIterator const iter(*this);
            ++*this;
            return iter;
        }

        bool operator==(ThreadIterator const& other) const 
            HADESMEM_DETAIL_NOEXCEPT
        {
            return impl_ == other.impl_;
        }

        bool operator!=(ThreadIterator const& other) const 
            HADESMEM_DETAIL_NOEXCEPT
        {
            return !(*this == other);
        }

    private:
        void Advance()
        {
            for (;;)
            {
                hadesmem::detail::Optional<THREADENTRY32> const entry =
                    detail::Thread32Next(impl_->snap_.GetHandle());
                if (!entry)
                {
                    impl_.reset();
                    break;
                }

                if (IsTargetThread(entry->th32OwnerProcessID))
                {
                    impl_->thread_ = ThreadEntry(*entry);
                    break;
                }
            }
        }

        bool IsTargetThread(DWORD owner_id) HADESMEM_DETAIL_NOEXCEPT
        {
            return pid_ == static_cast<DWORD>(-1) || pid_ == owner_id;
        }

        struct Impl
        {
            Impl() HADESMEM_DETAIL_NOEXCEPT
            : snap_(INVALID_HANDLE_VALUE),
            thread_()
            { }

            detail::SmartSnapHandle snap_;
            hadesmem::detail::Optional<ThreadEntry> thread_;
        };

        // Using a shared_ptr to provide shallow copy semantics, as 
        // required by InputIterator.
        std::shared_ptr<Impl> impl_;
        DWORD pid_;
    };

    class ThreadList
    {
    public:
        using value_type = ThreadEntry;
        using iterator = ThreadIterator<ThreadEntry>;
        using const_iterator = ThreadIterator<ThreadEntry const>;

        HADESMEM_DETAIL_CONSTEXPR ThreadList() HADESMEM_DETAIL_NOEXCEPT
            : pid_(static_cast<DWORD>(-1))
        { }

        HADESMEM_DETAIL_CONSTEXPR ThreadList(DWORD pid) 
            HADESMEM_DETAIL_NOEXCEPT
            : pid_(pid)
        { }

        iterator begin()
        {
            return iterator(pid_);
        }

        const_iterator begin() const
        {
            return const_iterator(pid_);
        }

        const_iterator cbegin() const
        {
            return const_iterator(pid_);
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
        DWORD pid_;
    };

}
