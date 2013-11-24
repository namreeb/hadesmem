// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstddef>

#include <windows.h>

#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/protect_guard.hpp>
#include <hadesmem/detail/query_region.hpp>
#include <hadesmem/detail/type_traits.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/protect.hpp>

namespace hadesmem
{

    namespace detail
    {

        inline void ReadUnchecked(
            Process const& process,
            LPVOID address,
            LPVOID data,
            std::size_t len)
        {
            HADESMEM_DETAIL_ASSERT(address != nullptr);
            HADESMEM_DETAIL_ASSERT(data != nullptr);
            HADESMEM_DETAIL_ASSERT(len != 0);

            SIZE_T bytes_read = 0;
            if (!::ReadProcessMemory(
                process.GetHandle(), 
                address, 
                data, 
                len,
                &bytes_read) || bytes_read != len)
            {
                DWORD const last_error = ::GetLastError();
                HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                    ErrorString("ReadProcessMemory failed.") <<
                    ErrorCodeWinLast(last_error));
            }
        }

        inline void ReadImpl(
            Process const& process,
            LPVOID address,
            LPVOID data,
            std::size_t len)
        {
            HADESMEM_DETAIL_ASSERT(address != nullptr);
            HADESMEM_DETAIL_ASSERT(data != nullptr);
            HADESMEM_DETAIL_ASSERT(len != 0);

            for (;;)
            {
                ProtectGuard protect_guard(
                    process, 
                    address, 
                    ProtectGuardType::kRead);

                MEMORY_BASIC_INFORMATION const mbi = detail::Query(
                    process, 
                    address);
                PVOID const region_next = static_cast<PBYTE>(mbi.BaseAddress) +
                    mbi.RegionSize;

                LPVOID const address_end = static_cast<LPBYTE>(address)+len;
                if (address_end <= region_next)
                {
                    ReadUnchecked(process, address, data, len);

                    protect_guard.Restore();

                    return;
                }
                else
                {
                    std::size_t const len_new = 
                        reinterpret_cast<DWORD_PTR>(region_next)-
                        reinterpret_cast<DWORD_PTR>(address);

                    ReadUnchecked(process, address, data, len_new);

                    protect_guard.Restore();

                    address = static_cast<LPBYTE>(address)+len_new;
                    data = static_cast<LPBYTE>(data)+len_new;
                    len -= len_new;
                }
            }
        }

        template <typename T>
        T ReadImpl(
            Process const& process,
            PVOID address)
        {
            HADESMEM_DETAIL_STATIC_ASSERT(
                detail::IsTriviallyCopyable<T>::value);
            HADESMEM_DETAIL_STATIC_ASSERT(
                detail::IsDefaultConstructible<T>::value);

            HADESMEM_DETAIL_ASSERT(address != nullptr);

            T data;
            ReadImpl(process, address, std::addressof(data), sizeof(data));
            return data;
        }

    }

}
