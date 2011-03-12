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
#include <vector>
#include <fstream>

// Boost
#pragma warning(push, 1)
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#pragma warning(pop)

// Windows API
#include <Windows.h>

// Hades
#include "Pow.hpp"
#include "Error.hpp"
#include "WinAux.hpp"
#include "StringBuffer.hpp"

namespace Hades
{
  namespace Windows
  {
    // Filesystem error type
    class FilesystemError : public virtual HadesError 
    { };

    // Load a buffer into a file
    inline void BufferToFile(std::vector<BYTE> const& Buffer, 
      boost::filesystem::path const& Path)
    {
      // Open file
      boost::filesystem::basic_ofstream<BYTE> File(Path, std::ios::binary);
      if (!File)
      {
        BOOST_THROW_EXCEPTION(FilesystemError() << 
          ErrorFunction("BufferToFile") << 
          ErrorString("Could not open file."));
      }

      // Copy buffer to file
      File.write(&Buffer[0], Buffer.size());
    }

    // Load a file into a buffer
    inline std::vector<BYTE> FileToBuffer(boost::filesystem::path const& Path)
    {
      // Open file
      boost::filesystem::basic_ifstream<BYTE> File(Path, std::ios::binary | 
        std::ios::ate);
      if (!File)
      {
        BOOST_THROW_EXCEPTION(FilesystemError() << 
          ErrorFunction("FileToBuffer") << 
          ErrorString("Could not open file."));
      }

      // Copy file to buffer
      std::vector<BYTE> Buffer(static_cast<std::vector<BYTE>::size_type>(
        File.tellg()));
      File.seekg(0, std::ios::beg);
      File.read(&Buffer[0], Buffer.size());

      // Return file as buffer
      return Buffer;
    }

    // Get path to module
    inline boost::filesystem::path GetModulePath(HMODULE Module)
    {
      // Get path to self
      DWORD const PathSize = 32767;
      std::wstring FullPath;
      if (!GetModuleFileName(Module, Util::MakeStringBuffer(FullPath, 
        PathSize), PathSize) || GetLastError() == ERROR_INSUFFICIENT_BUFFER)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(HadesError() << 
          ErrorFunction("GetModulePath") << 
          ErrorString("Could not get path to module.") << 
          ErrorCode(LastError));
      }

      // Path to self
      return FullPath;
    }

    // Get path to self (directory)
    inline boost::filesystem::path GetSelfPath()
    {
      // Get path to self
      DWORD const SelfPathSize = 32767;
      std::wstring SelfFullPath;
      if (!GetModuleFileName(GetHandleToSelf(), Util::MakeStringBuffer(
        SelfFullPath, SelfPathSize), SelfPathSize) || GetLastError() == 
        ERROR_INSUFFICIENT_BUFFER)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(HadesError() << 
          ErrorFunction("GetSelfPath") << 
          ErrorString("Could not get path to self.") << 
          ErrorCode(LastError));
      }

      // Path to self
      return SelfFullPath;
    }

    // Get path to self (directory)
    inline boost::filesystem::path GetSelfDirPath()
    {
      // Path to self dir
      return GetSelfPath().parent_path();
    }
  }
}
