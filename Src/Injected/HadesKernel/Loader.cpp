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
    // Todo: Hook other process creation APIs such as ShellExecuteEx after 
    // confirming they don't use already hooked APIs internally.
    
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
      
      // If suspended process is requested by caller don't automatically 
      // resume.
      bool ResumeProc = !(dwCreationFlags & CREATE_SUSPENDED);

      // Call trampoline
      BOOL Ret = pCreateProcessInternalW(
        hToken, 
        lpApplicationName, 
        lpCommandLine, 
        lpProcessAttributes, 
        lpThreadAttributes, 
        bInheritHandles, 
        (dwCreationFlags | CREATE_SUSPENDED), 
        lpEnvironment, 
        lpCurrentDirectory, 
        lpStartupInfo, 
        lpProcessInformation, 
        hNewToken);
        
      // Debug output
      HADES_LOG_THREAD_SAFE(
      std::wcout << boost::wformat(L"Loader::CreateProcessInternalW_Hook: "
        L"App = %s, CmdLine = %s. Return = %u.") 
        %(lpApplicationName ? lpApplicationName : L"<None>") 
        %(lpCommandLine ? lpCommandLine: L"<None>") 
        %Ret << std::endl);

      // Return if call failed
      if (!Ret)
      {
        return Ret;
      }
      
      // Ensure thread is resumed if required
      Windows::EnsureResumeThread ProcThread(ResumeProc ? 
        lpProcessInformation->hThread : nullptr);
      
      // Attempt injection
      try
      {
        // Get WoW64 status of target
        BOOL IsTargetWoW64 = FALSE;
        if (!IsWow64Process(lpProcessInformation->hProcess, &IsTargetWoW64))
        {
          std::error_code const LastError = GetLastErrorCode();
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("Loader::CreateProcessInternalW_Hook") << 
            ErrorString("Could not get WoW64 status of target.") << 
            ErrorCode(LastError));
        }
        
        // Get WoW64 status of self
        BOOL IsMeWoW64 = FALSE;
        if (!IsWow64Process(GetCurrentProcess(), &IsMeWoW64))
        {
          std::error_code const LastError = GetLastErrorCode();
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("Loader::CreateProcessInternalW_Hook") << 
            ErrorString("Could not get WoW64 status of self.") << 
            ErrorCode(LastError));
        }
        
        // Check if the current process is x86 and the target process is x64 
        // or vice-versa
        // i.e. Do we need to break the WoW64 barrier?
        if (IsTargetWoW64 != IsMeWoW64)
        {
          // Breaking the WoW64 barrier in either direction is currrently 
          // unsupported
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("Loader::CreateProcessInternalW_Hook") << 
            ErrorString("WoW64 bypass currently unsupported."));
        }
        
        // Create memory manager
        Memory::MemoryMgr MyMemory(lpProcessInformation->dwProcessId);
          
        // Create injector
        Memory::Injector MyInjector(MyMemory);

        // Inject module
#if defined(_M_AMD64) 
        MyInjector.CallExport("HadesKernel.dll", MyInjector.InjectDll(
          "HadesKernel.dll"), "Initialize");
#elif defined(_M_IX86) 
        MyInjector.CallExport("HadesKernel.dll", MyInjector.InjectDll(
          "HadesKernel.dll"), "_Initialize@4");
#else 
#error "[HadesMem] Unsupported architecture."
#endif
      
        // Debug output
        HADES_LOG_THREAD_SAFE(std::wcout << "Injection successful." 
          << std::endl);
      }
      catch (std::exception const& e)
      {
        // Debug output
        HADES_LOG_THREAD_SAFE(
        std::cout << boost::format("Loader::CreateProcessInternalW_Hook: "
          "Error! %s.") %e.what() << std::endl);
      }
      
      // Return result from trampoline
      return Ret;
    }
  }
}
