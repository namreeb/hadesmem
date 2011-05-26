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
#include <vector>

// Boost
#include <boost/algorithm/string.hpp>

namespace Hades
{
  namespace Memory
  {
    // Create process (as suspended) and inject DLL
    // Todo: Rewrite this API with 'manual' implementations of APIs which rely 
    // on the Module APIs so we don't have to implement workarounds in all the 
    // APIs this function depends on.
    CreateAndInjectData CreateAndInject(
      boost::filesystem::path const& Path, 
      boost::filesystem::path const& WorkDir, 
      std::wstring const& Args, 
      boost::filesystem::path const& Module, 
      std::string const& Export, 
      bool PathResolution)
    {
      // Set up args for CreateProcess
      STARTUPINFO StartInfo;
      ZeroMemory(&StartInfo, sizeof(StartInfo));
      StartInfo.cb = sizeof(StartInfo);
      PROCESS_INFORMATION ProcInfo;
      ZeroMemory(&ProcInfo, sizeof(ProcInfo));

      // Construct command line.
      std::wstring const CommandLine(L"\"" + Path.native() + L"\" " + Args);
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
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Injector::Error() << 
          ErrorFunction("CreateAndInject") << 
          ErrorString("Could not create process.") << 
          ErrorCode(LastError));
      }

      // Ensure cleanup
      Windows::EnsureCloseHandle const ProcHandle(ProcInfo.hProcess);
      Windows::EnsureCloseHandle const ThreadHandle(ProcInfo.hThread);

      try
      {
        // Memory manager instance
        MemoryMgr const MyMemory(ProcInfo.dwProcessId);

        // Create DLL injector
        Injector const MyInjector(MyMemory);

        // Inject DLL
        HMODULE const ModBase = MyInjector.InjectDll(Module, PathResolution);

        // Call export if one has been specified
        std::pair<DWORD_PTR, DWORD> ExpRetData;
        DWORD_PTR ExportRet = 0;
        DWORD ExportLastError = 0;
        if (!Export.empty())
        {
          ExpRetData = MyInjector.CallExport(Module, ModBase, Export);
          ExportRet = ExpRetData.first;
          ExportLastError = ExpRetData.second;
        }

        // Success! Let the process continue execution.
        if (ResumeThread(ProcInfo.hThread) == static_cast<DWORD>(-1))
        {
          std::error_code const LastError = GetLastErrorCode();
          std::error_code const LastErrorRemote = std::error_code(
            ExportLastError, std::system_category());
          BOOST_THROW_EXCEPTION(Injector::Error() << 
            ErrorFunction("CreateAndInject") << 
            ErrorString("Could not resume process.") << 
            ErrorCode(LastError) << 
            ErrorCode(LastErrorRemote));
        }

        // Return data to caller
        return CreateAndInjectData(MyMemory, ModBase, ExportRet, ExportLastError);
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
      bool PathResolution) const
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
      // Note: We can't use our module enumeration APIs here as module 
      // snapshots can't be generated for newly created suspended processes, 
      // and this API is called by CreateAndInjectDll which depends on 
      // exactly that.
      // Todo: Find a way to fix this so we don't have to assume that 
      // kernel32.dll shares a common base address across all processes.
      // Maybe generate a code stub to do a GetModuleHandle and GetProcAddress 
      // in the context of the remote process? Alternatively we could perform 
      // manual PEB enumeration.
      HMODULE const hKernel32 = GetModuleHandle(L"Kernel32.dll");
      if (!hKernel32)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Injector::InjectDll") << 
          ErrorString("Could not get handle to Kernel32.") << 
          ErrorCode(LastError));
      }
      FARPROC const pLoadLibraryW = GetProcAddress(hKernel32, "LoadLibraryW");
      if (!pLoadLibraryW)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Injector::InjectDll") << 
          ErrorString("Could not get pointer to LoadLibraryW.") << 
          ErrorCode(LastError));
      }
      DWORD_PTR const pLoadLibraryWTemp = reinterpret_cast<DWORD_PTR>(
        pLoadLibraryW);

      // Load module in remote process using LoadLibraryW
      std::vector<PVOID> Args;
      Args.push_back(LibFileRemote.GetBase());
      std::pair<DWORD_PTR, DWORD> RemoteRet = m_Memory.Call(
        reinterpret_cast<PVOID>(pLoadLibraryWTemp), Args);
      if (!RemoteRet.first)
      {
        std::error_code const LastError = std::error_code(RemoteRet.second, 
          std::system_category());
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Injector::InjectDll") << 
          ErrorString("Call to LoadLibraryW in remote process failed.") << 
          ErrorCode(LastError));
      }

      // Look for target module
      // Note: If creating a module snapshot fails we simply assume injection 
      // succeeded if we've gotten this far. This is because module snapshots 
      // can't be generated for newly created suspended processes, which 
      // CreateAndInjectDll depends on.
      // Todo: Find a better way to do this. (See Injector::InjectDll notes 
      // for the Kernel32 GetModuleHandle for some ideas...)
      Windows::EnsureCloseSnap MySnap(CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE, m_Memory.GetProcessID()));
      if (MySnap != INVALID_HANDLE_VALUE)
      {
        Module NewModule(m_Memory, reinterpret_cast<HMODULE>(RemoteRet.first));
      }

      // Return module base
      return reinterpret_cast<HMODULE>(RemoteRet.first);
    }

    // Call export
    std::pair<DWORD_PTR, DWORD> Injector::CallExport(
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
        ExportArgs);
    }
  }
}
