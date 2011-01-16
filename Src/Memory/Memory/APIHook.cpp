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

// Boost
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

// Windows API
#include <Windows.h>
#include <ImageHlp.h>

// Hades
#include "PeFile.hpp"
#include "Module.hpp"
#include "APIHook.hpp"
#include "MemoryMgr.hpp"
#include "ImportDir.hpp"
#include "ImportEnum.hpp"
#include "HadesCommon/I18n.hpp"

namespace Hades
{
  namespace Memory
  {
    // Hook list
    std::list<APIHook*> APIHook::sm_HookList;
    
    // Hook placeholders
    std::shared_ptr<APIHook> APIHook::sm_LoadLibraryA;
    std::shared_ptr<APIHook> APIHook::sm_LoadLibraryW;
    std::shared_ptr<APIHook> APIHook::sm_LoadLibraryExA;
    std::shared_ptr<APIHook> APIHook::sm_LoadLibraryExW;
    std::shared_ptr<APIHook> APIHook::sm_GetProcAddress;
      
    void APIHook::Initialize()
    {
      // Hook LoadLibrary functions and GetProcAddress so that hooked functions 
      // are handled correctly if these functions are called.
      sm_LoadLibraryA.reset(new APIHook(L"Kernel32.dll", L"LoadLibraryA", 
        reinterpret_cast<PROC>(APIHook::LoadLibraryA)));
      sm_LoadLibraryW.reset(new APIHook(L"Kernel32.dll", L"LoadLibraryW", 
        reinterpret_cast<PROC>(APIHook::LoadLibraryW)));
      sm_LoadLibraryExA.reset(new APIHook(L"Kernel32.dll", L"LoadLibraryExA", 
        reinterpret_cast<PROC>(APIHook::LoadLibraryExA)));
      sm_LoadLibraryExW.reset(new APIHook(L"Kernel32.dll", L"LoadLibraryExW", 
        reinterpret_cast<PROC>(APIHook::LoadLibraryExW)));
      sm_GetProcAddress.reset(new APIHook(L"Kernel32.dll", L"GetProcAddress", 
        reinterpret_cast<PROC>(APIHook::GetProcAddress)));
    }
    
    // Hook a function in all modules
    APIHook::APIHook(std::wstring const& ModuleName, 
      std::wstring const& FunctionName, PROC pHook) 
      : m_ModuleName(ModuleName), 
      m_FunctionName(FunctionName), 
      m_pOrig(nullptr), 
      m_pHook(pHook)
    {
      // Note: the function can be hooked only if the exporting module 
      // is already loaded. A solution could be to store the function
      // name as a member; then, in the hooked LoadLibrary* handlers, parse
      // the list of APIHook instances, check if pszCalleeModName
      // is the name of the loaded module to hook its export table and 
      // re-hook the import tables of all loaded modules.

      // Add current instance to hook list
      sm_HookList.push_back(this);

      // Get original function address
      m_pOrig = GetProcAddressRaw(GetModuleHandle(ModuleName.c_str()), 
        boost::lexical_cast<std::string>(FunctionName).c_str());

      // If function does not exit,... bye bye
      // This happens when the module is not already loaded
      if (m_pOrig == NULL)
      {
        std::string ErrorMsg(boost::str(boost::format("Could not find %s!%s.") 
          %boost::lexical_cast<std::string>(m_ModuleName) 
          %boost::lexical_cast<std::string>(m_FunctionName)));
          
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("APIHook::APIHook") << 
          ErrorString(ErrorMsg));
      }

      // Hook this function in all currently loaded modules
      ReplaceIATEntryInAllMods(m_ModuleName, m_pOrig, m_pHook);
    }

    APIHook::~APIHook() 
    {
      // Unhook this function from all modules
      ReplaceIATEntryInAllMods(m_ModuleName, m_pHook, m_pOrig);

      // Remove this object from the linked list
      auto Iter = std::find(sm_HookList.rbegin(), sm_HookList.rend(), this);
      if (Iter != sm_HookList.rend())
      {
        sm_HookList.erase(Iter.base());
      }
    }

    // NOTE: This function must NOT be inlined
    __declspec(noinline) FARPROC WINAPI APIHook::GetProcAddressRaw(
      HMODULE hModule, 
      PCSTR lpProcName
    ) 
    {
      return ::GetProcAddress(hModule, lpProcName);
    }

    // Returns the HMODULE that contains the specified memory address
    static HMODULE ModuleFromAddress(
      PVOID Address
    ) 
    {
      MEMORY_BASIC_INFORMATION MyMbi;
      return (
        (VirtualQuery(Address, &MyMbi, sizeof(MyMbi)) != 0) 
        ? reinterpret_cast<HMODULE>(MyMbi.AllocationBase) 
        : NULL
      );
    }

    void APIHook::ReplaceIATEntryInAllMods(
      std::wstring const& ModuleName, 
      PROC pCurrent, 
      PROC pNew
    ) 
    {
      HMODULE ThisMod = ModuleFromAddress(reinterpret_cast<PVOID>(
        ReplaceIATEntryInAllMods));
          
      static MemoryMgr MyMemory(GetCurrentProcessId());
      for (ModuleListIter i(MyMemory); *i; ++i)
      {
        Module& Current = **i;

        // NOTE: We don't hook functions in our own module
        if (Current.GetBase() != ThisMod) 
        {
          // Hook this function in this module
          ReplaceIATEntryInOneMod(ModuleName, pCurrent, pNew, 
            Current.GetBase());
        }
      }
    }

    // Handle unexpected exceptions if the module is unloaded
    LONG WINAPI InvalidReadExceptionFilter(
      PEXCEPTION_POINTERS /*pExceptionInfo*/
    )
    {
      // Handle all unexpected exceptions because we simply don't patch
      // any module in that case
      LONG Disposition = EXCEPTION_EXECUTE_HANDLER;

      // Note: pExceptionInfo->ExceptionRecord->ExceptionCode has 
      // 0xc0000005 as a value

      return Disposition;
    }

    void APIHook::ReplaceIATEntryInOneMod(
      std::wstring const& ModuleName, 
      PROC pCurrent, 
      PROC pNew, 
      HMODULE CallerMod
    ) 
    {
      MemoryMgr MyMemory(GetCurrentProcessId());
      PeFile MyPeFile(MyMemory, CallerMod);
      ImportDir const CheckImpDir(MyPeFile);
      if (!CheckImpDir.IsValid())
      {
        return;
      }

      std::string ModuleNameA(boost::lexical_cast<std::string>(ModuleName));
      boost::to_lower(ModuleNameA);
      
      for (ImportDirIter i(MyPeFile); *i; ++i)
      {
        ImportDir const& MyImportDir = **i;
        
        std::string CurMod(MyImportDir.GetName());
        boost::to_lower(CurMod);
        
        if (CurMod != ModuleNameA)
        {
          continue;
        }
        
        for (ImportThunkIter j(MyPeFile, MyImportDir.GetFirstThunk()); *j; ++j)
        {
          ImportThunk const& MyImportThunk = **j;
          
          if (MyImportThunk.GetFunction() == reinterpret_cast<DWORD_PTR>(
            pCurrent))
          {
            MyImportThunk.SetFunction(reinterpret_cast<DWORD_PTR>(pNew));
            return;
          }
        }
      }
    }

    void APIHook::FixupNewlyLoadedModule(
      HMODULE hModule, 
      DWORD dwFlags
    ) 
    {
      PVOID pFixupNewlyLoadedModule = reinterpret_cast<PVOID>(
        FixupNewlyLoadedModule);
        
      // MinGW workaround
      #ifndef LOAD_LIBRARY_AS_IMAGE_RESOURCE
      #define LOAD_LIBRARY_AS_IMAGE_RESOURCE 0x00000020
      #endif
        
      // If a new module is loaded, hook the hooked functions
      if ((hModule != NULL) && 
        (hModule != ModuleFromAddress(pFixupNewlyLoadedModule)) && 
        ((dwFlags & LOAD_LIBRARY_AS_DATAFILE) == 0) &&
        ((dwFlags & LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE) == 0) &&
        ((dwFlags & LOAD_LIBRARY_AS_IMAGE_RESOURCE) == 0)
      ) 
      {
        for (auto i = sm_HookList.rbegin(); i != sm_HookList.rend(); ++i) 
        {
          APIHook* p = *i;
          if (p->m_pOrig != NULL) 
          {
            ReplaceIATEntryInAllMods(p->m_ModuleName, p->m_pOrig, p->m_pHook);  
          } 
          else 
          {
            // We should never get here
            
            std::string ErrorMsg(boost::str(boost::format(
              "Could not find %s!%s.") 
              %boost::lexical_cast<std::string>(p->m_ModuleName) 
              %boost::lexical_cast<std::string>(p->m_FunctionName)));
              
            BOOST_THROW_EXCEPTION(Error() << 
              ErrorFunction("APIHook::APIHook") << 
              ErrorString(ErrorMsg));
          }
        }
      }
    }

    HMODULE WINAPI APIHook::LoadLibraryA(
      PCSTR lpFileName
    ) 
    {
      HMODULE hModule = ::LoadLibraryA(lpFileName);
      FixupNewlyLoadedModule(hModule, 0);
      return hModule;
    }

    HMODULE WINAPI APIHook::LoadLibraryW(
      PCWSTR lpFileName
    ) 
    {
      HMODULE hModule = ::LoadLibraryW(lpFileName);
      FixupNewlyLoadedModule(hModule, 0);
      return hModule;
    }

    HMODULE WINAPI APIHook::LoadLibraryExA(
      PCSTR lpFileName, 
      HANDLE hFile, 
      DWORD dwFlags
    ) 
    {
       HMODULE hModule = ::LoadLibraryExA(lpFileName, hFile, dwFlags);
       FixupNewlyLoadedModule(hModule, dwFlags);
       return hModule;
    }

    HMODULE WINAPI APIHook::LoadLibraryExW(
      PCWSTR lpFileName, 
      HANDLE hFile, 
      DWORD dwFlags
    ) 
    {
      HMODULE hModule = ::LoadLibraryExW(lpFileName, hFile, dwFlags);
      FixupNewlyLoadedModule(hModule, dwFlags);
      return hModule;
    }
    
    FARPROC WINAPI APIHook::GetProcAddress(
      HMODULE hModule, 
      PCSTR lpProcName
    ) 
    {
      // Get the true address of the function
      FARPROC pFunc = GetProcAddressRaw(hModule, lpProcName);

      // Is it one of the functions that we want hooked?
      for (auto i = sm_HookList.rbegin(); (i != sm_HookList.rend()) && 
        (pFunc != NULL); ++i) 
      {
        APIHook* p = *i;
        if (pFunc == p->m_pOrig) 
        {
          // The address to return matches an address we want to hook
          // Return the hook function address instead
          pFunc = p->m_pHook;
          break;
        }
      }

      return pFunc;
    }
  }
}
