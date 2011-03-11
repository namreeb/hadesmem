/*
This file is part of HadesMem.
Copyright (C) 2010 Joshua Boyce (aka RaptorFactor, Cypherjb, Cypher, Chazwazza).
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

// C++ Standard Library
#include <list>
#include <string>
#include <memory>

// Windows API
#include <Windows.h>

// Hades
#include "Error.hpp"

namespace Hades
{
  namespace Memory
  {
    class APIHook 
    {
    public:
      // APIHook exception type
      class Error : public virtual HadesMemError 
      { };

      // Hook a function in all modules
      APIHook(std::wstring const& ModuleName, std::wstring const& FunctionName, 
        PROC pHook);
      
      // Unhook a function from all modules
      ~APIHook();

      // Returns the original address of the hooked function
      operator PROC() 
      { 
        return m_pOrig; 
      }

      // Calls the real GetProcAddress 
      static __declspec(noinline) FARPROC WINAPI GetProcAddressRaw(
        HMODULE hModule, 
        PCSTR lpProcName
      );
      
      // Initialize static hooks
      static void Initialize();

    private:
      // Replaces a symbol's address in a module's import section
      static void WINAPI ReplaceIATEntryInAllMods(
        std::wstring const& ModuleName, PROC pOrig, PROC pHook);

      // Replaces a symbol's address in all modules' import sections
      static void WINAPI ReplaceIATEntryInOneMod(
        std::wstring const& ModuleName, PROC pOrig, PROC pHook, 
        HMODULE CallerMod);

      // Used when a DLL is newly loaded after hooking a function
      static void WINAPI FixupNewlyLoadedModule(
        HMODULE hModule, 
        DWORD dwFlags
      );

      // Used to trap when DLLs are newly loaded
      static HMODULE WINAPI LoadLibraryA(
        PCSTR lpFileName
      );
      static HMODULE WINAPI LoadLibraryW(
        PCWSTR lpFileName
      );
      static HMODULE WINAPI LoadLibraryExA(
        PCSTR lpFileName, 
        HANDLE hFile, 
        DWORD dwFlags
      );
      static HMODULE WINAPI LoadLibraryExW(
        PCWSTR lpFileName, 
        HANDLE hFile, 
        DWORD dwFlags
      );

      // Returns address of replacement function if hooked function is requested
      static FARPROC WINAPI GetProcAddress(
        HMODULE hModule, 
        PCSTR lpProcName
      );

      // Hook list
      static std::list<APIHook*> sm_HookList;

      // Module containing the function
      std::wstring m_ModuleName;
      // Function name in callee
      std::wstring m_FunctionName;
      // Original function address in callee
      PROC m_pOrig;
      // Hook function address
      PROC m_pHook;
      
      // Instantiates hooks on these functions
      static std::shared_ptr<APIHook> sm_LoadLibraryA;
      static std::shared_ptr<APIHook> sm_LoadLibraryW;
      static std::shared_ptr<APIHook> sm_LoadLibraryExA;
      static std::shared_ptr<APIHook> sm_LoadLibraryExW;
      static std::shared_ptr<APIHook> sm_GetProcAddress;
    };
  }
}
