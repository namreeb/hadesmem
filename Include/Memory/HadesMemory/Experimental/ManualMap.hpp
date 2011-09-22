// Copyright Joshua Boyce 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

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
