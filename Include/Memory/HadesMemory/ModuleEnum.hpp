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

// Boost
#pragma warning(push, 1)
#include <boost/optional.hpp>
#include <boost/iterator/iterator_facade.hpp>
#pragma warning(pop)

// Windows API
#include <Windows.h>
#include <TlHelp32.h>

// Hades
#include "Fwd.hpp"
#include "Error.hpp"
#include "Module.hpp"
#include "MemoryMgr.hpp"
#include "HadesCommon/EnsureCleanup.hpp"

namespace Hades
{
  namespace Memory
  {
    // Module iterator
    class ModuleIter : public boost::iterator_facade<ModuleIter, 
      boost::optional<Module>, boost::incrementable_traversal_tag>, 
      private boost::noncopyable
    {
    public:
      // Constructor
      explicit ModuleIter(MemoryMgr const& MyMemory) 
        : m_Memory(MyMemory), 
        m_Snap(), 
        m_Current()
      {
        // Grab a new snapshot of the process
        m_Snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | 
          TH32CS_SNAPMODULE32, m_Memory.GetProcessID());
        if (m_Snap == INVALID_HANDLE_VALUE)
        {
          std::error_code const LastError = GetLastErrorCode();
          BOOST_THROW_EXCEPTION(Module::Error() << 
            ErrorFunction("ModuleEnum::First") << 
            ErrorString("Could not get module snapshot.") << 
            ErrorCode(LastError));
        }

        // Get first module entry
        MODULEENTRY32 MyModuleEntry = { sizeof(MyModuleEntry) };
        if (!Module32First(m_Snap, &MyModuleEntry))
        {
          std::error_code const LastError = GetLastErrorCode();
          BOOST_THROW_EXCEPTION(Module::Error() << 
            ErrorFunction("ModuleEnum::First") << 
            ErrorString("Could not get module info.") << 
            ErrorCode(LastError));
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
            std::error_code const LastError = GetLastErrorCode();
            BOOST_THROW_EXCEPTION(Module::Error() << 
              ErrorFunction("ModuleEnum::Next") << 
              ErrorString("Error enumerating module list.") << 
              ErrorCode(LastError));
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
