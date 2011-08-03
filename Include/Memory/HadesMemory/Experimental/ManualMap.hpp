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

// Hades
#include <HadesMemory/MemoryMgr.hpp>
#include <HadesMemory/Detail/Error.hpp>
#include <HadesMemory/PeLib/PeFile.hpp>
#include <HadesMemory/Detail/Config.hpp>

// C++ Standard Library
#include <string>

// Boost
#include <boost/filesystem.hpp>

// Windows API
#include <Windows.h>

namespace HadesMem
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
    
    // Manual mapping flags
    enum InjectFlags
    {
      InjectFlag_None
    };

    // Manually map DLL
    PVOID InjectDll(boost::filesystem::path const& Path, 
      std::string const& Export, 
      InjectFlags Flags = InjectFlag_None) const;

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
