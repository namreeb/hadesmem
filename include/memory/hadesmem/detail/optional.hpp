// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <type_traits>
#include <utility>

#include <hadesmem/config.hpp>

// TODO: Implement same interface as std::optional<T> to make switching later 
// easier.

// TODO: Add tests.

// TODO: Add constexpr support. (http://bit.ly/U7lSYy)

namespace hadesmem
{

    namespace detail
    {

        // WARNING: T must have no-throw move, no-throw move assignment and 
        // no-throw destruction.
        template <typename T>
        class Optional
        {
        private:
            typedef void (Optional::* Boolean)() const;

        public:
            HADESMEM_DETAIL_CONSTEXPR Optional() HADESMEM_DETAIL_NOEXCEPT
                : t_(),
                valid_(false)
            { }

            explicit Optional(T const& t)
                : t_(),
                valid_(false)
            {
                Construct(t);
            }

            // TODO: Conditional noexcept.
            explicit Optional(T&& t)
                : t_(),
                valid_(false)
            {
                Construct(std::move(t));
            }

            Optional(Optional const& other)
                : t_(),
                valid_(false)
            {
                Construct(other.Get());
            }

            Optional& operator=(Optional const& other)
            {
                Optional tmp(other);
                *this = std::move(tmp);

                return *this;
            }

            Optional(Optional&& other) HADESMEM_DETAIL_NOEXCEPT
                : t_(),
                valid_(false)
            {
                Construct(std::move(other.Get()));
                other.valid_ = false;
            }

            Optional& operator=(Optional&& other) HADESMEM_DETAIL_NOEXCEPT
            {
                Destroy();
                Construct(std::move(other.Get()));
                other.valid_ = false;

                return *this;
            }

            Optional& operator=(T const& t)
            {
                Destroy();
                Construct(t);

                return *this;
            }

            Optional& operator=(T&& t) HADESMEM_DETAIL_NOEXCEPT
            {
                Destroy();
                Construct(std::move(t));

                return *this;
            }

            ~Optional() HADESMEM_DETAIL_NOEXCEPT
            {
                Destroy();
            }

            // TODO: Emplacement support. (Including default construction of T.)

#if defined(HADESMEM_DETAIL_NO_EXPLICIT_CONVERSION_OPERATOR)
            operator Boolean() const HADESMEM_DETAIL_NOEXCEPT
            {
                return valid_ ? &Optional::NotComparable : nullptr;
            }
#else // #if defined(HADESMEM_DETAIL_NO_EXPLICIT_CONVERSION_OPERATOR)
            explicit operator bool() const HADESMEM_DETAIL_NOEXCEPT
            {
                return valid_;
            }
#endif // #if defined(HADESMEM_DETAIL_NO_EXPLICIT_CONVERSION_OPERATOR)

            T& Get() HADESMEM_DETAIL_NOEXCEPT
            {
                return *GetPtr();
            }

            T const& Get() const HADESMEM_DETAIL_NOEXCEPT
            {
                return *GetPtr();
            }

            T& operator*() HADESMEM_DETAIL_NOEXCEPT
            {
                return Get();
            }

            T const& operator*() const HADESMEM_DETAIL_NOEXCEPT
            {
                return Get();
            }

            T* GetPtr() HADESMEM_DETAIL_NOEXCEPT
            {
                return static_cast<T*>(static_cast<void*>(&t_));
            }

            T const* GetPtr() const HADESMEM_DETAIL_NOEXCEPT
            {
                return static_cast<T const*>(static_cast<void const*>(&t_));
            }

            T* operator->() HADESMEM_DETAIL_NOEXCEPT
            {
                return GetPtr();
            }

            T const* operator->() const HADESMEM_DETAIL_NOEXCEPT
            {
                return GetPtr();
            }

        private:
            void NotComparable() const HADESMEM_DETAIL_NOEXCEPT
            {}

            template <typename U>
            void Construct(U&& u)
            {
                if (valid_)
                {
                    Destroy();
                }

                new (&t_) T(std::forward<U>(u));
                valid_ = true;
            }

            void Destroy() HADESMEM_DETAIL_NOEXCEPT
            {
                if (valid_)
                {
                    GetPtr()->~T();
                    valid_ = false;
                }
            }

            typename std::aligned_storage<sizeof(T),
                std::alignment_of<T>::value>::type t_;
            bool valid_;
        };

        // TODO: Add conditional noexcept to operator overloads.

        template <typename T>
        inline bool operator==(Optional<T> const& lhs, Optional<T> const& rhs)
        {
            return (!lhs && !rhs) || ((rhs && lhs) && *rhs == *lhs);
        }

        template <typename T>
        inline bool operator!=(Optional<T> const& lhs, Optional<T> const& rhs)
        {
            return !(lhs == rhs);
        }

        template <typename T>
        inline bool operator<(Optional<T> const& lhs, Optional<T> const& rhs)
        {
            return (!lhs && !rhs) || ((rhs && lhs) && *rhs < *lhs);
        }

    }

}
