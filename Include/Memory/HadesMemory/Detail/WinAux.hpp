// Copyright Joshua Boyce 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

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
