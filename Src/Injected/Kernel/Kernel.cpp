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

// C++ Standard Library
#include <iostream>

// Boost
#include <boost/format.hpp>

// Hades
#include "Kernel.hpp"
#include "HadesCommon/Filesystem.hpp"
#include "HadesCommon/StringBuffer.hpp"

namespace Hades
{
  namespace Kernel
  {
    Kernel::Kernel() 
    {
      // Get string to binary we're injected into
      DWORD const BinPathSize = MAX_PATH;
      std::wstring BinPath;
      if (!GetModuleFileName(nullptr, Util::MakeStringBuffer(BinPath, 
        BinPathSize), BinPathSize))
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Kernel::Kernel") << 
          ErrorString("Could not get path to current binary.") << 
          ErrorCode(LastError));
      }

      // Debug output
      std::wcout << boost::wformat(L"Kernel::Kernel: Path to current binary = "
        L"\"%ls\".") %BinPath << std::endl;

      // Path to self
      auto const PathToSelf(Windows::GetSelfPath());
      auto const PathToSelfDir(Windows::GetSelfDirPath());

      // Debug output
      std::wcout << boost::wformat(L"Kernel::Kernel: Path to self (Full): = "
        L"\"%ls\", Path To self (Dir): = \"%ls\".") %PathToSelf.native() 
        %PathToSelfDir.native() << std::endl;
        
      // Debug output
      std::wcout << "Kernel::Kernel: Hades-Kernel initialized." << std::endl;
    }
  }
}
