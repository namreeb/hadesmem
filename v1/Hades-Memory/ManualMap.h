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
#include <fstream>
#include <iterator>
#include <iostream>

// Hades
#include "Memory.h"
#include "Module.h"
#include "Injector.h"

namespace Hades
{
  namespace Memory
  {
    // ManualMap exception type
    class ManualMapError : public virtual HadesMemError 
    { };

    // Manual mapping class
    // Credits to Darawk for the original implementation this is based off
    class ManualMap
    {
    public:
      // Constructor
      inline ManualMap(MemoryMgr const& MyMemory);

      // Manually map DLL
      inline PVOID Map(std::wstring const& Path, std::string const& Export = 
        "", bool InjectHelper = true) const;

    private:
      // Convert RVA to file offset
      inline DWORD_PTR RvaToFileOffset(PIMAGE_NT_HEADERS pNtHeaders, 
        DWORD_PTR Rva) const;

      // Map sections
      inline void MapSections(PIMAGE_NT_HEADERS pNtHeaders, PVOID RemoteAddr, 
        std::vector<BYTE> const& ModBuffer) const;

      // Fix imports
      inline void FixImports(std::vector<BYTE>& ModBuffer, 
        PIMAGE_NT_HEADERS pNtHeaders, PIMAGE_IMPORT_DESCRIPTOR pImpDesc) const;

      // Fix relocations
      inline void FixRelocations(std::vector<BYTE>& ModBuffer, 
        PIMAGE_NT_HEADERS pNtHeaders, PIMAGE_BASE_RELOCATION pRelocDesc, 
        DWORD RelocDirSize, PVOID RemoteBase) const;

      // Disable assignment
      ManualMap& operator= (ManualMap const&);

      // MemoryMgr instance
      MemoryMgr const& m_Memory;
    };

    // Constructor
    ManualMap::ManualMap(MemoryMgr const& MyMemory) 
      : m_Memory(MyMemory)
    { }

    // Manually map DLL
    PVOID ManualMap::Map(std::wstring const& Path, std::string const& Export, 
      bool InjectHelper) const
    {
      // Open file for reading
      std::wcout << "Opening module file for reading." << std::endl;
      std::ifstream ModuleFile(Path.c_str(), std::ios::binary);
      if (!ModuleFile)
      {
        BOOST_THROW_EXCEPTION(ManualMapError() << 
          ErrorFunction("ManualMap::Map") << 
          ErrorString("Could not open module file."));
      }

      // Read file into buffer
      std::wcout << "Reading module file into buffer." << std::endl;
      std::vector<BYTE> ModuleFileBuf((std::istreambuf_iterator<char>(
        ModuleFile)), std::istreambuf_iterator<char>());

      // Ensure file is a valid PE file
      std::wcout << "Performing PE file format validation." << std::endl;
      auto pBase = &ModuleFileBuf[0];
      auto const pDosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(pBase);
      if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
      {
        BOOST_THROW_EXCEPTION(ManualMapError() << 
          ErrorFunction("ManualMap::Map") << 
          ErrorString("Target file is not MyJitFunc valid PE file (DOS)."));
      }
      auto const pNtHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(pBase + 
        pDosHeader->e_lfanew);
      if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE)
      {
        BOOST_THROW_EXCEPTION(ManualMapError() << 
          ErrorFunction("ManualMap::Map") << 
          ErrorString("Target file is not MyJitFunc valid PE file (NT)."));
      }

      // Allocate memory for image
      std::wcout << "Allocating memory for module." << std::endl;
      PVOID const RemoteBase = m_Memory.Alloc(pNtHeaders->OptionalHeader.
        SizeOfImage);
      std::wcout << "Module base address: " << RemoteBase << "." << std::endl;
      std::wcout << "Module size: " << std::hex << pNtHeaders->OptionalHeader.
        SizeOfImage << std::dec << "." << std::endl;

      // Get all TLS callbacks
      std::vector<PIMAGE_TLS_CALLBACK> TlsCallbacks;
      auto const TlsDirSize = pNtHeaders->OptionalHeader.DataDirectory
        [IMAGE_DIRECTORY_ENTRY_TLS].Size;
      if (TlsDirSize)
      {
        auto const pTlsDir = reinterpret_cast<PIMAGE_TLS_DIRECTORY>(pBase + 
          RvaToFileOffset(pNtHeaders, pNtHeaders->OptionalHeader.
          DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress));

        std::wcout << "Enumerating TLS callbacks." << std::endl;
        std::wcout << "Image Base: " << reinterpret_cast<PVOID>(pNtHeaders->
          OptionalHeader.ImageBase) << "." << std::endl;
        std::wcout << "Address Of Callbacks: " << reinterpret_cast<PVOID>(
          pTlsDir->AddressOfCallBacks) << "." << std::endl;

        for (auto pCallbacks = reinterpret_cast<PIMAGE_TLS_CALLBACK*>(pBase + 
          RvaToFileOffset(pNtHeaders, pTlsDir->AddressOfCallBacks - 
          pNtHeaders->OptionalHeader.ImageBase)); *pCallbacks; ++pCallbacks)
        {
          std::wcout << "TLS Callback: " << *pCallbacks << "." << std::endl;
          TlsCallbacks.push_back(reinterpret_cast<PIMAGE_TLS_CALLBACK>(
            reinterpret_cast<DWORD_PTR>(*pCallbacks) - pNtHeaders->
            OptionalHeader.ImageBase));
        }
      }

      // Fix import table if applicable
      auto const ImpDirSize = pNtHeaders->OptionalHeader.DataDirectory
        [IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
      if (ImpDirSize)
      {
        auto const pImpDir = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(pBase + 
          RvaToFileOffset(pNtHeaders, pNtHeaders->OptionalHeader.
          DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress));

        FixImports(ModuleFileBuf, pNtHeaders, pImpDir);
      }

      // Fix relocations if applicable
      auto const RelocDirSize = pNtHeaders->OptionalHeader.DataDirectory
        [IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
      if (RelocDirSize)
      {
        auto const pRelocDir = reinterpret_cast<PIMAGE_BASE_RELOCATION>(pBase + 
          RvaToFileOffset(pNtHeaders, pNtHeaders->OptionalHeader.
          DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress));

        FixRelocations(ModuleFileBuf, pNtHeaders, pRelocDir, RelocDirSize, 
          RemoteBase);
      }

      // Write DOS header to process
      std::wcout << "Writing DOS header." << std::endl;
      std::wcout << "DOS Header: " << pBase << std::endl;
      m_Memory.Write(RemoteBase, *reinterpret_cast<PIMAGE_DOS_HEADER>(pBase));

      // Write PE header to process
      PBYTE const PeHeaderStart = reinterpret_cast<PBYTE>(pNtHeaders);
      PBYTE const PeHeaderEnd = reinterpret_cast<PBYTE>(IMAGE_FIRST_SECTION(
        pNtHeaders));
      std::vector<BYTE> PeHeaderBuf(PeHeaderStart, PeHeaderEnd);
      PBYTE const TargetAddr = static_cast<PBYTE>(RemoteBase) + 
        pDosHeader->e_lfanew;
      std::wcout << "Writing NT header." << std::endl;
      std::wcout << "NT Header: " << TargetAddr << std::endl;
      m_Memory.Write(TargetAddr, PeHeaderBuf);

      // Write sections to process
      MapSections(pNtHeaders, RemoteBase, ModuleFileBuf);

      // Calculate module entry point
      PVOID const EntryPoint = static_cast<PBYTE>(RemoteBase) + pNtHeaders->
        OptionalHeader.AddressOfEntryPoint;
      std::wcout << "Entry Point: " << EntryPoint << "." << std::endl;

      // Get address of export in remote process
      PVOID const ExportAddr = m_Memory.GetRemoteProcAddress(
        reinterpret_cast<HMODULE>(RemoteBase), Path, Export.c_str());
      std::wcout << "Export Address: " << ExportAddr << "." << std::endl;

      // Inject helper module
      if (InjectHelper)
      {
#if defined(_M_AMD64) 
        Map(L"Hades-MMHelper_AMD64.dll", "Initialize", false);
#elif defined(_M_IX86) 
        Map(L"Hades-MMHelper_IA32.dll", "_Initialize@4", false);
#else 
#error "Unsupported architecture."
#endif
      }

      // Call all TLS callbacks
      std::for_each(TlsCallbacks.begin(), TlsCallbacks.end(), 
        [this, RemoteBase] (PIMAGE_TLS_CALLBACK pCallback) 
      {
        std::vector<PVOID> TlsCallArgs;
        TlsCallArgs.push_back(0);
        TlsCallArgs.push_back(reinterpret_cast<PVOID>(DLL_PROCESS_ATTACH));
        TlsCallArgs.push_back(RemoteBase);
        DWORD TlsRet = m_Memory.Call(reinterpret_cast<PBYTE>(RemoteBase) + 
          reinterpret_cast<DWORD_PTR>(pCallback), TlsCallArgs);
        std::wcout << "Tls Callback Returned: " << TlsRet << "." << std::endl;
      });

      // Call entry point
      std::vector<PVOID> EpArgs;
      EpArgs.push_back(0);
      EpArgs.push_back(reinterpret_cast<PVOID>(DLL_PROCESS_ATTACH));
      EpArgs.push_back(RemoteBase);
      DWORD const EpRet = m_Memory.Call(EntryPoint, EpArgs);
      std::wcout << "Entry Point Returned: " << EpRet << "." << std::endl;

      // Call remote export (if specified)
      if (ExportAddr)
      {
        std::vector<PVOID> ExpArgs;
        ExpArgs.push_back(RemoteBase);
        DWORD ExpRet = m_Memory.Call(ExportAddr, ExpArgs);
        std::wcout << "Export Returned: " << ExpRet << "." << std::endl;
      }

      // Return pointer to module in remote process
      return RemoteBase;
    }

    // Convert RVA to file offset
    DWORD_PTR ManualMap::RvaToFileOffset(PIMAGE_NT_HEADERS pNtHeaders, 
      DWORD_PTR Rva) const
    {
      // Get number of sections
      WORD const NumSections = pNtHeaders->FileHeader.NumberOfSections;
      // Get pointer to first section
      PIMAGE_SECTION_HEADER pCurrent = IMAGE_FIRST_SECTION(pNtHeaders);

      // Loop over all sections
      for (WORD i = 0; i < NumSections; ++i, ++pCurrent)
      {
        // Check if the RVA may be inside the current section
        if (pCurrent->VirtualAddress <= Rva)
        {
          // Check if the RVA is inside the current section
          if ((pCurrent->VirtualAddress + pCurrent->Misc.VirtualSize) > Rva)
          {
            // Convert RVA to file (raw) offset
            Rva -= pCurrent->VirtualAddress;
            Rva += pCurrent->PointerToRawData;
            return Rva;
          }
        }
      }

      // Could not perform conversion. Return number signifying an error.
      return static_cast<DWORD>(-1);
    }

    // Map sections
    void ManualMap::MapSections(PIMAGE_NT_HEADERS pNtHeaders, PVOID RemoteAddr, 
      std::vector<BYTE> const& ModBuffer) const
    {
      // Debug output
      std::wcout << "Mapping sections." << std::endl;

      // Get number of sections
      WORD const NumSections = pNtHeaders->FileHeader.NumberOfSections;
      // Get pointer to first section
      PIMAGE_SECTION_HEADER pCurrent = IMAGE_FIRST_SECTION(pNtHeaders);

      // Loop over all sections
      for(WORD i = 0; i != NumSections; ++i, ++pCurrent) 
      {
        // Debug output
        std::string Name(pCurrent->Name, pCurrent->Name + 8);
        std::wcout << "Section Name: " << Name.c_str() << std::endl;

        // Calculate target address for section in remote process
        PVOID const TargetAddr = reinterpret_cast<PBYTE>(RemoteAddr) + 
          pCurrent->VirtualAddress;
        std::wcout << "Target Address: " << TargetAddr << std::endl;

        // Calculate virtual size of section
        DWORD const VirtualSize = pCurrent->Misc.VirtualSize; 
        std::wcout << "Virtual Size: " << TargetAddr << std::endl;

        // Calculate start and end of section data in buffer
        PBYTE const DataStart = const_cast<PBYTE>(&ModBuffer[0]) + 
          pCurrent->PointerToRawData;
        PBYTE const DataEnd = DataStart + pCurrent->SizeOfRawData;

        // Get section data
        std::vector<BYTE> const SectionData(DataStart, DataEnd);

        // Write section data to process
        m_Memory.Write(TargetAddr, SectionData);

        // Set the proper page protections for this section
        DWORD OldProtect;
        if (!VirtualProtectEx(m_Memory.GetProcessHandle(), TargetAddr, 
          VirtualSize, pCurrent->Characteristics & 0x00FFFFFF, &OldProtect))
        {
          DWORD LastError = GetLastError();
          BOOST_THROW_EXCEPTION(ManualMapError() << 
            ErrorFunction("ManualMap::MapSections") << 
            ErrorString("Could not change page protections for section.") << 
            ErrorCodeWin(LastError));
        }
      }
    }

    // Fix imports
    void ManualMap::FixImports(std::vector<BYTE>& ModBuffer, 
      PIMAGE_NT_HEADERS pNtHeaders, PIMAGE_IMPORT_DESCRIPTOR pImpDesc) const
    {
      // Debug output
      std::wcout << "Fixing imports." << std::endl;

      // Loop through all the required modules
      char* ModuleName = nullptr;
      PBYTE ModBase = &ModBuffer[0];
      for (; pImpDesc && pImpDesc->Name; ++pImpDesc) 
      {
        // Get module name
        ModuleName = reinterpret_cast<char*>(ModBase + RvaToFileOffset(
          pNtHeaders, pImpDesc->Name));
        std::string const ModuleNameA(ModuleName);
        auto const ModuleNameW(boost::lexical_cast<std::wstring>(ModuleName));
        auto const ModuleNameLowerW(boost::to_lower_copy(ModuleNameW));
        std::wcout << "Module Name: " << ModuleNameW << "." << std::endl;

        // Check whether dependent module is already loaded
        auto const ModuleList(GetModuleList(m_Memory));
        auto const ModIter = std::find_if(ModuleList.begin(), ModuleList.end(), 
          [&ModuleNameLowerW] (boost::shared_ptr<Module> Current)
        {
          return 
            boost::to_lower_copy(Current->GetName()) == ModuleNameLowerW || 
            boost::to_lower_copy(Current->GetPath()) == ModuleNameLowerW;
        });

        // Module base address and name
        HMODULE CurModBase = 0;
        std::wstring CurModName;

        // If dependent module is not yet loaded then inject it
        if (ModIter == ModuleList.end())
        {
          // Inject dependent DLL
          std::wcout << "Injecting dependent DLL." << std::endl;
          Injector MyInjector(m_Memory);
          CurModBase = MyInjector.InjectDll(ModuleNameW, false);
          CurModName = ModuleNameW;
        }
        else
        {
          CurModBase = (*ModIter)->GetBase();
          CurModName = (*ModIter)->GetName();
        }

        // Lookup the first import thunk for this module
        // Todo: Forwarded import support
        auto pThunkData = reinterpret_cast<PIMAGE_THUNK_DATA>(ModBase + 
          RvaToFileOffset(pNtHeaders, pImpDesc->FirstThunk));
        while(pThunkData->u1.AddressOfData) 
        {
          // Get import data
          auto const pNameImport = reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(
            ModBase + RvaToFileOffset(pNtHeaders, pThunkData->u1.
            AddressOfData));

          // Get name of function
          std::string const ImpName(reinterpret_cast<char*>(pNameImport->Name));
          std::cout << "Function Name: " << ImpName << "." << std::endl;

          // Get function address in remote process
          bool const ByOrdinal = IMAGE_SNAP_BY_ORDINAL(pNameImport->Hint);
          LPCSTR Function = ByOrdinal ? reinterpret_cast<LPCSTR>(IMAGE_ORDINAL(
            pNameImport->Hint)) : reinterpret_cast<LPCSTR>(pNameImport->Name);
          FARPROC FuncAddr = m_Memory.GetRemoteProcAddress(CurModBase, 
            CurModName, Function);

          // Set function address
          pThunkData->u1.Function = reinterpret_cast<DWORD_PTR>(FuncAddr);

          // Advance to next function
          pThunkData++;
        }
      } 
    }

    // Fix relocations
    void ManualMap::FixRelocations(std::vector<BYTE>& ModBuffer, 
      PIMAGE_NT_HEADERS pNtHeaders, PIMAGE_BASE_RELOCATION pRelocDesc, 
      DWORD RelocDirSize, PVOID RemoteBase) const
    {
      // Debug output
      std::wcout << "Fixing relocations." << std::endl;

      // Get image base
      DWORD_PTR const ImageBase = pNtHeaders->OptionalHeader.ImageBase;

      // Calcuate module delta
      DWORD_PTR const Delta = reinterpret_cast<DWORD_PTR>(RemoteBase) - 
        ImageBase;

      // Get local module buffer base
      PBYTE const ModBase = &ModBuffer[0];

      // Number of bytes processed
      DWORD BytesProcessed = 0; 

      // Ensure we don't read into invalid data
      while (BytesProcessed < RelocDirSize)
      {
        // Get base of reloc dir
        PVOID RelocBase = ModBase + RvaToFileOffset(pNtHeaders, 
          pRelocDesc->VirtualAddress);

        // Get number of relocs
        DWORD const NumRelocs = (pRelocDesc->SizeOfBlock - sizeof(
          IMAGE_BASE_RELOCATION)) / sizeof(WORD); 

        // Get pointer to reloc data
        auto pRelocData = reinterpret_cast<WORD*>(reinterpret_cast<DWORD_PTR>(
          pRelocDesc) + sizeof(IMAGE_BASE_RELOCATION));

        // Loop over all relocation entries
        for(DWORD i = 0; i < NumRelocs; ++i, ++pRelocData) 
        {
          // Perform relocation if necessary
          if ((*pRelocData >> 12) & IMAGE_REL_BASED_HIGHLOW)
          {
            *reinterpret_cast<DWORD_PTR*>(static_cast<PBYTE>(RelocBase) + 
              (*pRelocData & 0x0FFF)) += Delta;
          }
        }

        // Add to number of bytes processed
        BytesProcessed += pRelocDesc->SizeOfBlock;

        // Advance to next reloc info block
        pRelocDesc = reinterpret_cast<PIMAGE_BASE_RELOCATION>(pRelocData); 
      }
    }
  }
}
