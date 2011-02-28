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

// C++ Standard Library
#include <vector>
#include <iterator>
#include <algorithm>

// Boost
#include <boost/algorithm/string.hpp>

// Hades
#include "Module.hpp"
#include "Symbol.hpp"
#include "HadesCommon/I18n.hpp"
#include "HadesCommon/Filesystem.hpp"

namespace Hades
{
  namespace Memory
  {
    // Constructor
    Symbols::Symbols(MemoryMgr const& MyMemory, 
      std::wstring const& SearchPath) 
      : m_Memory(MyMemory), 
      m_DbgHelpMod()
    {
      // Load DbgHelp.dll
      m_DbgHelpMod = LoadLibrary((Windows::GetSelfDirPath() / "dbghelp.dll").
        string<std::wstring>().c_str());
      if (!m_DbgHelpMod)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Symbols::Symbols") << 
          ErrorString("Failed to load DbgHelp.dll.") << 
          ErrorCode(LastError));
      }
      
      // Get address of DbgHelp!SymSetOptions
      typedef DWORD (WINAPI* tSymSetOptions)(DWORD SymOptions);
      auto pSymSetOptions = reinterpret_cast<tSymSetOptions>(GetProcAddress(
        m_DbgHelpMod, "SymSetOptions"));
      if (!pSymSetOptions)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Symbols::Symbols") << 
          ErrorString("Failed to find DbgHelp!SymSetOptions.") << 
          ErrorCode(LastError));
      }
      
      // SYMOPT_DEBUG is not really necessary, but the debug output is always 
      // good if something goes wrong
      pSymSetOptions(SYMOPT_DEBUG | SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME);
      
      // Get address of DbgHelp!SymInitialize
      typedef BOOL (WINAPI* tSymInitialize)(
        HANDLE hProcess, 
        wchar_t const* UserSearchPath, 
        BOOL fInvadeProcess
      );
      auto pSymInitialize = reinterpret_cast<tSymInitialize>(GetProcAddress(
        m_DbgHelpMod, "SymInitialize"));
      if (!pSymInitialize)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Symbols::Symbols") << 
          ErrorString("Failed to find DbgHelp!SymInitialize.") << 
          ErrorCode(LastError));
      }
      
      // Initialize symbol APIs
      if(!pSymInitialize(m_Memory.GetProcessHandle(), SearchPath.empty() ? 
        nullptr : &SearchPath[0], FALSE))
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Symbols::Symbols") << 
          ErrorString("Failed to initialize symbols.") << 
          ErrorCode(LastError));
      }
    }
    
    // Destructor
    Symbols::~Symbols()
    {
      // Get address of DbgHelp!SymCleanup
      typedef BOOL (WINAPI* tSymCleanup)(HANDLE hProcess);
      auto pSymCleanup = reinterpret_cast<tSymCleanup>(GetProcAddress(
        m_DbgHelpMod, "SymCleanup"));
      if (!pSymCleanup)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Symbols::~Symbols") << 
          ErrorString("Failed to find DbgHelp!SymCleanup.") << 
          ErrorCode(LastError));
      }
      
      // Clean up symbol APIs
      if (!pSymCleanup(m_Memory.GetProcessHandle()))
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Symbols::~Symbols") << 
          ErrorString("Failed to clean up symbols.") << 
          ErrorCode(LastError));
      }
    }
  
    // Load symbols for module
    void Symbols::LoadForModule(std::wstring const& ModuleName)
    {
      // Convert module name to lowercase
      auto ModuleNameLower(boost::to_lower_copy(ModuleName));
      
      // Look up module in remote process
      boost::optional<Module> MyModule;
      for (ModuleListIter i(m_Memory); *i; ++i)
      {
        Module& Current = **i;
        if (boost::to_lower_copy(Current.GetName()) == ModuleNameLower || 
          boost::to_lower_copy(Current.GetPath()) == ModuleNameLower)
        {
          MyModule = *i;
          break;
        }
      }
      
      // Ensure module was found
      if (!MyModule)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Symbols::LoadForModule") << 
          ErrorString("Could not find module in remote process."));
      }
      
      // Get address of DbgHelp!SymLoadModuleEx
      typedef DWORD64 (WINAPI* tSymLoadModuleEx)(
        HANDLE hProcess,
        HANDLE hFile,
        wchar_t const* ImageName,
        wchar_t const* ModuleName,
        DWORD64 BaseOfDll,
        DWORD DllSize,
        PMODLOAD_DATA Data,
        DWORD Flags
      );
      auto pSymLoadModuleEx = reinterpret_cast<tSymLoadModuleEx>(
        GetProcAddress(m_DbgHelpMod, "SymLoadModuleEx"));
      if (!pSymLoadModuleEx)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Symbols::LoadForModule") << 
          ErrorString("Failed to find DbgHelp!SymLoadModuleEx.") << 
          ErrorCode(LastError));
      }
      
      // Load symbols for module
      if(!pSymLoadModuleEx(m_Memory.GetProcessHandle(), 
        nullptr, 
        &MyModule->GetName()[0], 
        nullptr, 
        reinterpret_cast<DWORD64>(MyModule->GetBase()), 
        0, 
        nullptr, 
        0))
      {
        std::error_code const LastError = GetLastErrorCode();
        // ERROR_SUCCESS indicates that the symbols were already loaded
        if (LastError.value() != ERROR_SUCCESS)
        {
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("Symbols::LoadForModule") << 
            ErrorString("Failed to load symbols for module.") << 
            ErrorCode(LastError));
        }
      }
      
      // Get address of DbgHelp!SymGetModuleInfo64
      typedef BOOL (WINAPI* tSymGetModuleInfo64)(
        HANDLE hProcess,
        DWORD64 dwAddr,
        PIMAGEHLP_MODULE64 ModuleInfo
      );
      auto pSymGetModuleInfo64 = reinterpret_cast<tSymGetModuleInfo64>(
        GetProcAddress(m_DbgHelpMod, "SymGetModuleInfo64"));
      if (!pSymGetModuleInfo64)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Symbols::LoadForModule") << 
          ErrorString("Failed to find DbgHelp!SymGetModuleInfo64.") << 
          ErrorCode(LastError));
      }
      
      // Get module info
      IMAGEHLP_MODULE64 ModuleInfo = { sizeof(ModuleInfo) };
      if (!pSymGetModuleInfo64(m_Memory.GetProcessHandle(), 
        reinterpret_cast<DWORD64>(MyModule->GetBase()), 
        &ModuleInfo))
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Symbols::LoadForModule") << 
          ErrorString("Failed to get module info.") << 
          ErrorCode(LastError));
      }
    }
  
    // Get address for symbol
    PVOID Symbols::GetAddress(std::wstring const& Name)
    {
      // Construct buffer for symbol API
      std::size_t const BufferSize = (sizeof(SYMBOL_INFO) + MAX_SYM_NAME * 
        sizeof(wchar_t) + sizeof(ULONG64) - 1) / sizeof(ULONG64);
      std::vector<ULONG64> SymInfoBuf(BufferSize);
      PSYMBOL_INFO pSymbol = reinterpret_cast<PSYMBOL_INFO>(&SymInfoBuf[0]);
      pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
      pSymbol->MaxNameLen = MAX_SYM_NAME;
      
      // Get address of DbgHelp!SymFromName
      typedef BOOL (WINAPI* tSymFromName)(
        HANDLE hProcess,
        wchar_t const* Name,
        PSYMBOL_INFO Symbol
      );
      auto pSymFromName = reinterpret_cast<tSymFromName>(
        GetProcAddress(m_DbgHelpMod, "SymFromName"));
      if (!pSymFromName)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Symbols::LoadForModule") << 
          ErrorString("Failed to find DbgHelp!SymFromName.") << 
          ErrorCode(LastError));
      }
      
      // Look up symbol
      if (!pSymFromName(m_Memory.GetProcessHandle(), &Name[0], pSymbol))
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Symbols::GetAddress") << 
          ErrorString("Failed to get address for symbol.") << 
          ErrorCode(LastError));
      }
      
      // Return symbol address
      return reinterpret_cast<PVOID>(pSymbol->Address);
    }
  }
}
