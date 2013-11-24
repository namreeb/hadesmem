// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <string>
#include <vector>

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/detail/winapi.hpp>

namespace hadesmem
{

    inline std::wstring GetPath(Process const& process)
    {
        return detail::QueryFullProcessImageName(process.GetHandle());
    }

    inline bool IsWoW64(Process const& process)
    {
        return detail::IsWoW64Process(process.GetHandle());
    }

}
