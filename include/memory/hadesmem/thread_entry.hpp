// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>
#include <tlhelp32.h>

#include <hadesmem/config.hpp>

namespace hadesmem
{

    class ThreadEntry
    {
    public:
        HADESMEM_DETAIL_CONSTEXPR explicit ThreadEntry(
            THREADENTRY32 const& entry) HADESMEM_DETAIL_NOEXCEPT
            : usage_(entry.cntUsage),
            thread_id_(entry.th32ThreadID),
            owner_process_id_(entry.th32OwnerProcessID),
            base_priority_(entry.tpBasePri),
            delta_priority_(entry.tpDeltaPri),
            flags_(entry.dwFlags)
        { }

        HADESMEM_DETAIL_CONSTEXPR DWORD GetUsage() const 
            HADESMEM_DETAIL_NOEXCEPT
        {
            return usage_;
        }

        HADESMEM_DETAIL_CONSTEXPR DWORD GetId() const HADESMEM_DETAIL_NOEXCEPT
        {
            return thread_id_;
        }

        HADESMEM_DETAIL_CONSTEXPR DWORD GetOwnerId() const 
            HADESMEM_DETAIL_NOEXCEPT
        {
            return owner_process_id_;
        }

        HADESMEM_DETAIL_CONSTEXPR LONG GetBasePriority() const
            HADESMEM_DETAIL_NOEXCEPT
        {
            return base_priority_;
        }

        HADESMEM_DETAIL_CONSTEXPR LONG GetDeltaPriority() const
            HADESMEM_DETAIL_NOEXCEPT
        {
            return delta_priority_;
        }

        HADESMEM_DETAIL_CONSTEXPR DWORD GetFlags() const 
            HADESMEM_DETAIL_NOEXCEPT
        {
            return flags_;
        }

    private:
        DWORD usage_;
        DWORD thread_id_;
        DWORD owner_process_id_;
        LONG base_priority_;
        LONG delta_priority_;
        DWORD flags_;
    };

}
