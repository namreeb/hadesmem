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
        
      // Enumerate import dir list for current module
      Hades::Memory::ImportDirList ImpDirs(MyPeFile);
      std::for_each(ImpDirs.begin(), ImpDirs.end(), 
        [&] (Hades::Memory::ImportDir& I)
        {
          // Enumerate import thunk list for current module
          Hades::Memory::ImportThunkList ImpThunks(MyPeFile, 
            I.GetCharacteristics());
          std::for_each(ImpThunks.begin(), ImpThunks.end(), 
            [&] (Hades::Memory::ImportThunk& T)
            {
              // Dump module name and import name/ordinal
              if (T.ByOrdinal())
              {
                std::cout << boost::format("%s!%d\n") %I.GetName() 
                  %T.GetOrdinal();
              }
              else
              {
                std::cout << boost::format("%s!%s\n") %I.GetName() 
                  %T.GetName();
              }
            });
        });
    });
}
