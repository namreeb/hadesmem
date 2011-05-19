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

// C++ Standard Library
#include <string>

// Boost
#include <boost/filesystem.hpp>
#include <boost/noncopyable.hpp>

// Windows API
#include <Windows.h>
#define DBGHELP_TRANSLATE_TCHAR
#include <DbgHelp.h>

// Hades
#include "HadesMemory/Fwd.hpp"
#include "HadesMemory/Error.hpp"
#include "HadesMemory/MemoryMgr.hpp"
#include "HadesCommon/EnsureCleanup.hpp"

// Note: Symbol APIs provided by DbgHelp are NOT thread-safe. 
// Todo: Implement optional synchronization.

namespace Hades
{
  namespace Memory
  {
    // Symbol handler
    // Note: Thanks to _Mike on MMOwned for the idea and initial PoC base
    class Symbols : private boost::noncopyable
    {
    public:
      // Symbols exception type
      class Error : public virtual HadesMemError 
      { };

      // Constructor
      explicit Symbols(MemoryMgr const& MyMemory, 
        std::wstring const& SearchPath = std::wstring());
        
      // Destructor
      ~Symbols();
      
      // Load symbols for module
      void LoadForModule(std::wstring const& ModuleName);
      
      // Get address for symbol
      PVOID GetAddress(std::wstring const& Name);
        
    private:
      // Memory instance
      MemoryMgr m_Memory;
      
      // Module handle
      Windows::EnsureFreeLibrary m_DbgHelpMod;
    };
  }
}
