/*
This file is part of HadesMem.
Copyright © 2010 Cypherjb (aka Chazwazza, aka Cypher). 
<http://www.cypherjb.com/> <cypher.jb@gmail.com>

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
#include <vector>
#include <fstream>
#include <iterator>

// Boost
#include <boost/filesystem.hpp>

// Windows API
#include <Windows.h>

// Hades
#include "Error.h"
#include "StringBuffer.h"

// Image base linker 'trick'
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

namespace Hades
{
  namespace Windows
  {
    // Load a file into a buffer
    inline void BufferToFile(std::vector<BYTE> const& Buffer, 
      std::wstring const& Path)
    {
      // Open file
      std::basic_ofstream<BYTE> File(Path.c_str(), std::ios::binary);
      if (!File)
      {
        BOOST_THROW_EXCEPTION(HadesError() << 
          ErrorFunction("FileToBuffer") << 
          ErrorString("Could not open file."));
      }

      // Copy buffer to file
      File.write(&Buffer[0], Buffer.size());
    }

    // Load a file into a buffer
    inline std::vector<BYTE> FileToBuffer(std::wstring const& Path)
    {
      // Open file
      std::basic_ifstream<BYTE> File(Path.c_str(), std::ios::binary | 
        std::ios::ate);
      if (!File)
      {
        BOOST_THROW_EXCEPTION(HadesError() << 
          ErrorFunction("FileToBuffer") << 
          ErrorString("Could not open file."));
      }

      // Copy file to buffer
      std::vector<BYTE> Buffer(static_cast<std::size_t>(File.tellg()));
      File.seekg(0, std::ios::beg);
      File.read(&Buffer[0], Buffer.size());

      // Return file as buffer
      return Buffer;
    }

    // Get path to self (directory)
    inline boost::filesystem::wpath GetSelfDirPath()
    {
      // Get self
      HMODULE const ModMe(reinterpret_cast<HMODULE>(&__ImageBase));

      // Get path to self
      DWORD const SelfPathSize = MAX_PATH;
      std::wstring SelfFullPath;
      if (!GetModuleFileName(ModMe, Util::MakeStringBuffer(SelfFullPath, 
        SelfPathSize), SelfPathSize))
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(HadesError() << 
          ErrorFunction("GetSelfDirPath") << 
          ErrorString("Could not get path to self.") << 
          ErrorCodeWin(LastError));
      }

      // Path to self dir
      auto const SelfDirPath(boost::filesystem::wpath(SelfFullPath).
        parent_path());
      return SelfDirPath;
    }

    // Get path to self (directory)
    inline boost::filesystem::wpath GetSelfPath()
    {
      // Get self
      HMODULE const ModMe(reinterpret_cast<HMODULE>(&__ImageBase));

      // Get path to self
      DWORD const SelfPathSize = MAX_PATH;
      std::wstring SelfFullPath;
      if (!GetModuleFileName(ModMe, Util::MakeStringBuffer(SelfFullPath, 
        SelfPathSize), SelfPathSize))
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(HadesError() << 
          ErrorFunction("GetSelfPath") << 
          ErrorString("Could not get path to self.") << 
          ErrorCodeWin(LastError));
      }

      // Path to self
      return boost::filesystem::wpath(SelfFullPath);
    }
  }
}
