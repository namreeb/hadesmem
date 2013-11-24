// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/smart_handle.hpp>
#include <hadesmem/error.hpp>

namespace hadesmem
{

    namespace detail
    {

        inline SYSTEM_INFO GetSystemInfo()
        {
            SYSTEM_INFO sys_info;
            ::ZeroMemory(&sys_info, sizeof(sys_info));
            ::GetSystemInfo(&sys_info);
            return sys_info;
        }

        inline bool IsWoW64Process(HANDLE handle)
        {
            BOOL is_wow64 = FALSE;
            if (!::IsWow64Process(handle, &is_wow64))
            {
                DWORD const last_error = ::GetLastError();
                HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                    ErrorString("IsWoW64Process failed.") <<
                    ErrorCodeWinLast(last_error));
            }

            return is_wow64 != FALSE;
        }

        inline detail::SmartHandle OpenProcessAllAccess(DWORD id)
        {
            HANDLE const handle = ::OpenProcess(
                PROCESS_ALL_ACCESS, 
                FALSE, 
                id);
            if (!handle)
            {
                DWORD const last_error = ::GetLastError();
                HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                    ErrorString("OpenProcess failed.") <<
                    ErrorCodeWinLast(last_error));
            }

            return detail::SmartHandle(handle);
        }

        inline detail::SmartHandle OpenThreadAllAccess(DWORD id)
        {
            HANDLE const handle = ::OpenThread(
                THREAD_ALL_ACCESS, 
                FALSE, 
                id);
            if (!handle)
            {
                DWORD const last_error = ::GetLastError();
                HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                    ErrorString("OpenThread failed.") <<
                    ErrorCodeWinLast(last_error));
            }

            return detail::SmartHandle(handle);
        }

        inline detail::SmartHandle DuplicateHandle(HANDLE handle)
        {
            HADESMEM_DETAIL_ASSERT(handle != nullptr);

            HANDLE new_handle = nullptr;
            if (!::DuplicateHandle(
                ::GetCurrentProcess(), 
                handle,
                ::GetCurrentProcess(), 
                &new_handle, 
                0, 
                FALSE, 
                DUPLICATE_SAME_ACCESS))
            {
                DWORD const last_error = ::GetLastError();
                HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                    ErrorString("DuplicateHandle failed.") <<
                    ErrorCodeWinLast(last_error));
            }

            return detail::SmartHandle(new_handle);
        }

        inline std::wstring QueryFullProcessImageName(HANDLE handle)
        {
            HADESMEM_DETAIL_ASSERT(handle != nullptr);

            std::vector<wchar_t> path(HADESMEM_DETAIL_MAX_PATH_UNICODE);
            DWORD path_len = static_cast<DWORD>(path.size());
            if (!::QueryFullProcessImageNameW(
                handle, 
                0, 
                path.data(), 
                &path_len))
            {
                DWORD const last_error = ::GetLastError();
                HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                    ErrorString("QueryFullProcessImageName failed.") <<
                    ErrorCodeWinLast(last_error));
            }

            return path.data();
        }

    }

}
