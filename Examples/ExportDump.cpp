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

// C++ Standard Library
#include <iostream>
#include <algorithm>

// Boost
#include <boost/format.hpp>

// Hades
#include "HadesMemory/Memory.hpp"

int main()
{
  // Open memory manager for self
  Hades::Memory::MemoryMgr MyMemory(GetCurrentProcessId());
  
  // Enumerate module list
  Hades::Memory::ModuleList Modules(MyMemory);
  std::for_each(Modules.begin(), Modules.end(), 
    [&] (Hades::Memory::Module const& Mod) 
    {
      // Create PE file object for current module
      Hades::Memory::PeFile MyPeFile(MyMemory, Mod.GetBase());
        
      // Enumerate export list for current module
      Hades::Memory::ExportList Exports(MyPeFile);
      std::for_each(Exports.begin(), Exports.end(), 
        [&] (Hades::Memory::Export& E)
        {
          // Dump name info if available
          if (E.ByName())
          {
            std::wcout << Mod.GetName() << "!";
            std::cout << E.GetName();
          }
          // Otherwise dump ordinal info
          else
          {
            std::wcout << boost::wformat(L"%s!%d (%x)") %Mod.GetName() 
              %E.GetOrdinal() %E.GetOrdinal();
          }
          
          // Dump forwarder info if available
          if (E.Forwarded())
          {
            std::cout << boost::format(" -> %s\n") %E.GetForwarder();
          }
          // Otherwise dump RVA and VA info
          else
          {
            std::cout << boost::format(" -> %x (%p)\n") %E.GetRva() 
              %E.GetVa();
          }
        });
    });
}
