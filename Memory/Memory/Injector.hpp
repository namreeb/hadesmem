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

// C++ Standard Library
#include <tuple>
#include <string>
#include <utility>

// Boost
#ifdef _MSC_VER
#pragma warning(push, 1)
#endif // #ifdef _MSC_VER
#include <boost/filesystem.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif // #ifdef _MSC_VER

// Hades
#include "Fwd.hpp"
#include "Error.hpp"
#include "MemoryMgr.hpp"
#include "Common/I18n.hpp"

namespace Hades
{
  namespace Memory
  {
    // DLL injection class
    class Injector
    {
    public:
      // Injector exception type
      class Error : public virtual HadesMemError 
      { };

      // Constructor
      explicit Injector(MemoryMgr const& MyMemory);

      // Inject DLL
      HMODULE InjectDll(boost::filesystem::path const& Path, 
        bool PathResolution = true) const;

      // Call export
      DWORD_PTR CallExport(boost::filesystem::path const& ModulePath, 
        HMODULE ModuleRemote, std::string const& Export) const;

    private:
      // MemoryMgr instance
      MemoryMgr m_Memory;
    };
    
    // Create process (as suspended) and inject DLL
    std::tuple<MemoryMgr, HMODULE, DWORD_PTR> CreateAndInject(
      boost::filesystem::path const& Path, 
      boost::filesystem::path const& WorkDir, 
      std::basic_string<TCHAR> const& Args, 
      std::basic_string<TCHAR> const& Module, 
      std::string const& Export);
  }
}
