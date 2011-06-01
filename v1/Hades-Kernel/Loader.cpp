// C++ Standard Library
#include <fstream>
#include <iostream>

// Boost
#include <boost/format.hpp>

// RapidXML
#include <RapidXML/rapidxml.hpp>

// Hades
#include "Loader.h"
#include "Kernel.h"
#include "Hades-Memory/Injector.h"
#include "Hades-Common/Filesystem.h"
#include "Hades-Common/EnsureCleanup.h"

namespace Hades
{
  namespace Kernel
  {
    // Memory manager
    std::shared_ptr<Memory::MemoryMgr> Loader::m_Memory;

    // Settings
    Loader::ProcAndModList Loader::m_ProcsAndMods;

    // CreateProcessInternalW hook
    std::shared_ptr<Memory::PatchDetour> Loader::m_pCreateProcessInternalWHk;

    // Initialize Loader
    void Loader::Initialize(Kernel* pKernel)
    {
      // Set memory manager
      m_Memory = pKernel->GetMemoryMgr();

      // Ensure hooks are only applied once
      if (!m_pCreateProcessInternalWHk && pKernel->IsHookEnabled(
        L"kernel32.dll!CreateProcessInternalW"))
      {
        // Get handle to Kernel32
        HMODULE Kernel32Mod = GetModuleHandle(L"kernel32.dll");
        if (!Kernel32Mod)
        {
          DWORD LastError = GetLastError();
          BOOST_THROW_EXCEPTION(LoaderError() << 
            ErrorFunction("Loader::Initialize") << 
            ErrorString("Could not find Kernel32.dll.") << 
            ErrorCodeWin(LastError));
        }

        // Get address of CreateProcessInternalW
        FARPROC pCreateProcessInternalW = GetProcAddress(Kernel32Mod, 
          "CreateProcessInternalW");
        if (!pCreateProcessInternalW)
        {
          DWORD LastError = GetLastError();
          BOOST_THROW_EXCEPTION(LoaderError() << 
            ErrorFunction("Loader::Initialize") << 
            ErrorString("Could not find CreateProcessInternalW.") << 
            ErrorCodeWin(LastError));
        }

        // Target and detour pointer
        PBYTE Target = reinterpret_cast<PBYTE>(pCreateProcessInternalW);
        PBYTE Detour = reinterpret_cast<PBYTE>(CreateProcessInternalW_Hook);

        // Debug output
        std::wcout << "Loader::Initialize: Hooking kernel32.dll!"
          "CreateProcessInternalW." << std::endl;
        std::wcout << boost::wformat(L"Loader::Initialize: Target = %p, "
          L"Detour = %p.") %Target %Detour << std::endl;

        // Hook CreateProcessInternalW
        m_pCreateProcessInternalWHk.reset(new Memory::PatchDetour(*m_Memory, 
          Target, Detour));
        m_pCreateProcessInternalWHk->Apply();
      }

      // Debug output
      std::wcout << "Loader::Loader: Loader initialized." << std::endl;
    }

    // Initialize settings and hook APIs
    void Loader::AddExe(const std::wstring& ProcessName, 
      const std::wstring& ModuleName)
    {
      // Initialize data
      m_ProcsAndMods.push_back(std::make_pair(ProcessName, ModuleName));
    }

    // CreateProcessW API hook
    BOOL WINAPI Loader::CreateProcessInternalW_Hook(PVOID Unknown1, 
      LPCWSTR lpApplicationName, LPWSTR lpCommandLine, 
      LPSECURITY_ATTRIBUTES lpProcessAttributes, 
      LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, 
      DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, 
      LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation, 
      PVOID Unknown2)
    {
      try
      {
        // Get trampoline
        typedef BOOL (WINAPI* tCreateProcessInternalW)(
          PVOID Unknown1, 
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
          PVOID Unknown2);
        auto pCreateProcessW = reinterpret_cast<tCreateProcessInternalW>(
          m_pCreateProcessInternalWHk->GetTrampoline());

        // Debug output
        std::wcout << "Loader::CreateProcessInternalW_Hook: Called." 
          << std::endl;

        // Check application name
        std::wstring AppName;
        if (lpApplicationName) 
        {
          AppName = lpApplicationName;
        }
        else if (lpCommandLine) 
        {
          AppName = lpCommandLine; 
        }
        else 
        {
          AppName = L"<None>";
        }

        // If suspended process is requested by caller don't automatically 
        // resume.
        bool ResumeProc = !(dwCreationFlags & CREATE_SUSPENDED);

        // Call original API
        BOOL RetVal = pCreateProcessW(
          Unknown1, 
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
          Unknown2);

        // Ensure thread is resumed if required
        Windows::EnsureResumeThread ProcThread(ResumeProc ? 
          lpProcessInformation->hThread : nullptr);

        // Debug output
        std::wcout << boost::wformat(L"Loader::CreateProcessInternalW_Hook: "
          L"App = %s, CmdLine = %s. Return = %u.") 
          %(lpApplicationName ? lpApplicationName : L"<None>") 
          %(lpCommandLine ? lpCommandLine: L"<None>") 
          %RetVal << std::endl;

        // Backup previous error code
        DWORD LastError = GetLastError();

        // Check call succeeded
        // Check if we need to go any further
        if (!RetVal)
        {
          SetLastError(LastError);
          return RetVal;
        }

        // Check if the process is one we want to inject into
        std::wstring Module(ShouldInject(AppName));
        if (Module.empty())
        {
          SetLastError(LastError);
          return RetVal;
        }

        // Attempt the injection
        AttemptInjection(lpProcessInformation, Module);

        // Return value from trampoline
        SetLastError(LastError);
        return RetVal;
      }
      catch (boost::exception const& e)
      {
        // Debug output
        std::cout << boost::format("Loader::CreateProcessInternalW_Hook: "
          "Error! %s.") %boost::diagnostic_information(e) << std::endl;
      }
      catch (std::exception const& e)
      {
        // Debug output
        std::cout << boost::format("Loader::CreateProcessInternalW_Hook: "
          "Error! %s.") %e.what() << std::endl;
      }

      return FALSE;
    }

    // Attempt to inject module into target
    void Loader::AttemptInjection(LPPROCESS_INFORMATION ProcInfo, 
      const std::wstring& Module)
    {
      // Exception handling
      try
      {
        // Path to module
        std::wstring Path(Windows::GetSelfPath().file_string());

        // If we get to here then we're attempting to inject into the process
        std::cout << "Loader::AttemptInjection: Attempting injection!" 
          << std::endl;

        // Check if the current process is x86 and the target process is x64
        // i.e. Do we need to break the WoW64 barrier?
        BOOL IsWoW64 = FALSE;
        if (IsWow64Process(GetCurrentProcess(), &IsWoW64) && IsWoW64 && 
          IsWow64Process(ProcInfo->hProcess, &IsWoW64) && !IsWoW64)
        {
          // Debug output
          std::cout << "Loader::AttemptInjection: Attempting to break WoW64 "
            "barrier." << std::endl;

          // Construct module path
          std::wstring const ModulePath(Windows::GetSelfDirPath().
            file_string() + L"\\" + Module);

          // Construct injector path
          std::wstring const AppPath(Windows::GetSelfDirPath().file_string() + 
            L"\\Hades-MemHack_AMD64.exe");

          // Export to call
          std::wstring Export(L"_Initialize@4");

          // Construct loader args
          std::wstring const Args(boost::str(boost::wformat(
            L"HadesMem.WriteLn(\"Creating memory manager.\"); "
            L"MyMem = HadesMem.CreateMemoryMgr(%u); "
            L"HadesMem.WriteLn(\"Creating injector.\"); "
            L"MyInjector = HadesMem.CreateInjector(MyMem); "
            L"HadesMem.WriteLn(\"Injecting Hades-Kernel.\"); "
            L"ModBase = MyInjector:InjectDll(\"%s\"); "
            L"HadesMem.WriteLn(\"Base: \".. HadesMem.ToHexStr(ModBase)); "
            L"InitRet = MyInjector:CallExport(\"%s\", ModBase, \"%s\");"
            L"HadesMem.WriteLn(\"Initialize: \".. InitRet); "
            ) 
            %ProcInfo->dwProcessId 
            %ModulePath 
            %ModulePath
            %Export));

          // Construct command line.
          std::wstring CommandLine(L"\"" + AppPath + L"\" " + Args);
          // Copy command line to buffer
          std::vector<wchar_t> ProcArgs(CommandLine.begin(), 
            CommandLine.end());
          // Null-terminate buffer
          ProcArgs.push_back(L'\0');

          // CreateProcess args
          STARTUPINFOW SInfo = { sizeof(SInfo) };
          PROCESS_INFORMATION PInfo = { 0 };

          // Run the 64-bit injector
          if (!CreateProcess(AppPath.c_str(), &ProcArgs[0], nullptr, nullptr, 
            FALSE, 0, nullptr, nullptr, &SInfo, &PInfo))
          {
            DWORD LastError = GetLastError();
            BOOST_THROW_EXCEPTION(LoaderError() << 
              ErrorFunction("Loader::AttemptInjection") << 
              ErrorString("Could not run x64 injector.") << 
              ErrorCodeWin(LastError));
          }

          // Wait for the injector to terminate
          if (WaitForSingleObject(PInfo.hProcess, INFINITE) != WAIT_OBJECT_0)
          {
            DWORD LastError = GetLastError();
            BOOST_THROW_EXCEPTION(LoaderError() << 
              ErrorFunction("Loader::AttemptInjection") << 
              ErrorString("Waiting for injector process failed.") << 
              ErrorCodeWin(LastError));
          }
        }
        else
        {
          // Create memory manager for target
          Memory::MemoryMgr TargetMemory(ProcInfo->dwProcessId);

          // Create injector for target
          Memory::Injector TargetInjector(TargetMemory);

          // Inject module
          TargetInjector.CallExport(Path, TargetInjector.InjectDll(Path), 
            "_Initialize@4");
        }

        // If we get to here then the injection worked
        std::cout << "Loader::AttemptInjection: Injection succeeded!" 
          << std::endl;
      }
      catch (boost::exception const& e)
      {
        // Debug output
        std::cout << boost::format("Loader::AttemptInjection: Error! %s.") 
          %boost::diagnostic_information(e) << std::endl;
      }
      catch (std::exception const& e)
      {
        // Debug output
        std::cout << boost::format("Loader::AttemptInjection: Error! %s.") 
          %e.what() << std::endl;
      }
    }

    // Whether we should inject into the process
    std::wstring Loader::ShouldInject(const std::wstring& ProcessName)
    {
      // Check if current process is in target list
      auto Iter = std::find_if(m_ProcsAndMods.begin(), m_ProcsAndMods.end(), 
        [&] (ProcAndMod const& Current)
      {
        return boost::to_lower_copy(ProcessName).find(boost::to_lower_copy(
          Current.first)) != std::wstring::npos;
      });

      // Return module if process is injection target, or an empty string 
      // otherwise.
      return Iter != m_ProcsAndMods.end() ? Iter->second : std::wstring();
    }

    // Load configuration data from XML file
    void Loader::LoadConfig(std::wstring const& Path)
    {
      // Open config file
      std::wifstream ConfigFile(Path.c_str());
      if (!ConfigFile)
      {
        BOOST_THROW_EXCEPTION(LoaderError() << 
          ErrorFunction("Loader::LoadConfig") << 
          ErrorString("Could not open config file."));
      }

      // Copy file to buffer
      std::istreambuf_iterator<wchar_t> const ConfFileBeg(ConfigFile);
      std::istreambuf_iterator<wchar_t> const ConfFileEnd;
      std::vector<wchar_t> ConfFileBuf(ConfFileBeg, ConfFileEnd);
      ConfFileBuf.push_back(L'\0');

      // Open XML document
      rapidxml::xml_document<wchar_t> ConfigDoc;
      ConfigDoc.parse<0>(&ConfFileBuf[0]);

      // Ensure loader tag is found
      auto const TargetsTag = ConfigDoc.first_node(L"Targets");
      if (!TargetsTag)
      {
        BOOST_THROW_EXCEPTION(LoaderError() << 
          ErrorFunction("Loader::LoadConfig") << 
          ErrorString("Invalid config file format."));
      }

      // Loop over all targets
      for (auto Pattern = TargetsTag->first_node(L"Target"); Pattern; 
        Pattern = Pattern->next_sibling(L"Target"))
      {
        // Get target attributes
        auto const NameNode = Pattern->first_attribute(L"Name");
        auto const ModuleNode = Pattern->first_attribute(L"Module");
        std::wstring const Name(NameNode ? NameNode->value() : L"");
        std::wstring const Module(ModuleNode ? ModuleNode->value() : L"");

        // Ensure data is valid
        if (Name.empty() || Module.empty())
        {
          BOOST_THROW_EXCEPTION(LoaderError() << 
            ErrorFunction("Loader::LoadConfig") << 
            ErrorString("Invalid target attributes."));
        }

        // Add current game
        Loader::AddExe(Name, Module);
      }
    }
  }
}
