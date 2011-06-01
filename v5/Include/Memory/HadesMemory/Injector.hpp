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
#include <HadesMemory/Fwd.hpp>
#include <HadesCommon/I18n.hpp>
#include <HadesMemory/Error.hpp>
#include <HadesMemory/MemoryMgr.hpp>

// C++ Standard Library
#include <string>
#include <memory>
#include <utility>

// Boost
#include <boost/filesystem.hpp>

// Windows API
#include <Windows.h>

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
      std::pair<DWORD_PTR, DWORD> CallExport(
        boost::filesystem::path const& ModulePath, 
        HMODULE ModuleRemote, std::string const& Export) const;

    private:
      // MemoryMgr instance
      MemoryMgr m_Memory;
    };
    
    // Return data for CreateAndInject
    class CreateAndInjectData
    {
    public:
      CreateAndInjectData(MemoryMgr const& MyMemory, HMODULE Module, 
        DWORD_PTR ExportRet, DWORD ExportLastError) 
        : m_Memory(new MemoryMgr(MyMemory)), 
        m_Module(Module), 
        m_ExportRet(ExportRet), 
        m_ExportLastError(ExportLastError)
      { }
      
      CreateAndInjectData(CreateAndInjectData const& Rhs)
        : m_Memory(new MemoryMgr(*Rhs.m_Memory)), 
        m_Module(Rhs.m_Module), 
        m_ExportRet(Rhs.m_ExportRet), 
        m_ExportLastError(Rhs.m_ExportLastError)
      { }
      
      CreateAndInjectData& operator=(CreateAndInjectData const& Rhs)
      {
        m_Memory.reset(new Hades::Memory::MemoryMgr(*Rhs.m_Memory));
        m_Module = Rhs.m_Module;
        m_ExportRet = Rhs.m_ExportRet;
        m_ExportLastError = Rhs.m_ExportLastError;
        return *this;
      }
      
      MemoryMgr GetMemoryMgr() const
      {
        return *m_Memory;
      }
      
      HMODULE GetModule() const
      {
        return m_Module;
      }
      
      DWORD_PTR GetExportRet() const
      {
        return m_ExportRet;
      }
      
      DWORD GetExportLastError() const
      {
        return m_ExportLastError;
      }
      
    private:
      std::unique_ptr<MemoryMgr> m_Memory;
      HMODULE m_Module;
      DWORD_PTR m_ExportRet;
      DWORD m_ExportLastError;
    };
    
    // Create process (as suspended) and inject DLL
    CreateAndInjectData CreateAndInject(
      boost::filesystem::path const& Path, 
      boost::filesystem::path const& WorkDir, 
      std::wstring const& Args, 
      boost::filesystem::path const& Module, 
      std::string const& Export, 
      bool PathResolution = true);
  }
}
