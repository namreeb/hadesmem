/*
This file is part of HadesMem.
Copyright (C) 2011 Joshua Boyce (a.k.a. RaptorFactor).
<http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

HadesMem is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

HadesMem is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with HadesMem.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

// Hades
#include <HadesCommon/Error.hpp>

// Windows API
#include <Windows.h>

namespace Hades
{
  namespace Windows
  {
    // WinAux error type
    class WinAuxError : public virtual HadesError 
    { };

    // Get base of self
    inline PVOID GetBaseOfSelf()
    {
      MEMORY_BASIC_INFORMATION MemInfo;
      ZeroMemory(&MemInfo, sizeof(MemInfo));
      DWORD_PTR const pSelfAddr = reinterpret_cast<DWORD_PTR>(&GetBaseOfSelf);
      if (!VirtualQuery(reinterpret_cast<LPCVOID>(pSelfAddr), &MemInfo, 
        sizeof(MemInfo)))
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(WinAuxError() << 
          ErrorFunction("GetBaseOfSelf") << 
          ErrorString("Failed to query memory.") << 
          ErrorCode(LastError));
      }

      return MemInfo.AllocationBase;
    }

    // Get handle to self
    inline HMODULE GetHandleToSelf()
    {
      return reinterpret_cast<HMODULE>(GetBaseOfSelf());
    }
  }
}
