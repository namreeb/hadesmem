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
#include <Windows.h>
#ifdef UNICODE
#define DBGHELP_TRANSLATE_TCHAR
#endif
#include <DbgHelp.h>

// C++ Standard Library
#include <string>

// Boost
#include <boost/filesystem.hpp>

// Hades
#include "Fwd.h"
#include "Error.h"
#include "MemoryMgr.h"

// Note: Symbol APIs provided by DbgHelp are NOT thread-safe. 
// Todo: Implement optional synchronization.

namespace Hades
{
  namespace Memory
  {
    // Symbol handler
    // Note: Thanks to _Mike on MMOwned for the idea and initial PoC base
    class Symbols
    {
    public:
      // Symbols exception type
      class Error : public virtual HadesMemError 
      { };

      // Constructor
      Symbols(MemoryMgr const& MyMemory, boost::filesystem::path const& SearchPath = boost::filesystem::path());
        
      // Destructor
      ~Symbols();
      
      // Load symbols for module
      void LoadForModule(std::basic_string<TCHAR> const& ModuleName);
      
      // Get address for symbol
      PVOID GetAddress(std::basic_string<TCHAR> const& Name);
        
    private:
      // Memory instance
      MemoryMgr m_Memory;
    };
  }
}
