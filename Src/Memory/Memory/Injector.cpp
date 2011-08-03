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
#include <HadesMemory/Detail/Config.hpp>
#include <HadesMemory/Detail/WinAux.hpp>
#include <HadesMemory/Detail/ArgQuote.hpp>
#include <HadesMemory/Detail/StringBuffer.hpp>
#include <HadesMemory/Detail/EnsureCleanup.hpp>

// Windows API
#include <Windows.h>

// C++ Standard Library
#include <algorithm>

// Boost
#include <boost/filesystem.hpp>
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

namespace HadesMem
{
  // Constructor
  Injector::Injector(MemoryMgr const& MyMemory) 
    : m_Memory(MyMemory)
  { }
      
  // Copy constructor
  Injector::Injector(Injector const& Other)
    : m_Memory(Other.m_Memory)
  { }
  
  // Copy assignment operator
  Injector& Injector::operator=(Injector const& Other)
  {
    this->m_Memory = Other.m_Memory;
    
    return *this;
  }
  
  // Move constructor
  Injector::Injector(Injector&& Other)
    : m_Memory(std::move(Other.m_Memory))
  { }
  
  // Move assignment operator
  Injector& Injector::operator=(Injector&& Other)
  {
    this->m_Memory = std::move(Other.m_Memory);
    
    return *this;
  }
  
  // Destructor
  Injector::~Injector()
  { }

  // Inject DLL
  HMODULE Injector::InjectDll(std::wstring const& Path, 
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
      PathReal = boost::filesystem::absolute(PathReal, 
        Detail::GetSelfDirPath());
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
    m_Memory.WriteString(LibFileRemote.GetBase(), PathString);
    
    // Get address of LoadLibraryW in Kernel32.dll
    Module Kernel32Mod(m_Memory, L"kernel32.dll");
    FARPROC const pLoadLibraryW = Kernel32Mod.FindProcedure("LoadLibraryW");
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
    FARPROC const pFreeLibrary = Kernel32Mod.FindProcedure("FreeLibrary");
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
    HMODULE RemoteModule, std::string const& Export) const
  {
    // Get export address
    Module TargetMod(m_Memory, RemoteModule);
    FARPROC const pExportAddr = TargetMod.FindProcedure(Export);
    DWORD_PTR const pExportAddrTemp = reinterpret_cast<DWORD_PTR>(
      pExportAddr);

    // Create a remote thread that calls the desired export
    std::vector<PVOID> ExportArgs;
    ExportArgs.push_back(RemoteModule);
    return m_Memory.Call(reinterpret_cast<PVOID>(pExportAddrTemp), 
      MemoryMgr::CallConv_Default, ExportArgs);
  }
  
  // Equality operator
  bool Injector::operator==(Injector const& Rhs) const
  {
    return m_Memory == Rhs.m_Memory;
  }
  
  // Inequality operator
  bool Injector::operator!=(Injector const& Rhs) const
  {
    return !(*this == Rhs);
  }

  // Constructor
  CreateAndInjectData::CreateAndInjectData(MemoryMgr const& MyMemory, 
    HMODULE Module, DWORD_PTR ExportRet, DWORD ExportLastError) 
    : m_Memory(MyMemory), 
    m_Module(Module), 
    m_ExportRet(ExportRet), 
    m_ExportLastError(ExportLastError)
  { }
  
  // Copy constructor
  CreateAndInjectData::CreateAndInjectData(CreateAndInjectData const& Other)
    : m_Memory(Other.m_Memory), 
    m_Module(Other.m_Module), 
    m_ExportRet(Other.m_ExportRet), 
    m_ExportLastError(Other.m_ExportLastError)
  { }
  
  // Get memory manager
  MemoryMgr CreateAndInjectData::GetMemoryMgr() const
  {
    return m_Memory;
  }
  
  // Get module
  HMODULE CreateAndInjectData::GetModule() const
  {
    return m_Module;
  }
  
  // Get export return value
  DWORD_PTR CreateAndInjectData::GetExportRet() const
  {
    return m_ExportRet;
  }
  
  // Get export last error code
  DWORD CreateAndInjectData::GetExportLastError() const
  {
    return m_ExportLastError;
  }
  
  // Create process (as suspended) and inject DLL
  CreateAndInjectData CreateAndInject(
    std::wstring const& Path, 
    std::wstring const& WorkDir, 
    std::vector<std::wstring> const& Args, 
    std::wstring const& Module, 
    std::string const& Export, 
    Injector::InjectFlags Flags)
  {
    // Create filesystem object for path
    boost::filesystem::path const PathReal(Path);
    
    // Set up args for CreateProcess
    STARTUPINFO StartInfo;
    ZeroMemory(&StartInfo, sizeof(StartInfo));
    StartInfo.cb = sizeof(StartInfo);
    PROCESS_INFORMATION ProcInfo;
    ZeroMemory(&ProcInfo, sizeof(ProcInfo));

    // Construct command line.
    std::wstring CommandLine;
    Detail::ArgvQuote(PathReal.native(), CommandLine, false);
    std::for_each(Args.begin(), Args.end(), 
      [&] (std::wstring const& Arg) 
      {
        CommandLine += L' ';
        Detail::ArgvQuote(Arg, CommandLine, false);
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
    else if (PathReal.has_parent_path())
    {
      WorkDirReal = PathReal.parent_path();
    }
    else
    {
      WorkDirReal = L"./";
    }

    // Attempt process creation
    if (!CreateProcess(PathReal.c_str(), ProcArgs.data(), nullptr, nullptr, FALSE, 
      CREATE_SUSPENDED, nullptr, WorkDirReal.c_str(), &StartInfo, &ProcInfo))
    {
      DWORD const LastError = GetLastError();
      BOOST_THROW_EXCEPTION(Injector::Error() << 
        ErrorFunction("CreateAndInject") << 
        ErrorString("Could not create process.") << 
        ErrorCodeWinLast(LastError));
    }

    // Ensure cleanup
    Detail::EnsureCloseHandle const ProcHandle(ProcInfo.hProcess);
    Detail::EnsureCloseHandle const ThreadHandle(ProcInfo.hThread);

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
      MyMemory.WriteList(pRemoteStub, CodeReal);

      // Call stub via creating a remote thread in the target.
      Detail::EnsureCloseHandle const MyThread(CreateRemoteThread(
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
      MemoryMgr::RemoteFunctionRet ExpRetData(0, 0, 0);
      if (!Export.empty())
      {
        ExpRetData = MyInjector.CallExport(ModBase, Export);
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
}
