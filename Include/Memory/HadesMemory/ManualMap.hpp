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
#include <string>

// Boost
#pragma warning(push, 1)
#include <boost/filesystem.hpp>
#pragma warning(pop)

// Windows API
#include <Windows.h>

// Hades
#include "Fwd.hpp"
#include "Error.hpp"
#include "MemoryMgr.hpp"

namespace Hades
{
  namespace Memory
  {
    // Manual mapping class
    class ManualMap
    {
    public:
      // ManualMap exception type
      class Error : public virtual HadesMemError 
      { };

      // Constructor
      explicit ManualMap(MemoryMgr const& MyMemory);

      // Manually map DLL
      PVOID Map(boost::filesystem::path const& Path, 
        std::string const& Export = "", bool InjectHelper = true) const;

    private:
      // Map sections
      void MapSections(PeFile& MyPeFile, PVOID RemoteAddr) const;

      // Fix imports
      void FixImports(PeFile& MyPeFile) const;

      // Fix relocations
      void FixRelocations(PeFile& MyPeFile, PVOID RemoteAddr) const;

      // MemoryMgr instance
      MemoryMgr m_Memory;
    };
  }
}
