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

// Hades
#include <HadesMemory/Module.hpp>

// Boost
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

namespace Hades
{
  namespace Memory
  {
    // Find module by name
    Module::Module(MemoryMgr const& MyMemory, 
      std::wstring const& ModuleName) 
      : m_Memory(MyMemory), 
      m_Base(nullptr), 
      m_Size(0), 
      m_Name(), 
      m_Path()
    {
      // Grab a new snapshot of the process
      Windows::EnsureCloseSnap const Snap(CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, MyMemory.GetProcessID()));
      if (Snap == INVALID_HANDLE_VALUE)
      {
        DWORD const LastError = GetLastError();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Module::Module") << 
          ErrorString("Could not get module snapshot.") << 
          ErrorCodeWinLast(LastError));
      }

      // Convert module name to lowercase
      std::wstring const ModuleNameLower(boost::to_lower_copy(
        ModuleName));
        
      // Detect paths
      bool IsPath = (ModuleNameLower.find('\\') != std::wstring::npos);

      // Search for module
      MODULEENTRY32 ModEntry;
      ZeroMemory(&ModEntry, sizeof(ModEntry));
      ModEntry.dwSize = sizeof(ModEntry);
      
      // Module searching predicate function
      auto FindModByName = 
        [&] () -> bool
        {
          // Process entire module list
          for (BOOL MoreMods = Module32First(Snap, &ModEntry); MoreMods; 
            MoreMods = Module32Next(Snap, &ModEntry)) 
          {
            // Perform path comparison
            if (IsPath && boost::to_lower_copy(static_cast<std::wstring>(
              ModEntry.szExePath)) == ModuleNameLower)
            {
              return true;
            }
            
            // Perform name comparison
            if (!IsPath && boost::to_lower_copy(static_cast<std::wstring>(
              ModEntry.szModule)) == ModuleNameLower)
            {
              return true;
            }
          }
          
          // Nothing found
          return false;
        };

      // Check module was found
      if (!FindModByName())
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Module::Module") << 
          ErrorString("Could not find module."));
      }

      // Get module data
      m_Base = ModEntry.hModule;
      m_Size = ModEntry.modBaseSize;
      m_Name = ModEntry.szModule;
      m_Path = ModEntry.szExePath;
    }

    // Find module by handle
    Module::Module(MemoryMgr const& MyMemory, HMODULE Handle) 
      : m_Memory(MyMemory), 
      m_Base(nullptr), 
      m_Size(0), 
      m_Name(), 
      m_Path()
    {
      // Grab a new snapshot of the process
      Windows::EnsureCloseSnap const Snap(CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, MyMemory.GetProcessID()));
      if (Snap == INVALID_HANDLE_VALUE)
      {
        DWORD const LastError = GetLastError();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Module::Module") << 
          ErrorString("Could not get module snapshot.") << 
          ErrorCodeWinLast(LastError));
      }

      // Search for module
      MODULEENTRY32 ModEntry;
      ZeroMemory(&ModEntry, sizeof(ModEntry));
      ModEntry.dwSize = sizeof(ModEntry);
      
      // Module searching predicate function
      auto FindModByHandle = 
        [&] () -> bool
        {
          // Process entire module list
          for (BOOL MoreMods = Module32First(Snap, &ModEntry); MoreMods; 
            MoreMods = Module32Next(Snap, &ModEntry)) 
          {
            // Perform handle comparison
            if (ModEntry.hModule == Handle)
            {
              return true;
            }
          }
          
          // Nothing found
          return false;
        };

      // Check module was found
      if (!FindModByHandle())
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Module::Module") << 
          ErrorString("Could not find module."));
      }

      // Get module data
      m_Base = ModEntry.hModule;
      m_Size = ModEntry.modBaseSize;
      m_Name = ModEntry.szModule;
      m_Path = ModEntry.szExePath;
    }

    Module::Module(MemoryMgr const& MyMemory, MODULEENTRY32 const& ModuleEntry) 
      : m_Memory(MyMemory), 
      m_Base(ModuleEntry.hModule), 
      m_Size(ModuleEntry.modBaseSize), 
      m_Name(ModuleEntry.szModule), 
      m_Path(ModuleEntry.szExePath)
    { }

    // Get module base address
    HMODULE Module::GetBase() const
    {
      return m_Base;
    }

    // Get module size
    DWORD Module::GetSize() const
    {
      return m_Size;
    }

    // Get module name
    std::wstring Module::GetName() const
    {
      return m_Name;
    }

    // Get module path
    boost::filesystem::path Module::GetPath() const
    {
      return m_Path;
    }
    
    // Get remote module handle
    HMODULE GetRemoteModuleHandle(MemoryMgr const& MyMemory, 
      LPCWSTR ModuleName)
    {
      // Get module list
      ModuleList Modules(MyMemory);
      
      // If pointer to module name is null, return a handle to the file used 
      // to create the calling process. (i.e. The first module in the list)
      if (!ModuleName)
      {
        return Modules.begin()->GetBase();
      }
      
      // Pointer is non-null, so convert to lowercase C++ string
      std::wstring ModuleNameReal(ModuleName);
      boost::to_lower(ModuleNameReal);
      
      // Find location of file extension
      auto const PeriodPos = ModuleNameReal.find(L'.');
      // If no extension is found, assume a DLL is being requested
      if (PeriodPos == std::wstring::npos)
      {
        ModuleNameReal += L".dll";
      }
      // If there is an 'empty' extension (i.e. a trailing period), this 
      // indicates no extension (and '.dll' should not be appended). Remove 
      // the trailing peroid so the string can be used for name/path 
      // comparisons.
      else if (PeriodPos == ModuleNameReal.size() - 1)
      {
        ModuleNameReal.erase(ModuleNameReal.size() - 1);
      }
      
      // Detect paths
      bool const IsPath = (ModuleNameReal.find(L'\\') != std::wstring::npos);
      
      // Find target module
      auto Iter = std::find_if(Modules.begin(), Modules.end(), 
        [&] (Module const& M) -> bool
        {
          if (IsPath)
          {
            return boost::filesystem::equivalent(ModuleNameReal, M.GetPath());
          }
          else
          {
            return ModuleNameReal == boost::to_lower_copy(M.GetName());
          }
        });
      // Return module base if target found
      if (Iter != Modules.end())
      {
        return Iter->GetBase();
      }
      
      // Return null if target not found
      return nullptr;
    }
  }
}
