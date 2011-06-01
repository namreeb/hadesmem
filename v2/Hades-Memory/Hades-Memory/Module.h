/*
This file is part of HadesMem.
Copyright © 2010 RaptorFactor (aka Cypherjb, Cypher, Chazwazza). 
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

// Windows API
#include <tchar.h>
#include <Windows.h>
#include <TlHelp32.h>

// C++ Standard Library
#include <string>
#include <memory>

// Boost
#pragma warning(push, 1)
#pragma warning (disable: ALL_CODE_ANALYSIS_WARNINGS)
#include <boost/optional.hpp>
#include <boost/iterator/iterator_facade.hpp>
#pragma warning(pop)

// Hades
#include "Fwd.h"
#include "Error.h"
#include "MemoryMgr.h"
#include "Hades-Common/EnsureCleanup.h"

namespace Hades
{
  namespace Memory
  {
    // Module managing class
    class Module
    {
    public:
      // Module exception type
      class Error : public virtual HadesMemError
      { };

      // Create module
      Module(MemoryMgr const& MyMemory, MODULEENTRY32 const& ModuleEntry);

      // Find module by handle
      Module(MemoryMgr const& MyMemory, HMODULE Handle);

      // Find module by name
      Module(MemoryMgr const& MyMemory, 
        std::basic_string<TCHAR> const& ModuleName);

      // Get module base
      HMODULE GetBase() const;
      // Get module size
      DWORD GetSize() const;

      // Get module name
      std::basic_string<TCHAR> GetName() const;
      // Get module path
      std::basic_string<TCHAR> GetPath() const;

    private:
      // Memory instance
      MemoryMgr m_Memory;

      // Module base address
      HMODULE m_Base;
      // Module size
      DWORD m_Size;
      // Module name
      std::basic_string<TCHAR> m_Name;
      // Module path
      std::basic_string<TCHAR> m_Path;
    };

    // Module iterator
    class ModuleListIter : public boost::iterator_facade<ModuleListIter, 
      boost::optional<Module>, boost::incrementable_traversal_tag>, 
      private boost::noncopyable
    {
    public:
      // Constructor
      ModuleListIter(MemoryMgr const& MyMemory) 
        : m_Memory(MyMemory), 
        m_Snap(), 
        m_Current()
      {
        // Grab a new snapshot of the process
        m_Snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, m_Memory.
          GetProcessID());
        if (m_Snap == INVALID_HANDLE_VALUE)
        {
          DWORD const LastError = GetLastError();
          BOOST_THROW_EXCEPTION(Module::Error() << 
            ErrorFunction("ModuleEnum::First") << 
            ErrorString("Could not get module snapshot.") << 
            ErrorCodeWin(LastError));
        }

        // Get first module entry
        MODULEENTRY32 MyModuleEntry = { sizeof(MyModuleEntry) };
        if (!Module32First(m_Snap, &MyModuleEntry))
        {
          DWORD const LastError = GetLastError();
          BOOST_THROW_EXCEPTION(Module::Error() << 
            ErrorFunction("ModuleEnum::First") << 
            ErrorString("Could not get module info.") << 
            ErrorCodeWin(LastError));
        }

        m_Current = Module(m_Memory, MyModuleEntry);
      }

    private:
      // Allow Boost.Iterator access to internals
      friend class boost::iterator_core_access;

      // For Boost.Iterator
      void increment() 
      {

        MODULEENTRY32 MyModuleEntry = { sizeof(MyModuleEntry) };
        if (!Module32Next(m_Snap, &MyModuleEntry))
        {
          if (GetLastError() != ERROR_NO_MORE_FILES)
          {
            DWORD const LastError = GetLastError();
            BOOST_THROW_EXCEPTION(Module::Error() << 
              ErrorFunction("ModuleEnum::Next") << 
              ErrorString("Error enumerating module list.") << 
              ErrorCodeWin(LastError));
          }

          m_Current = boost::optional<Module>();
        }
        else
        {
          m_Current = Module(m_Memory, MyModuleEntry);
        }
      }

      // For Boost.Iterator
      boost::optional<Module>& dereference() const
      {
        return m_Current;
      }

      // Memory instance
      MemoryMgr m_Memory;

      // Toolhelp32 snapshot handle
      Windows::EnsureCloseSnap m_Snap;

      // Current module
      mutable boost::optional<Module> m_Current;
    };
  }
}
