/*
This file is part of HadesMem.
Copyright © 2010 Cypherjb (aka Chazwazza, aka Cypher). 
<http://www.cypherjb.com/> <cypher.jb@gmail.com>

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

// Windows API
#include <Windows.h>

// C++ Standard Library
#include <memory>
#include <string>
#include <vector>

// Boost
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

// Hades
#include "Module.h"
#include "Memory.h"

// Image base linker 'trick'
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

namespace Hades
{
  namespace Memory
  {
    // Injector exception type
    class InjectorError : public virtual HadesMemError 
    { };

    // DLL injection class
    class Injector
    {
    public:
      // Constructor
      inline Injector(MemoryMgr const& MyMemory);

      // Inject DLL
      inline HMODULE InjectDll(std::wstring const& Path, 
        bool PathResolution = true) const;

      // Call export
      inline DWORD CallExport(std::wstring const& ModulePath, 
        HMODULE ModuleRemote, std::string const& Export) const;

    private:
      // Disable assignment
      Injector& operator= (Injector const&);

      // MemoryMgr instance
      MemoryMgr const& m_Memory;
    };

    // Create process (as suspended) and inject DLL
    inline boost::shared_ptr<MemoryMgr> CreateAndInject(
      std::wstring const& Path, 
      std::wstring const& Args, 
      std::wstring const& Module, 
      std::string const& Export, 
      HMODULE* ModBaseOut = nullptr, 
      DWORD* ExportRetOut = nullptr)
    {
      // Set up args for CreateProcess
      STARTUPINFO StartInfo = { sizeof(StartInfo) };
      PROCESS_INFORMATION ProcInfo = { 0 };

      // Construct command line.
      std::wstring CommandLine(L"\"" + Path + L"\" " + Args);
      // Copy command line to buffer
      std::vector<wchar_t> ProcArgs(CommandLine.begin(), CommandLine.end());
      ProcArgs.push_back(L'\0');

      // Attempt process creation
      if (!CreateProcess(Path.c_str(), &ProcArgs[0], NULL, NULL, FALSE, 
        CREATE_SUSPENDED, NULL, NULL, &StartInfo, &ProcInfo))
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(InjectorError() << 
          ErrorFunction("CreateAndInject") << 
          ErrorString("Could not create process.") << 
          ErrorCodeWin(LastError));
      }

      // Ensure cleanup
      Windows::EnsureCloseHandle ProcHandle(ProcInfo.hProcess);
      Windows::EnsureCloseHandle ThreadHandle(ProcInfo.hThread);

      // Memory manager instance
      boost::shared_ptr<MemoryMgr> MyMemory;

      try
      {
        // Create memory manager
        MyMemory.reset(new Hades::Memory::MemoryMgr(ProcInfo.dwProcessId));

        // Create DLL injector
        Hades::Memory::Injector MyInjector(*MyMemory);

        // Inject DLL
        HMODULE ModBase = MyInjector.InjectDll(Module);
        if (ModBaseOut)
        {
          *ModBaseOut = ModBase;
        }

        // If export has been specified
        if (!Export.empty())
        {
          // Call remote export
          DWORD ExportRet = MyInjector.CallExport(Module, ModBase, Export);
          if (ExportRetOut)
          {
            *ExportRetOut = ExportRet;
          }
        }
      }
      // Catch exceptions
      catch (std::exception const& /*e*/)
      {
        // Terminate process if injection failed
        TerminateProcess(ProcInfo.hProcess, 0);
        // Rethrow exception
        throw;
      }

      // Success! Let the process continue execution.
      ResumeThread(ProcInfo.hThread);

      // Return memory manager instance for use in further calls
      return MyMemory;
    }

    // Constructor
    Injector::Injector(MemoryMgr const& MyMemory) 
      : m_Memory(MyMemory)
    { }

    // Inject DLL
    HMODULE Injector::InjectDll(std::wstring const& Path, bool PathResolution) 
      const
    {
      // String to hold 'real' path to module
      std::wstring PathReal(Path);

      // Check whether we need to convert the path from a relative to 
      // an absolute
      if (PathResolution && !boost::filesystem::wpath(Path).
        has_root_directory())
      {
        // Get handle to self
        HMODULE const Self = reinterpret_cast<HMODULE>(&__ImageBase);

        // Get path to self
        std::vector<wchar_t> SelfPath(MAX_PATH);
        if (!GetModuleFileName(Self, &SelfPath[0], MAX_PATH) || 
          GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
          DWORD LastError = GetLastError();
          BOOST_THROW_EXCEPTION(InjectorError() << 
            ErrorFunction("Injector::InjectDll") << 
            ErrorString("Could not get path to self.") << 
            ErrorCodeWin(LastError));
        }

        // Convert relative path to absolute path
        PathReal = boost::filesystem::complete(Path, boost::filesystem::wpath(
          &SelfPath[0]).parent_path()).file_string();
      }

      // Convert path to lower case
      boost::to_lower(PathReal);

      // Ensure target file exists
      if (PathResolution && !boost::filesystem::exists(PathReal))
      {
        BOOST_THROW_EXCEPTION(InjectorError() << 
          ErrorFunction("Injector::InjectDll") << 
          ErrorString("Could not find module file."));
      }

      // Calculate the number of bytes needed for the DLL's pathname
      size_t const PathBufSize  = (Path.length() + 1) * sizeof(wchar_t);

      // Allocate space in the remote process for the pathname
      AllocAndFree const LibFileRemote(m_Memory, PathBufSize);
      if (!LibFileRemote.GetAddress())
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(InjectorError() << 
          ErrorFunction("Injector::InjectDll") << 
          ErrorString("Could not allocate memory.") << 
          ErrorCodeWin(LastError));
      }

      // Copy the DLL's pathname to the remote process' address space
      m_Memory.Write(LibFileRemote.GetAddress(), PathReal);

      // Get the real address of LoadLibraryW in Kernel32.dll
      HMODULE const hKernel32 = GetModuleHandleW(L"Kernel32");
      if (!hKernel32)
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(InjectorError() << 
          ErrorFunction("Injector::InjectDll") << 
          ErrorString("Could not get handle to Kernel32.") << 
          ErrorCodeWin(LastError));
      }
      FARPROC const pLoadLibraryW = GetProcAddress(hKernel32, "LoadLibraryW");
      if (!pLoadLibraryW)
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(InjectorError() << 
          ErrorFunction("Injector::InjectDll") << 
          ErrorString("Could not get pointer to LoadLibraryW.") << 
          ErrorCodeWin(LastError));
      }

      // Load module in remote process using LoadLibraryW
      std::vector<PVOID> Args;
      Args.push_back(LibFileRemote.GetAddress());
      if (!m_Memory.Call(pLoadLibraryW, Args))
      {
        BOOST_THROW_EXCEPTION(InjectorError() << 
          ErrorFunction("Injector::InjectDll") << 
          ErrorString("Call to LoadLibraryW in remote process failed."));
      }

      // Get module list for remote process
      auto const ModuleList(GetModuleList(m_Memory));
      // Look for target module
      auto const Iter = std::find_if(ModuleList.begin(), ModuleList.end(), 
        [&PathReal] (boost::shared_ptr<Module> const& MyModule) 
      {
        return boost::to_lower_copy(MyModule->GetName()) == PathReal || 
          boost::to_lower_copy(MyModule->GetPath()) == PathReal;
      });
      // Ensure target module was found
      if (Iter == ModuleList.end())
      {
        BOOST_THROW_EXCEPTION(InjectorError() << 
          ErrorFunction("Injector::InjectDll") << 
          ErrorString("Could not find module in remote process."));
      }

      // Return module base
      return (*Iter)->GetBase();
    }

    // Call export
    DWORD Injector::CallExport(std::wstring const& ModulePath, 
      HMODULE ModuleRemote, std::string const& Export) const
    {
      // Load module as data so we can read the EAT locally
      Windows::EnsureFreeLibrary const MyModule(LoadLibraryExW(
        ModulePath.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES));
      if (!MyModule)
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(InjectorError() << 
          ErrorFunction("Injector::CallExport") << 
          ErrorString("Could not load module locally.") << 
          ErrorCodeWin(LastError));
      }

      // Find export
      PVOID pExportAddr = GetProcAddress(MyModule, Export.c_str());

      // Nothing found, throw exception
      if (!pExportAddr)
      {
        BOOST_THROW_EXCEPTION(InjectorError() << 
          ErrorFunction("Injector::CallExport") << 
          ErrorString("Could not find export."));
      }

      // If image is relocated we need to recalculate the address
      if (MyModule != ModuleRemote)
      {
        // Get local module pointer
        PBYTE const ModuleLocal = static_cast<PBYTE>(static_cast<PVOID>(
          MyModule));

        // Calculate new address
        pExportAddr = reinterpret_cast<PBYTE>(ModuleRemote) + 
          (static_cast<PBYTE>(pExportAddr) - ModuleLocal);
      }

      // Create a remote thread that calls the desired export
      std::vector<PVOID> ExportArgs;
      ExportArgs.push_back(ModuleRemote);
      return m_Memory.Call(pExportAddr, ExportArgs);
    }
  }
}
