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

// Windows API
#include <Windows.h>

// C++ Standard Library
#include <vector>

// Boost
#pragma warning(push, 1)
#include <boost/algorithm/string.hpp>
#pragma warning(pop)

// Hades
#include "Module.hpp"
#include "Injector.hpp"
#include "MemoryMgr.hpp"
#include "HadesCommon/Filesystem.hpp"
#include "HadesCommon/EnsureCleanup.hpp"

namespace Hades
{
  namespace Memory
  {
    // Create process (as suspended) and inject DLL
    std::tuple<MemoryMgr, HMODULE, DWORD_PTR> CreateAndInject(
      boost::filesystem::path const& Path, 
      boost::filesystem::path const& WorkDir, 
      std::wstring const& Args, 
      std::wstring const& Module, 
      std::string const& Export)
    {
      // Set up args for CreateProcess
      STARTUPINFO StartInfo = { sizeof(StartInfo) };
      PROCESS_INFORMATION ProcInfo = { 0 };

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
      if (!CreateProcess(Path.c_str(), &ProcArgs[0], nullptr, nullptr, FALSE, 
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
        MemoryMgr MyMemory(ProcInfo.dwProcessId);

        // Create DLL injector
        Hades::Memory::Injector const MyInjector(MyMemory);

        // Inject DLL
        HMODULE const ModBase = MyInjector.InjectDll(Module);

        // If export has been specified
        DWORD_PTR const ExportRet = Export.empty() ? 0 : MyInjector.CallExport(
          Module, ModBase, Export);

        // Success! Let the process continue execution.
        if (ResumeThread(ProcInfo.hThread) == static_cast<DWORD>(-1))
        {
          std::error_code const LastError = GetLastErrorCode();
          BOOST_THROW_EXCEPTION(Injector::Error() << 
            ErrorFunction("CreateAndInject") << 
            ErrorString("Could not resume process.") << 
            ErrorCode(LastError));
        }

        // Return data to caller
        return std::make_tuple(MyMemory, ModBase, ExportRet);
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
    // Fixme: Perform necessary adjustments to module base if necessary.
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
      std::wstring const PathStringTemp(PathReal.native());
      std::wstring const PathString(boost::to_lower_copy(PathStringTemp));

      // Calculate the number of bytes needed for the DLL's pathname
      std::size_t const PathBufSize = (PathString.size() + 1) * 
        sizeof(wchar_t);

      // Allocate space in the remote process for the pathname
      AllocAndFree const LibFileRemote(m_Memory, PathBufSize);
      if (!LibFileRemote.GetAddress())
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Injector::InjectDll") << 
          ErrorString("Could not allocate memory.") << 
          ErrorCode(LastError));
      }

      // Copy the DLL's pathname to the remote process' address space
      m_Memory.Write(LibFileRemote.GetAddress(), PathString);

      // Get address of LoadLibraryW in Kernel32.dll
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
      DWORD_PTR pLoadLibraryWTemp = reinterpret_cast<DWORD_PTR>(pLoadLibraryW);

      // Load module in remote process using LoadLibraryW
      std::vector<PVOID> Args;
      Args.push_back(LibFileRemote.GetAddress());
      if (!m_Memory.Call(reinterpret_cast<PVOID>(pLoadLibraryWTemp), Args))
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Injector::InjectDll") << 
          ErrorString("Call to LoadLibraryW in remote process failed."));
      }

      // Look for target module
      boost::optional<Module> MyModule;
      for (ModuleListIter MyIter(m_Memory); *MyIter; ++MyIter)
      {
        if (PathResolution)
        {
          if (boost::filesystem::equivalent((*MyIter)->GetPath(), PathReal))
          {
            MyModule = *MyIter;
          }
        }
        else
        {
          if (boost::to_lower_copy((*MyIter)->GetName()) == PathString || 
            boost::to_lower_copy((*MyIter)->GetPath()) == PathString)
          {
            MyModule = *MyIter;
          }
        }
      }

      // Ensure target module was found
      if (!MyModule)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Injector::InjectDll") << 
          ErrorString("Could not find module in remote process."));
      }

      // Return module base
      return MyModule->GetBase();
    }

    // Call export
    DWORD_PTR Injector::CallExport(boost::filesystem::path const& ModulePath, 
      HMODULE ModuleRemote, std::string const& Export) const
    {
      // Get export address
      FARPROC const pExportAddr = m_Memory.GetRemoteProcAddress(ModuleRemote, 
        ModulePath, Export);
      if (!pExportAddr)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Injector::InjectDll") << 
          ErrorString("Could not find export in remote module."));
      }
      DWORD_PTR pExportAddrTemp = reinterpret_cast<DWORD_PTR>(pExportAddr);

      // Create a remote thread that calls the desired export
      std::vector<PVOID> ExportArgs;
      ExportArgs.push_back(ModuleRemote);
      return m_Memory.Call(reinterpret_cast<PVOID>(pExportAddrTemp), 
        ExportArgs);
    }
  }
}
