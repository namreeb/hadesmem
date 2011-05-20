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

// Hades
#include <HadesMemory/Scanner.hpp>

namespace Hades
{
  namespace Memory
  {
    // Constructor
    Scanner::Scanner(MemoryMgr const& MyMemory, PVOID Start, PVOID End) 
      : m_Memory(MyMemory), 
      m_Start(static_cast<PBYTE>(Start)), 
      m_End(static_cast<PBYTE>(End))
    {
      // Ensure range is valid
      if (m_End < m_Start)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Scanner::Scanner") << 
          ErrorString("Start or end address is invalid."));
      }
    }
  }
}
