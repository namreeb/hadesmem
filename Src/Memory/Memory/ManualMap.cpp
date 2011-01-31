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

// Note: This file contains some code from the ReactOS project.
// Fixme: Find and tag all such code.

// C++ Standard Library
#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <iterator>
#include <iostream>

// Boost
#ifdef _MSC_VER
#pragma warning(push, 1)
#endif // #ifdef _MSC_VER
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/fstream.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif // #ifdef _MSC_VER

// Hades
#include "PeLib.hpp"
#include "Module.hpp"
#include "Injector.hpp"
#include "MemoryMgr.hpp"
#include "ManualMap.hpp"
#include "HadesCommon/I18n.hpp"
#include "HadesCommon/EnsureCleanup.hpp"

namespace 
{
  std::array<ULONG, 16> const SectionCharacteristicsToProtect = 
  {{
    PAGE_NOACCESS,          /* 0 = NONE */
    PAGE_NOACCESS,          /* 1 = SHARED */
    PAGE_EXECUTE,           /* 2 = EXECUTABLE */
    PAGE_EXECUTE,           /* 3 = EXECUTABLE, SHARED */
    PAGE_READONLY,          /* 4 = READABLE */
    PAGE_READONLY,          /* 5 = READABLE, SHARED */
    PAGE_EXECUTE_READ,      /* 6 = READABLE, EXECUTABLE */
    PAGE_EXECUTE_READ,      /* 7 = READABLE, EXECUTABLE, SHARED */
    PAGE_READWRITE,         /* 8 = WRITABLE */
    PAGE_READWRITE,         /* 9 = WRITABLE, SHARED */
    PAGE_EXECUTE_READWRITE, /* 10 = WRITABLE, EXECUTABLE */
    PAGE_EXECUTE_READWRITE, /* 11 = WRITABLE, EXECUTABLE, SHARED */
    PAGE_READWRITE,         /* 12 = WRITABLE, READABLE */
    PAGE_READWRITE,         /* 13 = WRITABLE, READABLE, SHARED */
    PAGE_EXECUTE_READWRITE, /* 14 = WRITABLE, READABLE, EXECUTABLE */
    PAGE_EXECUTE_READWRITE, /* 15 = WRITABLE, READABLE, EXECUTABLE, SHARED */
  }};
}

namespace Hades
{
  namespace Memory
  {
    // Constructor
    ManualMap::ManualMap(MemoryMgr const& MyMemory) 
      : m_Memory(MyMemory)
    { }

    // Manually map DLL
    PVOID ManualMap::Map(boost::filesystem::path const& Path, 
      std::string const& Export, bool InjectHelper) const
    {
      // Do not continue if Shim Engine is enabled for local process, 
      // otherwise it could interfere with the address resolution.
      HMODULE const ShimEngMod = GetModuleHandle(L"ShimEng.dll");
      if (ShimEngMod)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("ManualMap::Map") << 
          ErrorString("Shims enabled for local process."));
      }
      
      // Open file for reading
      boost::filesystem::basic_ifstream<BYTE> ModuleFile(Path, 
        std::ios::binary | std::ios::ate);
      if (!ModuleFile)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("ManualMap::Map") << 
          ErrorString("Could not open image file."));
      }

      // Get file size
      std::streamsize const FileSize = ModuleFile.tellg();

      // Allocate memory to hold file data
      // Doing this rather than copying data into a vector to avoid having to 
      // play with the page protection flags on the heap.
      PBYTE const pBase = static_cast<PBYTE>(VirtualAlloc(nullptr, 
        static_cast<SIZE_T>(FileSize), MEM_COMMIT | MEM_RESERVE, 
        PAGE_READWRITE));
      if (!pBase)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("ManualMap::Map") << 
          ErrorString("Could not allocate memory for image data.") << 
          ErrorCode(LastError));
      }
      Windows::EnsureReleaseRegion const EnsureFreeLocalMod(pBase);

      // Seek to beginning of file
      ModuleFile.seekg(0, std::ios::beg);

      // Read file into memory
      ModuleFile.read(pBase, FileSize);

      // Create memory manager for local proc
      MemoryMgr MyMemoryLocal(GetCurrentProcessId());

      // Ensure file is a valid PE file
      std::cout << "Performing PE file format validation." << std::endl;
      PeFile MyPeFile(MyMemoryLocal, pBase, PeFile::FileType_Data);
      DosHeader const MyDosHeader(MyPeFile);
      NtHeaders const MyNtHeaders(MyPeFile);

      // Allocate memory for image
      std::cout << "Allocating remote memory for image." << std::endl;
      PVOID const RemoteBase = m_Memory.Alloc(MyNtHeaders.GetSizeOfImage());
      std::cout << "Image base address: " << RemoteBase << "." << std::endl;
      std::cout << "Image size: " << std::hex << MyNtHeaders.GetSizeOfImage() 
        << std::dec << "." << std::endl;

      // Get all TLS callbacks
      std::vector<PIMAGE_TLS_CALLBACK> TlsCallbacks;
      TlsDir const MyTlsDir(MyPeFile);
      if (MyTlsDir.IsValid())
      {
        TlsCallbacks = MyTlsDir.GetCallbacks();
        std::for_each(TlsCallbacks.cbegin(), TlsCallbacks.cend(), 
          [&] (PIMAGE_TLS_CALLBACK pCurrent)
        {
          std::cout << "TLS Callback: " << pCurrent << std::endl;
        });
      }

      // Process import table
      FixImports(MyPeFile);

      // Process relocations
      FixRelocations(MyPeFile, RemoteBase);

      // Write DOS header to process
      std::cout << "Writing DOS header." << std::endl;
      std::cout << "DOS Header: " << RemoteBase << std::endl;
      m_Memory.Write(RemoteBase, *reinterpret_cast<PIMAGE_DOS_HEADER>(
        pBase));

      // Write NT headers to process
      PBYTE const NtHeadersStart = reinterpret_cast<PBYTE>(MyNtHeaders.
        GetBase());
      PBYTE const NtHeadersEnd = Section(MyPeFile, 0).GetBase();
      std::vector<BYTE> const PeHeaderBuf(NtHeadersStart, NtHeadersEnd);
      PBYTE const TargetAddr = static_cast<PBYTE>(RemoteBase) + MyDosHeader.
        GetNewHeaderOffset();
      std::cout << "Writing NT header." << std::endl;
      std::cout << "NT Header: " << static_cast<PVOID>(TargetAddr) << 
        std::endl;
      m_Memory.Write(TargetAddr, PeHeaderBuf);

      // Write sections to process
      MapSections(MyPeFile, RemoteBase);

      // Calculate module entry point
      PVOID const EntryPoint = static_cast<PBYTE>(RemoteBase) + 
        MyNtHeaders.GetAddressOfEntryPoint();
      std::cout << "Entry Point: " << EntryPoint << "." << std::endl;

      // Get address of export in remote process
      PVOID const ExportAddr = reinterpret_cast<PVOID>(
        m_Memory.GetRemoteProcAddress(reinterpret_cast<HMODULE>(RemoteBase), 
        Path, Export.c_str()));
      std::cout << "Export Address: " << ExportAddr << "." << std::endl;

      // Inject helper module
      if (InjectHelper)
      {
#if defined(_M_AMD64) 
        Map(L"MMHelper.dll", "Initialize", false);
#elif defined(_M_IX86) 
        Map(L"MMHelper.dll", "_Initialize@4", false);
#else 
#error "[HadesMem] Unsupported architecture."
#endif
      }

      // Call all TLS callbacks
      std::for_each(TlsCallbacks.cbegin(), TlsCallbacks.cend(), 
        [&] (PIMAGE_TLS_CALLBACK pCallback) 
      {
        std::vector<PVOID> TlsCallArgs;
        TlsCallArgs.push_back(0);
        TlsCallArgs.push_back(reinterpret_cast<PVOID>(DLL_PROCESS_ATTACH));
        TlsCallArgs.push_back(RemoteBase);
        DWORD_PTR const TlsRet = m_Memory.Call(reinterpret_cast<PBYTE>(
          RemoteBase) + reinterpret_cast<DWORD_PTR>(pCallback), TlsCallArgs);
        std::cout << "TLS Callback Returned: " << TlsRet << "." << std::endl;
      });

      // Call entry point
      std::vector<PVOID> EpArgs;
      EpArgs.push_back(0);
      EpArgs.push_back(reinterpret_cast<PVOID>(DLL_PROCESS_ATTACH));
      EpArgs.push_back(RemoteBase);
      DWORD_PTR const EpRet = m_Memory.Call(EntryPoint, EpArgs);
      std::cout << "Entry Point Returned: " << EpRet << "." << std::endl;

      // Call remote export (if specified)
      if (ExportAddr)
      {
        std::vector<PVOID> ExpArgs;
        ExpArgs.push_back(RemoteBase);
        DWORD_PTR const ExpRet = m_Memory.Call(ExportAddr, ExpArgs);
        std::cout << "Export Returned: " << ExpRet << "." << std::endl;
      }

      // Return pointer to module in remote process
      return RemoteBase;
    }

    // Map sections
    void ManualMap::MapSections(PeFile& MyPeFile, PVOID RemoteAddr) const
    {
      // Debug output
      std::cout << "Mapping sections." << std::endl;

      // Enumerate all sections
      for (SectionIter i(MyPeFile); *i; ++i)
      {
        // Get section
        Section& Current = **i;

        // Get section name
        std::string const Name(Current.GetName());
        std::cout << "Section Name: " << Name.c_str() << std::endl;

        // Calculate target address for section in remote process
        PVOID const TargetAddr = reinterpret_cast<PBYTE>(RemoteAddr) + 
          Current.GetVirtualAddress();
        std::cout << "Target Address: " << TargetAddr << std::endl;

        // Calculate virtual size of section
        DWORD const VirtualSize = Current.GetVirtualSize(); 
        std::cout << "Virtual Size: " << std::hex << VirtualSize << std::dec 
          << std::endl;

        // Calculate start and end of section data in buffer
        DWORD const SizeOfRawData = Current.GetSizeOfRawData();
        PBYTE const DataStart = MyPeFile.GetBase() + Current.
          GetPointerToRawData();
        PBYTE const DataEnd = DataStart + SizeOfRawData;

        // Get section data
        std::vector<BYTE> const SectionData(DataStart, DataEnd);

        // Write section data to process
        if (!SectionData.empty())
        {
          m_Memory.Write(TargetAddr, SectionData);
        }

        // Get section characteristics
        DWORD SecCharacteristics = Current.GetCharacteristics();

        // Handle case where no explicit protection is provided. Infer 
        // protection flags from section type.
        if((SecCharacteristics & (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | 
          IMAGE_SCN_MEM_WRITE)) == 0)
        {
          if(SecCharacteristics & IMAGE_SCN_CNT_CODE)
          {
            SecCharacteristics |= IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;
          }

          if(SecCharacteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)
          {
            SecCharacteristics |= IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
          }

          if(SecCharacteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA)
          {
            SecCharacteristics |= IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
          }
        }

        // Look up protection flags for section
        DWORD SecProtect = SectionCharacteristicsToProtect[
          SecCharacteristics >> 28];

        // Set the proper page protections for this section
        DWORD OldProtect;
        if (!VirtualProtectEx(m_Memory.GetProcessHandle(), TargetAddr, 
          VirtualSize, SecProtect, &OldProtect))
        {
          std::error_code const LastError = GetLastErrorCode();
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("ManualMap::MapSections") << 
            ErrorString("Could not change page protections for section.") << 
            ErrorCode(LastError));
        }
      }
    }

    // Fix imports
    void ManualMap::FixImports(PeFile& MyPeFile) const
    {
      // Get NT headers
      NtHeaders const MyNtHeaders(MyPeFile);

      // Get import dir
      ImportDir const CheckImpDir(MyPeFile);
      if (!CheckImpDir.IsValid())
      {
        // Debug output
        std::cout << "Image has no imports." << std::endl;

        // Nothing more to do
        return;
      }

      // Debug output
      std::cout << "Fixing imports." << std::endl;

      // Loop through all the required modules
      for (ImportDirIter i(MyPeFile); *i; ++i)
      {
        // Get import dir
        ImportDir& MyImportDir = **i;

        // Check for forwarded imports
        // Fixme: Handle forwarded imports
        if (MyImportDir.GetTimeDateStamp())
        {
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("ManualMap::FixImports") << 
            ErrorString("Image has unhandled forwarded imports."));
        }

        // Get module name
        std::string const ModuleName(MyImportDir.GetName());
        std::wstring const ModuleNameW(boost::lexical_cast<std::wstring>(
          ModuleName));
        std::wstring const ModuleNameLowerW(boost::to_lower_copy(
          ModuleNameW));
        std::cout << "Module Name: " << ModuleName << "." << std::endl;
          
        // Check whether dependent module is already loaded
        boost::optional<Module> MyModule;
        for (ModuleListIter j(m_Memory); *j; ++j)
        {
          Module& Current = **j;
          if (boost::to_lower_copy(Current.GetName()) == ModuleNameLowerW || 
            boost::to_lower_copy(Current.GetPath()) == ModuleNameLowerW)
          {
            // Fixme: Support multiple modules with the same name in different 
            // paths.
            if (MyModule)
            {
              std::cout << "WARNING! Found two modules with the same name. You "
                "may experience unexpected behaviour." << std::endl;
            }
            
            MyModule = *j;
          }
        }

        // Module base address, name, and path
        HMODULE CurModBase = 0;
        std::wstring CurModName;
        std::wstring CurModPath;

        // If dependent module is not yet loaded then inject it
        if (!MyModule)
        {
          // Inject dependent DLL
          std::cout << "Injecting dependent DLL." << std::endl;
          Injector const MyInjector(m_Memory);
          CurModBase = MyInjector.InjectDll(ModuleName, false);
          CurModName = ModuleNameW;
          
          // Search again for dependent DLL and set module path
          for (ModuleListIter j(m_Memory); *j; ++j)
          {
            Module const& Current = **j;
            if (boost::to_lower_copy(Current.GetName()) == ModuleNameLowerW || 
              boost::to_lower_copy(Current.GetPath()) == ModuleNameLowerW)
            {
              // Fixme: Support multiple modules with the same name in different 
              // paths.
              if (!CurModPath.empty())
              {
                std::cout << "WARNING! Found two modules with the same name. "
                  "You may experience unexpected behaviour." << std::endl;
              }
              
              CurModPath = Current.GetPath();
            }
          }
        }
        else
        {
          CurModBase = MyModule->GetBase();
          CurModName = MyModule->GetName();
          CurModPath = MyModule->GetPath();
        }

        // Loop over import thunks for current module
        for (ImportThunkIter j(MyPeFile, MyImportDir.GetFirstThunk()); *j; ++j)
        {
          // Get import thunk
          ImportThunk& ImpThunk = **j;

          // Get function address in remote process
          FARPROC FuncAddr = 0;
          if (ImpThunk.ByOrdinal())
          {
            FuncAddr = m_Memory.GetRemoteProcAddress(CurModBase, 
              CurModPath.empty() ? CurModName : CurModPath, 
              ImpThunk.GetOrdinal());
          }
          else
          {
            // Get name of function
            std::string const ImpName(ImpThunk.GetName());
            std::cout << "Function Name: " << ImpName << "." << std::endl;

            FuncAddr = m_Memory.GetRemoteProcAddress(CurModBase, 
              CurModPath.empty() ? CurModName : CurModPath, 
              ImpThunk.GetName());
          }

          // Ensure function was found
          if (!FuncAddr)
          {
            BOOST_THROW_EXCEPTION(Error() << 
              ErrorFunction("ManualMap::FixImports") << 
              ErrorString("Could not find current import."));
          }

          // Set function address
          ImpThunk.SetFunction(reinterpret_cast<DWORD_PTR>(FuncAddr));
        }
      } 
    }

    // Fix relocations
    void ManualMap::FixRelocations(PeFile& MyPeFile, PVOID pRemoteBase) const
    {
      // Get NT headers
      NtHeaders const MyNtHeaders(MyPeFile);

      // Get import data dir size and address
      DWORD const RelocDirSize = MyNtHeaders.GetDataDirectorySize(NtHeaders::
        DataDir_BaseReloc);
      PIMAGE_BASE_RELOCATION pRelocDir = 
        static_cast<PIMAGE_BASE_RELOCATION>(MyPeFile.RvaToVa(MyNtHeaders.
        GetDataDirectoryVirtualAddress(NtHeaders::DataDir_BaseReloc)));
      if (!RelocDirSize || !pRelocDir)
      {
        // Debug output
        std::cout << "Image has no relocations." << std::endl;

        // Nothing more to do
        return;
      }

      // Get end of reloc dir
      PVOID pRelocDirEnd = reinterpret_cast<PBYTE>(pRelocDir) + RelocDirSize;

      // Debug output
      std::cout << "Fixing relocations." << std::endl;

      // Get image base
      ULONG_PTR const ImageBase = MyNtHeaders.GetImageBase();

      // Calcuate module delta
      LONG_PTR const Delta = reinterpret_cast<ULONG_PTR>(pRemoteBase) - 
        ImageBase;

      // Ensure we don't read into invalid data
      while (pRelocDir < pRelocDirEnd && pRelocDir->SizeOfBlock > 0)
      {
        // Get base of reloc dir
        PBYTE const RelocBase = static_cast<PBYTE>(MyPeFile.RvaToVa(
          pRelocDir->VirtualAddress));

        // Get number of relocs
        DWORD const NumRelocs = (pRelocDir->SizeOfBlock - sizeof(
          IMAGE_BASE_RELOCATION)) / sizeof(WORD); 

        // Get pointer to reloc data
        PWORD pRelocData = reinterpret_cast<PWORD>(pRelocDir + 1);

        // Loop over all relocation entries
        for(DWORD i = 0; i < NumRelocs; ++i, ++pRelocData) 
        {
          // Get reloc data
          BYTE RelocType = *pRelocData >> 12;
          WORD Offset = *pRelocData & 0xFFF;

          // Process reloc
          switch (RelocType)
          {
          case IMAGE_REL_BASED_ABSOLUTE:
            break;

          case IMAGE_REL_BASED_HIGHLOW:
            *reinterpret_cast<DWORD32*>(RelocBase + Offset) += 
              static_cast<DWORD32>(Delta);
            break;

          case IMAGE_REL_BASED_DIR64:
            *reinterpret_cast<DWORD64*>(RelocBase + Offset) += Delta;
            break;

          default:
            std::cout << "Unsupported relocation type: " << RelocType << 
              std::endl;

            BOOST_THROW_EXCEPTION(Error() << 
              ErrorFunction("ManualMap::FixRelocations") << 
              ErrorString("Unsuppported relocation type."));
          }
        }

        // Advance to next reloc info block
        pRelocDir = reinterpret_cast<PIMAGE_BASE_RELOCATION>(pRelocData); 
      }
    }
  }
}
