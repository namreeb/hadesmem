/*
This file is part of HadesMem.
Copyright © 2010 Cypherjb (aka Chazwazza, aka Cypher). 
<http://www.cypherjb.com/> <cypher.jb@gmail.com>

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

// C++ Standard Library
#include <iostream>

// Boost C++ Libraries
#pragma warning(push, 1)
#include <boost/thread.hpp>
#pragma warning(pop)

// Hades
#include "Speeder.h"

namespace Hades
{
  namespace HXGenHack
  {
    // Speed multiplier
    DWORD Speeder::m_Multiplier = 1;

    // QueryPerformanceCounter hook
    std::shared_ptr<Hades::Memory::PatchDetour> Speeder::
      m_pQueryPerformanceCounterHk;

    // GetTickCount hook
    std::shared_ptr<Hades::Memory::PatchDetour> Speeder::m_pGetTickCountHk;

    // Constructor
    void Speeder::Startup(Kernel::Kernel* pKernel)
    {
      // Load Kernel32
      // Todo: Defer hooking until game loads Kernel32 (e.g. via LoadLibrary 
      // hook).
      HMODULE Kernel32Mod = LoadLibrary(L"Kernel32.dll");
      if (!Kernel32Mod)
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(SpeederError() << 
          ErrorFunction("Speeder::Startup") << 
          ErrorString("Could not load Kernel32.") << 
          ErrorCodeWin(LastError));
      }

      // Hook if required
      if (!m_pGetTickCountHk && pKernel->IsHookEnabled(
        L"kernel32.dll!GetTickCount"))
      {
        // Get address of GetTickCount
        FARPROC pGetTickCount = GetProcAddress(Kernel32Mod, "GetTickCount");
        if (!pGetTickCount)
        {
          DWORD LastError = GetLastError();
          BOOST_THROW_EXCEPTION(SpeederError() << 
            ErrorFunction("Speeder::Startup") << 
            ErrorString("Could not get address of GetTickCount.") << 
            ErrorCodeWin(LastError));
        }

        // Target and detour pointer
        PBYTE Target = reinterpret_cast<PBYTE>(pGetTickCount);
        PBYTE Detour = reinterpret_cast<PBYTE>(&GetTickCount_Hook);

        // Debug output
        std::wcout << "Speeder::Startup: Hooking kernel32.dll!GetTickCount." 
          << std::endl;
        std::wcout << boost::wformat(L"Speeder::Startup: Target = %p, "
          L"Detour = %p.") %Target %Detour << std::endl;

        // Hook kernel32.dll!GetTickCount
        m_pGetTickCountHk.reset(new Hades::Memory::PatchDetour(
          *pKernel->GetMemoryMgr(), Target, Detour));
        m_pGetTickCountHk->Apply();
      }

      // Hook if required
      if (!m_pQueryPerformanceCounterHk && pKernel->IsHookEnabled(
        L"kernel32.dll!QueryPerformanceCounter"))
      {
        // Get address of QueryPerformanceCounter
        FARPROC pQueryPerformanceCounter = GetProcAddress(Kernel32Mod, 
          "QueryPerformanceCounter");
        if (!pQueryPerformanceCounter)
        {
          DWORD LastError = GetLastError();
          BOOST_THROW_EXCEPTION(SpeederError() << 
            ErrorFunction("Speeder::Startup") << 
            ErrorString("Could not get address of QueryPerformanceCounter.") << 
            ErrorCodeWin(LastError));
        }

        // Target and detour pointer
        PBYTE Target = reinterpret_cast<PBYTE>(pQueryPerformanceCounter);
        PBYTE Detour = reinterpret_cast<PBYTE>(&QueryPerformanceCounter_Hook);

        // Debug output
        std::wcout << "Speeder::Startup: Hooking kernel32.dll!"
          "QueryPerformanceCounter." << std::endl;
        std::wcout << boost::wformat(L"Speeder::Startup: Target = %p, "
          L"Detour = %p.") %Target %Detour << std::endl;

        // Hook kernel32.dll!QueryPerformanceCounter
        m_pQueryPerformanceCounterHk.reset(new Hades::Memory::PatchDetour(
          *pKernel->GetMemoryMgr(), Target, Detour));
        m_pQueryPerformanceCounterHk->Apply();
      }

      // Expose API
      luabind::module(pKernel->GetLuaMgr().GetState(), "HXGenHack")
      [
        luabind::def("GetSpeed", &Speeder::GetSpeed)
        ,luabind::def("SetSpeed", &Speeder::SetSpeed)
      ];

      // Debug output
      std::wcout << "Speeder::Startup: Speeder initialized." << std::endl;
    }

    // Hook function for QueryPerformanceCounter
    BOOL WINAPI Speeder::QueryPerformanceCounter_Hook(
      LARGE_INTEGER* lpPerformanceCount)
    {
      // Call original func
      typedef BOOL (WINAPI* tQueryPerformanceCounter)(
        LARGE_INTEGER* lpPerformanceCount);
      auto pQueryPerformanceCounter = 
        reinterpret_cast<tQueryPerformanceCounter>(
        m_pQueryPerformanceCounterHk->GetTrampoline());
      BOOL Result = pQueryPerformanceCounter(lpPerformanceCount);

      // Sanity check
      if (!lpPerformanceCount)
      {
        return Result;
      }

      // Use TLS for vales from last call
      static boost::thread_specific_ptr<LONGLONG> LastFake;
      static boost::thread_specific_ptr<LONGLONG> LastReal;
      if (!LastFake.get())
      {
        LastFake.reset(new LONGLONG(0));
      }
      if (!LastReal.get())
      {
        LastReal.reset(new LONGLONG(0));
      }

      // Initialize timing variables if they are not already
      if(*LastFake == 0 || *LastReal == 0)
      {
        *LastFake = lpPerformanceCount->QuadPart;
        *LastReal = lpPerformanceCount->QuadPart;
      }

      // Manipulate the timing value using the multiplier supplied
      LONGLONG NewVal = *LastFake + ((lpPerformanceCount->QuadPart - *LastReal) 
        * GetSpeed());

      // Save old values
      *LastReal = lpPerformanceCount->QuadPart;
      *LastFake = NewVal;

      // Overwrite with new values
      lpPerformanceCount->QuadPart = NewVal;

      // Return
      return Result;
    }

    // GetTickCount hook function
    DWORD WINAPI Speeder::GetTickCount_Hook()
    {
      // Call original func
      typedef DWORD (WINAPI* tGetTickCount)();
      auto pGetTickCount = reinterpret_cast<tGetTickCount>(
        m_pGetTickCountHk->GetTrampoline());
      DWORD Result = pGetTickCount();

      // Use TLS for vales from last call
      static boost::thread_specific_ptr<DWORD> LastFake;
      static boost::thread_specific_ptr<DWORD> LastReal;
      if (!LastFake.get())
      {
        LastFake.reset(new DWORD(0));
      }
      if (!LastReal.get())
      {
        LastReal.reset(new DWORD(0));
      }

      // Initialize timing variables if they are not already
      if(*LastFake == 0 || *LastReal == 0)
      {
        *LastFake = Result;
        *LastReal = Result;
      }

      // Manipulate the timing value using the multiplier supplied
      DWORD NewVal = *LastFake + ((Result - *LastReal) * GetSpeed());

      // Save old values
      *LastReal = Result;
      *LastFake = NewVal;

      // Overwrite with new values
      Result = NewVal;

      // Return
      return Result;
    }

    // Get speed multiplier
    DWORD Speeder::GetSpeed()
    {
      return m_Multiplier;
    }

    // Set speed multiplier
    void Speeder::SetSpeed(DWORD Multiplier)
    {
      m_Multiplier = Multiplier;
    }
  }
}
