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

#pragma once

// C++ Standard Library
#include <string>
#include <vector>
#include <memory>
#include <utility>

// Windows API
#include <Windows.h>

// Hades
#include "Hades-Memory/Memory.h"
#include "Hades-Memory/Patcher.h"

namespace Hades
{
  namespace Kernel
  {
    // Loader exception type
    class LoaderError : public virtual HadesError 
    { };

    // Loader class. Used to hook games that can only be run via loaders.
    // Todo: Support all loaders via generic method.
    // Todo: Add ShellExecuteEx support.
    class Loader
    {
    public:
      // Initialize loader
      static void Initialize(class Kernel* pKernel);

      // Load configuration data from XML file
      static void LoadConfig(std::wstring const& Path);

      // Initialize settings and hook APIs
      static void AddExe(std::wstring const& ProcessName, 
        std::wstring const& ModuleName);

    private:
      // Whether we should inject into the process
      static std::wstring ShouldInject(std::wstring const& ProcessName);

      // CreateProcessInternalW API hook
      static BOOL WINAPI CreateProcessInternalW_Hook(
        PVOID Unknown1, 
        LPCWSTR lpApplicationName, 
        LPWSTR lpCommandLine, 
        LPSECURITY_ATTRIBUTES lpProcessAttributes, 
        LPSECURITY_ATTRIBUTES lpThreadAttributes, 
        BOOL bInheritHandles, 
        DWORD dwCreationFlags, 
        LPVOID lpEnvironment, 
        LPCWSTR lpCurrentDirectory, 
        LPSTARTUPINFOW lpStartupInfo, 
        LPPROCESS_INFORMATION lpProcessInformation, 
        PVOID Unknown2);

      // Attempt to inject module into target
      static void AttemptInjection(LPPROCESS_INFORMATION ProcInfo, 
        std::wstring const& Module);

      // Memory manager
      static std::shared_ptr<Memory::MemoryMgr> m_Memory;

      // Settings
      typedef std::pair<std::wstring, std::wstring> ProcAndMod;
      typedef std::vector<ProcAndMod> ProcAndModList;
      static ProcAndModList m_ProcsAndMods;

      // CreateProcessInternalW hook
      static std::shared_ptr<Memory::PatchDetour> m_pCreateProcessInternalWHk;
    };
  }
}
