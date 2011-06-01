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

// Windows API
#include <Windows.h>
#include <TlHelp32.h>

// C++ Standard Library
#include <string>
#include <vector>
#include <iostream>

// Boost
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

// Hades
#include "Memory.h"

namespace Hades
{
  namespace Memory
  {
    // Module exception type
    class ModuleError : public virtual HadesMemError
    { };

    // Get module list
    inline std::vector<boost::shared_ptr<class Module>> GetModuleList(
      MemoryMgr const& MyMemory);

    // Module wide stream overload
    inline std::wostream& operator<< (std::wostream& Out, 
    class Module const& In);

    // Module managing class
    class Module
    {
    public:
      // Find module by handle
      inline Module(MemoryMgr const& MyMemory, HMODULE Handle);
      // Find module by name
      inline Module(MemoryMgr const& MyMemory, std::wstring const& ModuleName);

      // Get module base
      inline HMODULE GetBase() const;
      // Get module size
      inline DWORD GetSize() const;

      // Get module name
      inline std::wstring GetName() const;
      // Get module path
      inline std::wstring GetPath() const;

      // Whether module was found
      inline bool Found() const;

    private:
      // Disable assignment
      Module& operator= (Module const&);

      // Memory instance
      MemoryMgr const& m_Memory;

      // Module base address
      HMODULE m_Base;
      // Module size
      DWORD m_Size;
      // Module name
      std::wstring m_Name;
      // Module path
      std::wstring m_Path;

      // Whether module was found
      bool m_Found;
    };

    // Get module list
    inline std::vector<boost::shared_ptr<class Module>> GetModuleList(
      MemoryMgr const& MyMemory) 
    {
      // Grab a new snapshot of the process
      Windows::EnsureCloseHandle const Snap(CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE, MyMemory.GetProcessID()));
      if (Snap == INVALID_HANDLE_VALUE)
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(ModuleError() << 
          ErrorFunction("GetModuleList") << 
          ErrorString("Could not get module snapshot.") << 
          ErrorCodeWin(LastError));
      }

      // Container to hold module list
      std::vector<boost::shared_ptr<class Module>> ModList;

      // Get all modules
      MODULEENTRY32 ModEntry = { sizeof(ModEntry) };
      for (BOOL MoreMods = Module32First(Snap, &ModEntry); MoreMods; 
        MoreMods = Module32Next(Snap, &ModEntry)) 
      {
        ModList.push_back(boost::make_shared<Module>(MyMemory, 
          ModEntry.hModule));
      }

      // Return module list
      return ModList;
    }

    // Module wide stream overload
    inline std::wostream& operator<< (std::wostream& Stream, 
      Module const& MyModule)
    {
      Stream << "Module Base: " << MyModule.GetBase() << "." << std::endl;
      Stream << "Module Size: " << MyModule.GetSize() << "." << std::endl;
      Stream << "Module Name: " << MyModule.GetName() << "." << std::endl;
      Stream << "Module Path: " << MyModule.GetPath() << "." << std::endl;
      return Stream;
    }

    // Find module by name
    Module::Module(MemoryMgr const& MyMemory, std::wstring const& ModuleName) 
      : m_Memory(MyMemory), 
      m_Base(nullptr), 
      m_Size(0), 
      m_Name(), 
      m_Path(), 
      m_Found(false) 
    {
      // Grab a new snapshot of the process
      Windows::EnsureCloseHandle const Snap(CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE, MyMemory.GetProcessID()));
      if (Snap == INVALID_HANDLE_VALUE)
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(ModuleError() << 
          ErrorFunction("Module::Module") << 
          ErrorString("Could not get module snapshot.") << 
          ErrorCodeWin(LastError));
      }

      // Convert module name to lowercase
      std::wstring const ModuleNameLower(boost::to_lower_copy(ModuleName));

      // Search for process
      MODULEENTRY32 ModEntry = { sizeof(ModEntry) };
      bool Found = false;
      for (BOOL MoreMods = Module32First(Snap, &ModEntry); MoreMods; 
        MoreMods = Module32Next(Snap, &ModEntry)) 
      {
        Found = (boost::to_lower_copy(std::wstring(ModEntry.szModule)) == 
          ModuleNameLower) || (boost::to_lower_copy(std::wstring(
          ModEntry.szExePath)) == ModuleNameLower);
        if (Found)
        {
          break;
        }
      }

      // Check if module was found
      if (Found)
      {
        m_Base = ModEntry.hModule;
        m_Size = ModEntry.modBaseSize;
        m_Name = ModEntry.szModule;
        m_Path = ModEntry.szExePath;
      }

      // Whether module was found
      m_Found = Found;
    }

    // Find module by handle
    Module::Module(MemoryMgr const& MyMemory, HMODULE Handle) 
      : m_Memory(MyMemory), 
      m_Base(nullptr), 
      m_Size(0), 
      m_Name(), 
      m_Path(), 
      m_Found(false)
    {
      // Grab a new snapshot of the process
      Windows::EnsureCloseHandle const Snap(CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE, MyMemory.GetProcessID()));
      if (Snap == INVALID_HANDLE_VALUE)
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(ModuleError() << 
          ErrorFunction("Module::Module") << 
          ErrorString("Could not get module snapshot.") << 
          ErrorCodeWin(LastError));
      }

      // Search for process
      MODULEENTRY32 ModEntry = { sizeof(ModEntry) };
      bool Found = false;
      for (BOOL MoreMods = Module32First(Snap, &ModEntry); MoreMods; 
        MoreMods = Module32Next(Snap, &ModEntry)) 
      {
        Found = (ModEntry.hModule == Handle);
        if (Found)
        {
          break;
        }
      }

      // Check process was found
      if (!Found)
      {
        BOOST_THROW_EXCEPTION(ModuleError() << 
          ErrorFunction("Module::Module") << 
          ErrorString("Could not find module."));
      }

      // Get module data
      m_Base = ModEntry.hModule;
      m_Size = ModEntry.modBaseSize;
      m_Name = ModEntry.szModule;
      m_Path = ModEntry.szExePath;
    }

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
    std::wstring Module::GetPath() const
    {
      return m_Path;
    }

    // Whether module was found
    bool Module::Found() const
    {
      return m_Found;
    }
  }
}
