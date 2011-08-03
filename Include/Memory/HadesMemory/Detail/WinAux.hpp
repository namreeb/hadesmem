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
#include <HadesMemory/Detail/Error.hpp>
#include <HadesMemory/Detail/Config.hpp>
#include <HadesMemory/Detail/StringBuffer.hpp>

// Boost
#include <boost/filesystem.hpp>

// Windows API
#include <Windows.h>

namespace HadesMem
{
  namespace Detail
  {
    // Get base of self
    inline PVOID GetBaseOfSelf()
    {
      MEMORY_BASIC_INFORMATION MemInfo = { 0, 0, 0, 0, 0, 0, 0 };
      PVOID pGetBaseOfSelf = reinterpret_cast<PVOID>(
        reinterpret_cast<DWORD_PTR>(&GetBaseOfSelf));
      if (!VirtualQuery(pGetBaseOfSelf, &MemInfo, sizeof(MemInfo)))
      {
        DWORD const LastError = GetLastError();
        BOOST_THROW_EXCEPTION(HadesMemError() << 
          ErrorFunction("GetBaseOfSelf") << 
          ErrorString("Failed to query memory.") << 
          ErrorCodeWinLast(LastError));
      }
  
      return MemInfo.AllocationBase;
    }
  
    // Get handle to self
    inline HMODULE GetHandleToSelf()
    {
      return reinterpret_cast<HMODULE>(GetBaseOfSelf());
    }
  
    // Get path to self (directory)
    inline boost::filesystem::path GetSelfPath()
    {
      // Get path to self
      DWORD const SelfPathSize = 32767;
      std::wstring SelfFullPath;
      if (!GetModuleFileName(GetHandleToSelf(), Detail::MakeStringBuffer(
        SelfFullPath, SelfPathSize), SelfPathSize) || GetLastError() == 
        ERROR_INSUFFICIENT_BUFFER)
      {
        DWORD const LastError = GetLastError();
        BOOST_THROW_EXCEPTION(HadesMemError() << 
          ErrorFunction("GetSelfPath") << 
          ErrorString("Could not get path to self.") << 
          ErrorCodeWinLast(LastError));
      }
  
      // Path to self
      return SelfFullPath;
    }
  
    // Get path to self (directory)
    inline boost::filesystem::path GetSelfDirPath()
    {
      // Path to self dir
      return GetSelfPath().parent_path();
    }
  }
}
