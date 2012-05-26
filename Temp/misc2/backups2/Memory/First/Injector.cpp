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

// Hades
#include <HadesMemory/Injector.hpp>
#include <HadesMemory/Module.hpp>
#include <HadesMemory/MemoryMgr.hpp>
#include <HadesCommon/Filesystem.hpp>
#include <HadesCommon/EnsureCleanup.hpp>

// Windows API
#include <Windows.h>

// C++ Standard Library
#include <algorithm>

// Boost
#include <boost/algorithm/string.hpp>

// AsmJit
#ifdef HADES_MSVC
#pragma warning(push, 1)
#endif
#ifdef HADES_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-pedantic"
#pragma GCC diagnostic ignored "-Wattributes"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#include <AsmJit/AsmJit.h>
#ifdef HADES_MSVC
#pragma warning(pop)
#endif
#ifdef HADES_GCC
#pragma GCC diagnostic pop
#endif

namespace Hades
{
  namespace Memory
  {
    // This routine appends the given argument to a command line such
    // that CommandLineToArgvW will return the argument string unchanged.
    // Arguments in a command line should be separated by spaces; this
    // function does not add these spaces.
    void ArgvQuote(const std::wstring& Argument, std::wstring& CommandLine, 
      bool Force)    
    {
      // Unless we're told otherwise, don't quote unless we actually
      // need to do so --- hopefully avoid problems if programs won't
      // parse quotes properly
      if (!Force && !Argument.empty () && Argument.find_first_of(L" \t\n\v\"") 
        == Argument.npos)
      {
        CommandLine.append(Argument);
      }
      else 
      {
        CommandLine.push_back(L'"');
        
        for (auto It = Argument.begin(); ;++It)
        {
          unsigned NumberBackslashes = 0;
          
          while (It != Argument.end () && *It == L'\\') 
          {
            ++It;
            ++NumberBackslashes;
          }
      
          if (It == Argument.end ())
          {
            // Escape all backslashes, but let the terminating
            // double quotation mark we add below be interpreted
            // as a metacharacter.
            CommandLine.append(NumberBackslashes * 2, L'\\');
            break;
          }
          else if (*It == L'"')
          {
            // Escape all backslashes and the following
            // double quotation mark.
            CommandLine.append(NumberBackslashes * 2 + 1, L'\\');
            CommandLine.push_back(*It);
          }
          else
          {
            // Backslashes aren't special here.
            CommandLine.append(NumberBackslashes, L'\\');
            CommandLine.push_back(*It);
          }
        }
        
        CommandLine.push_back(L'"');
      }
    }

    // Create process (as suspended) and inject DLL
    CreateAndInjectData CreateAndInject(
      boost::filesystem::path const& Path, 
      boost::filesystem::path const& WorkDir, 
      std::vector<std::wstring> const& Args, 
      boost::filesystem::path const& Module, 
      std::string const& Export, 
      Injector::InjectFlags Flags)
    {
      // Set up args for CreateProcess
      STARTUPINFO StartInfo;
      ZeroMemory(&StartInfo, sizeof(StartInfo));
      StartInfo.cb = sizeof(StartInfo);
      PROCESS_INFORMATION ProcInfo;
      ZeroMemory(&ProcInfo, sizeof(ProcInfo));

      // Construct command line.
      std::wstring CommandLine;
      ArgvQuote(Path.native(), CommandLine, false);
      std::for_each(Args.begin(), Args.end(), 
        [&] (std::wstring const& Arg) 
        {
          CommandLine += L' ';
          ArgvQuote(Arg, CommandLine, false);
        });
      
      // Copy command line to buffer
      std::vector<wchar_t> ProcArgs(CommandLine.cbegin(), CommandLine.cend());
      ProcArgs.push_back(L'\0');
      
      // Set working directory
      boost::filesystem::path WorkDirReal;
      if (!WorkDir.empty())
      {
        WorkDirReal = WorkDir;
      }
      else if (Path.has_parent_path())
      {
        WorkDirReal = Path.parent_path();
      }
      else
      {
        WorkDirReal = L"./";
      }

      // Attempt process creation
      if (!CreateProcess(Path.c_str(), ProcArgs.data(), nullptr, nullptr, FALSE, 
        CREATE_SUSPENDED, nullptr, WorkDirReal.c_str(), &StartInfo, &ProcInfo))
      {
        DWORD const LastError = GetLastError();
        BOOST_THROW_EXCEPTION(Injector::Error() << 
          ErrorFunction("CreateAndInject") << 
          ErrorString("Could not create process.") << 
          ErrorCodeWinLast(LastError));
      }

      // Ensure cleanup
      Windows::EnsureCloseHandle const ProcHandle(ProcInfo.hProcess);
      Windows::EnsureCloseHandle const ThreadHandle(ProcInfo.hThread);

      try
      {
        // Memory manager instance
        MemoryMgr const MyMemory(ProcInfo.dwProcessId);
        
        // Create Assembler.
        // This is used to generate a 'nullsub' function, which is called in 
        // the context of the remote process in order to 'force' a call to 
        // ntdll.dll!LdrInitializeThunk. This is necessary because module 
        // enumeration will fail if LdrInitializeThunk has not been called, 
        // and Injector::InjectDll (and the APIs it uses) depend on the 
        // module enumeration APIs.
        AsmJit::Assembler MyJitFunc;

#if defined(_M_AMD64) 
        // Return
        MyJitFunc.ret();
#elif defined(_M_IX86) 
        // Return
        MyJitFunc.ret(AsmJit::Imm(0x4));
#else 
#error "[HadesMem] Unsupported architecture."
#endif

        // Get stub size
        DWORD_PTR const StubSize = MyJitFunc.getCodeSize();
  
        // Allocate memory for stub buffer
        AllocAndFree const StubMemRemote(MyMemory, StubSize);
        PBYTE pRemoteStub = static_cast<PBYTE>(StubMemRemote.GetBase());
        DWORD_PTR pRemoteStubTemp = reinterpret_cast<DWORD_PTR>(pRemoteStub);
  
        // Create buffer to hold relocated code plus the return value address
        std::vector<BYTE> CodeReal(StubSize);
  
        // Generate code
        MyJitFunc.relocCode(CodeReal.data(), reinterpret_cast<DWORD_PTR>(
          pRemoteStub));
  
        // Write stub buffer to process
        MyMemory.Write(pRemoteStub, CodeReal);
  
        // Call stub via creating a remote thread in the target.
        Windows::EnsureCloseHandle const MyThread(CreateRemoteThread(
          MyMemory.GetProcessHandle(), nullptr, 0, 
          reinterpret_cast<LPTHREAD_START_ROUTINE>(pRemoteStubTemp), nullptr, 
          0, nullptr));
        if (!MyThread)
        {
          DWORD const LastError = GetLastError();
          BOOST_THROW_EXCEPTION(Injector::Error() << 
            ErrorFunction("CreateAndInject") << 
            ErrorString("Could not create remote thread.") << 
            ErrorCodeWinLast(LastError));
        }
  
        // Wait for the remote thread to terminate
        if (WaitForSingleObject(MyThread, INFINITE) != WAIT_OBJECT_0)
        {
          DWORD const LastError = GetLastError();
          BOOST_THROW_EXCEPTION(Injector::Error() << 
            ErrorFunction("CreateAndInject") << 
            ErrorString("Could not wait for remote thread.") << 
            ErrorCodeWinLast(LastError));
        }

        // Create DLL injector
        Injector const MyInjector(MyMemory);

        // Inject DLL
        HMODULE const ModBase = MyInjector.InjectDll(Module, Flags);

        // Call export if one has been specified
        MemoryMgr::RemoteFunctionRet ExpRetData;
        if (!Export.empty())
        {
          ExpRetData = MyInjector.CallExport(Module, ModBase, Export);
        }

        // Success! Let the process continue execution.
        if (ResumeThread(ProcInfo.hThread) == static_cast<DWORD>(-1))
        {
          DWORD const LastError = GetLastError();
          BOOST_THROW_EXCEPTION(Injector::Error() << 
            ErrorFunction("CreateAndInject") << 
            ErrorString("Could not resume process.") << 
            ErrorCodeWinLast(LastError) << 
            ErrorCodeWinRet(ExpRetData.GetReturnValue()) << 
            ErrorCodeWinOther(ExpRetData.GetLastError()));
        }

        // Return data to caller
        return CreateAndInjectData(MyMemory, ModBase, 
          ExpRetData.GetReturnValue(), ExpRetData.GetLastError());
      }
      // Catch exceptions
      catch (std::exception const& /*e*/)
      {
        // Terminate process if injection failed
        TerminateProcess(ProcInfo.hProcess, 0);

        // Rethrow exception
        throw;
      }
    }

    // Constructor
    Injector::Injector(MemoryMgr const& MyMemory) 
      : m_Memory(MyMemory)
    { }

    // Inject DLL
    HMODULE Injector::InjectDll(boost::filesystem::path const& Path, 
      InjectFlags Flags) const
    {
      // Do not continue if Shim Engine is enabled for local process, 
      // otherwise it could interfere with the address resolution.
      HMODULE const ShimEngMod = GetModuleHandle(L"ShimEng.dll");
      if (ShimEngMod)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Injector::InjectDll") << 
          ErrorString("Shims enabled for local process."));
      }
      
      // String to hold 'real' path to module
      boost::filesystem::path PathReal(Path);
        
      // Check if path resolution was requested
      bool PathResolution = ((Flags & InjectFlag_PathResolution) == 
        InjectFlag_PathResolution);

      // Check whether we need to convert the path from a relative to 
      // an absolute
      if (PathResolution && PathReal.is_relative())
      {
        // Convert relative path to absolute path
        PathReal = boost::filesystem::absolute(PathReal, Hades::Windows::
          GetSelfDirPath());
      }

      // Convert path to preferred format
      PathReal.make_preferred();

      // Ensure target file exists
      // Note: Only performing this check when path resolution is enabled, 
      // because otherwise we would need to perform the check in the context 
      // of the remote process, which is not possible to do without 
      // introducing race conditions and other potential problems. So we just 
      // let LoadLibraryW do the check for us.
      if (PathResolution && !boost::filesystem::exists(PathReal))
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Injector::InjectDll") << 
          ErrorString("Could not find module file."));
      }

      // Get path as string
      std::wstring const PathString(PathReal.native());

      // Calculate the number of bytes needed for the DLL's pathname
      std::size_t const PathBufSize = (PathString.size() + 1) * 
        sizeof(wchar_t);

      // Allocate space in the remote process for the pathname
      AllocAndFree const LibFileRemote(m_Memory, PathBufSize);

      // Copy the DLL's pathname to the remote process' address space
      m_Memory.Write(LibFileRemote.GetBase(), PathString);
      
      // Get address of LoadLibraryW in Kernel32.dll
      Module Kernel32Mod(m_Memory, L"kernel32.dll");
      FARPROC const pLoadLibraryW = m_Memory.GetRemoteProcAddress(
        Kernel32Mod.GetBase(), "kernel32.dll", "LoadLibraryW");
      DWORD_PTR pLoadLibraryWTemp = reinterpret_cast<DWORD_PTR>(pLoadLibraryW);

      // Load module in remote process using LoadLibraryW
      std::vector<PVOID> Args;
      Args.push_back(LibFileRemote.GetBase());
      MemoryMgr::RemoteFunctionRet RemoteRet = m_Memory.Call(
        reinterpret_cast<PVOID>(pLoadLibraryWTemp), 
        MemoryMgr::CallConv_Default, Args);
      if (!RemoteRet.GetReturnValue())
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Injector::InjectDll") << 
          ErrorString("Call to LoadLibraryW in remote process failed.") << 
          ErrorCodeWinLast(RemoteRet.GetLastError()));
      }
      
      // Return module base
      return reinterpret_cast<HMODULE>(RemoteRet.GetReturnValue());
    }

    // Free DLL
    void Injector::FreeDll(HMODULE ModuleRemote) const
    {
      // Get address of FreeLibrary in Kernel32.dll
      Module Kernel32Mod(m_Memory, L"kernel32.dll");
      FARPROC const pFreeLibrary = m_Memory.GetRemoteProcAddress(
        Kernel32Mod.GetBase(), "kernel32.dll", "FreeLibrary");
      DWORD_PTR pFreeLibraryTemp = reinterpret_cast<DWORD_PTR>(pFreeLibrary);

      // Free module in remote process using FreeLibrary
      std::vector<PVOID> Args;
      Args.push_back(reinterpret_cast<PVOID>(ModuleRemote));
      MemoryMgr::RemoteFunctionRet RemoteRet = m_Memory.Call(
        reinterpret_cast<PVOID>(pFreeLibraryTemp), 
        MemoryMgr::CallConv_Default, Args);
      if (!RemoteRet.GetReturnValue())
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Injector::FreeDll") << 
          ErrorString("Call to FreeLibrary in remote process failed.") << 
          ErrorCodeWinLast(RemoteRet.GetLastError()));
      }
    }

    // Call export
    MemoryMgr::RemoteFunctionRet Injector::CallExport(
      boost::filesystem::path const& ModulePath, 
      HMODULE ModuleRemote, std::string const& Export) const
    {
      // Get export address
      FARPROC const pExportAddr = m_Memory.GetRemoteProcAddress(ModuleRemote, 
        ModulePath, Export);
      DWORD_PTR const pExportAddrTemp = reinterpret_cast<DWORD_PTR>(
        pExportAddr);

      // Create a remote thread that calls the desired export
      std::vector<PVOID> ExportArgs;
      ExportArgs.push_back(ModuleRemote);
      return m_Memory.Call(reinterpret_cast<PVOID>(pExportAddrTemp), 
        MemoryMgr::CallConv_Default, ExportArgs);
    }
  }
}
