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

// Hades
#include "Loader.hpp"
#include "HadesCommon/Logger.hpp"

namespace Hades
{
  namespace Kernel
  {
    Kernel* Loader::m_pKernel = nullptr;
    std::shared_ptr<Hades::Memory::PatchDetour> Loader::m_pCreateProcessInternalWHk;
      
    void Loader::Initialize(Kernel& MyKernel)
    {
      m_pKernel = &MyKernel;
    }
    
    void Loader::Hook()
    {
      HMODULE Kernel32Mod = GetModuleHandle(L"kernel32.dll");
      if (!Kernel32Mod)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Loader::Hook") << 
          ErrorString("Could not find Kernel32.dll") << 
          ErrorCode(LastError));
      }
      
      FARPROC const pCreateProcessInternalW = GetProcAddress(Kernel32Mod, 
        "CreateProcessInternalW");
      if (!pCreateProcessInternalW)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Loader::Hook") << 
          ErrorString("Could not find Kernel32!CreateProcessInternalW") << 
          ErrorCode(LastError));
      }
      
      HADES_LOG_THREAD_SAFE(std::wcout << boost::wformat(L"Loader::"
        L"Hook: pCreateProcessInternalW = %p.") %pCreateProcessInternalW 
        << std::endl);
          
      Memory::MemoryMgr const MyMemory(GetCurrentProcessId());
      m_pCreateProcessInternalWHk.reset(new Memory::PatchDetour(MyMemory, 
        pCreateProcessInternalW, &CreateProcessInternalW_Hook));
      m_pCreateProcessInternalWHk->Apply();
    }
    
    void Loader::Unhook()
    {
      if (m_pCreateProcessInternalWHk)
      {
        m_pCreateProcessInternalWHk->Remove();
      }
    }
      
    BOOL WINAPI Loader::CreateProcessInternalW_Hook(
      HANDLE hToken,
      LPCWSTR lpApplicationName,
      LPWSTR lpCommandLine,
      LPSECURITY_ATTRIBUTES lpProcessAttributes,
      LPSECURITY_ATTRIBUTES lpThreadAttributes,
      BOOL bInheritHandles,
      DWORD dwCreationFlags,
      LPVOID lpEnvironment,
      LPCWSTR lpCurrentDirectory,
      LPSTARTUPINFOW lpStartupInfo,
      LPPROCESS_INFORMATION lpProcessInformation,
      PHANDLE hNewToken)
    {
      HADES_LOG_THREAD_SAFE(std::wcout << 
        "Loader::CreateProcessInternalW_Hook: Called." << std::endl);
          
      typedef BOOL (WINAPI* tCreateProcessInternalW)(
        HANDLE hToken,
        LPCWSTR lpApplicationName,
        LPWSTR lpCommandLine,
        LPSECURITY_ATTRIBUTES lpProcessAttributes,
        LPSECURITY_ATTRIBUTES lpThreadAttributes,
        BOOL bInheritHandles,
        DWORD dwCreationFlags,
        LPVOID lpEnvironment,
        LPCWSTR lpCurrentDirectory,
        LPSTARTUPINFOW lpStartupInfo,
        LPPROCESS_INFORMATION lpProcessInformation,
        PHANDLE hNewToken);
      auto pCreateProcessInternalW = reinterpret_cast<tCreateProcessInternalW>(
        m_pCreateProcessInternalWHk->GetTrampoline());
        
      return pCreateProcessInternalW(hToken, lpApplicationName, lpCommandLine, 
        lpProcessAttributes, lpThreadAttributes, bInheritHandles, 
        dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, 
        lpProcessInformation, hNewToken);
    }
  }
}
