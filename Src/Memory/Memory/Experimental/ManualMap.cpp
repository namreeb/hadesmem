// Copyright Joshua Boyce 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include <HadesMemory/Experimental/ManualMap.hpp>
#include <HadesMemory/Module.hpp>
#include <HadesMemory/MemoryMgr.hpp>
#include <HadesMemory/PeLib/PeLib.hpp>
#include <HadesMemory/Detail/I18n.hpp>
#include <HadesMemory/Detail/WinAux.hpp>
#include <HadesMemory/Detail/EnsureCleanup.hpp>

#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <iterator>
#include <iostream>

#include <boost/optional.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/fstream.hpp>

namespace HadesMem
{
  // Constructor
  ManualMap::ManualMap(MemoryMgr const& MyMemory) 
    : m_Memory(MyMemory)
  { }
      
  // Copy constructor
  ManualMap::ManualMap(ManualMap const& Other)
    : m_Memory(Other.m_Memory)
  { }
  
  // Copy assignment operator
  ManualMap& ManualMap::operator=(ManualMap const& Other)
  {
    this->m_Memory = Other.m_Memory;
    
    return *this;
  }
  
  // Move constructor
  ManualMap::ManualMap(ManualMap&& Other)
    : m_Memory(std::move(Other.m_Memory))
  { }
  
  // Move assignment operator
  ManualMap& ManualMap::operator=(ManualMap&& Other)
  {
    this->m_Memory = std::move(Other.m_Memory);
    
    return *this;
  }
  
  // Destructor
  ManualMap::~ManualMap()
  { }

  // Manually map DLL
  HMODULE ManualMap::InjectDll(std::wstring const& Path, 
      std::string const& Export, 
      InjectFlags Flags) const
  {
    std::wcout << Path << " - InjectDll called." << std::endl;
    
    // Do not continue if Shim Engine is enabled for local process, 
    // otherwise it could interfere with the address resolution.
    HMODULE const ShimEngMod = GetModuleHandle(L"ShimEng.dll");
    if (ShimEngMod)
    {
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorFunction("ManualMap::Map") << 
        ErrorString("Shims enabled for local process."));
    }
      
    bool PathResolution = ((Flags & InjectFlag_PathResolution) == 
      InjectFlag_PathResolution);
    
    std::wcout << Path << " - Path resolution flag: " << PathResolution 
      << "." << std::endl;
    
    boost::filesystem::path PathReal(ResolvePath(Path, PathResolution));
    
    std::wcout << Path << " - Reading file." << std::endl;
    
    boost::filesystem::basic_ifstream<char> ModuleFile(PathReal, 
      std::ios::binary | std::ios::ate);
    if (!ModuleFile)
    {
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorFunction("ManualMap::Map") << 
        ErrorString("Could not open image file."));
    }
    
    std::streamsize const FileSize = ModuleFile.tellg();

    // Allocate memory to hold file data
    // Doing this rather than copying data into a vector to avoid having to 
    // play with the page protection flags on the heap.
    char* const pBase = static_cast<char*>(VirtualAlloc(nullptr, 
      static_cast<SIZE_T>(FileSize), MEM_COMMIT | MEM_RESERVE, 
      PAGE_READWRITE));
    if (!pBase)
    {
      DWORD const LastError = GetLastError();
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorFunction("ManualMap::Map") << 
        ErrorString("Could not allocate memory for image data.") << 
        ErrorCodeWinLast(LastError));
    }
    Detail::EnsureReleaseRegion const EnsureFreeLocalMod(pBase);
    
    if (!ModuleFile.seekg(0, std::ios::beg))
    {
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorFunction("ManualMap::Map") << 
        ErrorString("Could not seek to beginning of file."));
    }
    
    if (!ModuleFile.read(pBase, FileSize))
    {
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorFunction("ManualMap::Map") << 
        ErrorString("Could not read file into memory."));
    }
    
    MemoryMgr MyMemoryLocal(GetCurrentProcessId());
    
    std::wcout << Path << " - Performing PE file format validation." 
      << std::endl;
    PeFile MyPeFile(MyMemoryLocal, pBase, PeFile::FileType_Data);
    DosHeader const MyDosHeader(MyPeFile);
    NtHeaders const MyNtHeaders(MyPeFile);
    
    std::wcout << Path << " - Allocating remote memory for image." 
      << std::endl;
    PVOID const RemoteBase = m_Memory.Alloc(MyNtHeaders.GetSizeOfImage());
    std::wcout << Path << " - Image base address: " << RemoteBase << "." 
      << std::endl;
    std::wcout << Path << " - Image size: " << std::hex << 
      MyNtHeaders.GetSizeOfImage() << std::dec << "." << std::endl;
    
    m_MappedMods[boost::to_lower_copy(PathReal.native())] = 
      reinterpret_cast<HMODULE>(RemoteBase);
    
    std::vector<PIMAGE_TLS_CALLBACK> TlsCallbacks;
    TlsDir const MyTlsDir(MyPeFile);
    if (MyTlsDir.IsValid())
    {
      TlsCallbacks = MyTlsDir.GetCallbacks();
      std::for_each(TlsCallbacks.cbegin(), TlsCallbacks.cend(), 
        [&] (PIMAGE_TLS_CALLBACK pCurrent)
      {
        std::wcout << Path << " - TLS Callback: " << pCurrent << std::endl;
      });
    }
    
    std::wcout << Path << " - Writing DOS header." << std::endl;
    std::wcout << Path << " - DOS Header: " << RemoteBase << std::endl;
    m_Memory.Write(RemoteBase, *reinterpret_cast<PIMAGE_DOS_HEADER>(
      pBase));
    
    PBYTE const NtHeadersStart = reinterpret_cast<PBYTE>(MyNtHeaders.
      GetBase());
    PBYTE const NtHeadersEnd = static_cast<PBYTE>(
      Section(MyPeFile, 0).GetBase());
    std::vector<BYTE> const PeHeaderBuf(NtHeadersStart, NtHeadersEnd);
    PBYTE const TargetAddr = static_cast<PBYTE>(RemoteBase) + MyDosHeader.
      GetNewHeaderOffset();
    std::wcout << Path << " - Writing NT header." << std::endl;
    std::wcout << Path << " - NT Header: " << 
      static_cast<PVOID>(TargetAddr) << std::endl;
    m_Memory.WriteList(TargetAddr, PeHeaderBuf);
    
    FixRelocations(MyPeFile, RemoteBase);
    
    MapSections(MyPeFile, RemoteBase);

    // Import table must be processed in remote process due to cyclic 
    // depdendencies.
    PeFile RemotePeFile(m_Memory, RemoteBase);
    FixImports(RemotePeFile);
    
    std::for_each(TlsCallbacks.cbegin(), TlsCallbacks.cend(), 
      [&] (PIMAGE_TLS_CALLBACK pCallback) 
    {
      std::wcout << Path << " - TLS Callback: " << pCallback << "." 
        << std::endl;
      std::vector<PVOID> TlsCallArgs;
      TlsCallArgs.push_back(0);
      TlsCallArgs.push_back(reinterpret_cast<PVOID>(DLL_PROCESS_ATTACH));
      TlsCallArgs.push_back(RemoteBase);
      MemoryMgr::RemoteFunctionRet const TlsRet = 
        m_Memory.Call(reinterpret_cast<PBYTE>(RemoteBase) + 
        reinterpret_cast<DWORD_PTR>(pCallback), 
        MemoryMgr::CallConv_Default, TlsCallArgs);
      std::wcout << Path << " - TLS Callback Returned: " << 
        TlsRet.GetReturnValue() << "." << std::endl;
    });
    
    PVOID EntryPoint = nullptr;
    DWORD AddressOfEP = MyNtHeaders.GetAddressOfEntryPoint();
    if (AddressOfEP)
    {
      EntryPoint = static_cast<PBYTE>(RemoteBase) + 
        MyNtHeaders.GetAddressOfEntryPoint();
    }
    
    std::wcout << Path << " - Entry Point: " << EntryPoint << "." 
      << std::endl;
    
    if (EntryPoint)
    {
      std::vector<PVOID> EpArgs;
      EpArgs.push_back(0);
      EpArgs.push_back(reinterpret_cast<PVOID>(DLL_PROCESS_ATTACH));
      EpArgs.push_back(RemoteBase);
      MemoryMgr::RemoteFunctionRet const EpRet = m_Memory.Call(EntryPoint, 
        MemoryMgr::CallConv_Default, EpArgs);
      std::wcout << Path << " - Entry Point Returned: " << 
        EpRet.GetReturnValue() << "." << std::endl;
    }
    
    Detail::EnsureFreeLibrary const LocalMod(LoadLibraryEx(
      Path.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES));
    if (!LocalMod)
    {
      DWORD const LastError = GetLastError();
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorFunction("ManualMap::InjectDll") << 
        ErrorString("Could not load module locally.") << 
        ErrorCodeWinLast(LastError));
    }
    
    PVOID ExportAddr = nullptr;
    if (!Export.empty())
    {
      FARPROC const LocalFunc = GetProcAddress(LocalMod, Export.c_str());
      if (!LocalFunc)
      {
        DWORD const LastError = GetLastError();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("ManualMap::InjectDll") << 
          ErrorString("Could not find target function.") << 
          ErrorCodeWinLast(LastError));
      }
      
      LONG_PTR const FuncDelta = reinterpret_cast<DWORD_PTR>(LocalFunc) - 
        reinterpret_cast<DWORD_PTR>(static_cast<HMODULE>(LocalMod));
      
      FARPROC const RemoteFunc = reinterpret_cast<FARPROC>(
        reinterpret_cast<DWORD_PTR>(RemoteBase) + FuncDelta);
      
      ExportAddr = reinterpret_cast<PVOID const>(reinterpret_cast<DWORD_PTR>(
        RemoteFunc));
    }
    
    std::wcout << Path << " - Export Address: " << ExportAddr << "." 
      << std::endl;
    
    if (ExportAddr)
    {
      std::vector<PVOID> ExpArgs;
      ExpArgs.push_back(RemoteBase);
      MemoryMgr::RemoteFunctionRet const ExpRet = m_Memory.Call(ExportAddr, 
        MemoryMgr::CallConv_Default, ExpArgs);
      std::wcout << Path << " - Export Returned: " << ExpRet.GetReturnValue() 
        << "." << std::endl;
    }
    
    return reinterpret_cast<HMODULE>(RemoteBase);
  }

  // Map sections
  void ManualMap::MapSections(PeFile& MyPeFile, PVOID RemoteAddr) const
  {
    std::cout << "Mapping sections." << std::endl;
    
    SectionList Sections(MyPeFile);
    std::for_each(Sections.cbegin(), Sections.cend(), 
      [&] (Section const& S)
      {
        std::string const Name(S.GetName());
        std::cout << "Section Name: " << Name.c_str() << std::endl;
        
        PVOID const TargetAddr = reinterpret_cast<PBYTE>(RemoteAddr) + 
          S.GetVirtualAddress();
        std::cout << "Target Address: " << TargetAddr << std::endl;
        
        DWORD const VirtualSize = S.GetVirtualSize(); 
        std::cout << "Virtual Size: " << std::hex << VirtualSize << std::dec 
          << std::endl;
        
        DWORD const SizeOfRawData = S.GetSizeOfRawData();
        PBYTE const DataStart = static_cast<PBYTE>(MyPeFile.GetBase()) + 
          S.GetPointerToRawData();
        PBYTE const DataEnd = DataStart + SizeOfRawData;
        
        std::vector<BYTE> const SectionData(DataStart, DataEnd);
        
        if (!SectionData.empty())
        {
          m_Memory.WriteList(TargetAddr, SectionData);
        }
        
        DWORD SecCharacteristics = S.GetCharacteristics();
        
        std::array<ULONG, 16> const SectionCharacteristicsToProtect = 
        {{
          PAGE_NOACCESS, 
          PAGE_NOACCESS, 
          PAGE_EXECUTE, 
          PAGE_EXECUTE, 
          PAGE_READONLY, 
          PAGE_READONLY, 
          PAGE_EXECUTE_READ, 
          PAGE_EXECUTE_READ, 
          PAGE_READWRITE, 
          PAGE_READWRITE, 
          PAGE_EXECUTE_READWRITE, 
          PAGE_EXECUTE_READWRITE, 
          PAGE_READWRITE, 
          PAGE_READWRITE, 
          PAGE_EXECUTE_READWRITE, 
          PAGE_EXECUTE_READWRITE, 
        }};
        
        // Handle case where no explicit protection is provided
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
        
        DWORD SecProtect = SectionCharacteristicsToProtect[
          SecCharacteristics >> 28];
        
        DWORD OldProtect;
        if (!VirtualProtectEx(m_Memory.GetProcessHandle(), TargetAddr, 
          VirtualSize, SecProtect, &OldProtect))
        {
          DWORD const LastError = GetLastError();
          BOOST_THROW_EXCEPTION(ManualMap::Error() << 
            ErrorFunction("ManualMap::MapSections") << 
            ErrorString("Could not change page protections for section.") << 
            ErrorCodeWinLast(LastError));
        }
      });
  }

  // Fix imports
  void ManualMap::FixImports(PeFile& MyPeFile) const
  {
    NtHeaders const MyNtHeaders(MyPeFile);

    ImportDir const CheckImpDir(MyPeFile);
    if (!CheckImpDir.IsValid())
    {
      std::cout << "Image has no imports." << std::endl;
      return;
    }
    
    std::cout << "Fixing imports." << std::endl;
    
    ImportDirList ImportDirs(MyPeFile);
    std::for_each(ImportDirs.begin(), ImportDirs.end(), 
      [&] (ImportDir const& I)
      {
        std::string const ModuleName(I.GetName());
        std::wstring const ModuleNameW(boost::lexical_cast<std::wstring>(
          ModuleName));
        std::wstring const ModuleNameLowerW(boost::to_lower_copy(
          ModuleNameW));
        std::cout << "Module Name: " << ModuleName << "." << std::endl;
        
        HMODULE CurModBase = nullptr;
        
        // FIXME: Support both path resolution cases
        std::wstring const ResolvedModulePath = ResolvePath(ModuleNameLowerW, 
          false);
        auto const MappedModIter = m_MappedMods.find(boost::to_lower_copy(
          ResolvedModulePath));
        if (MappedModIter != m_MappedMods.end())
        {
          std::cout << "Found existing mapped instance of dependent DLL." << 
            std::endl;
          CurModBase = MappedModIter->second;
        }
        else
        {
          // Handle NTDLL.dll as a special case. It doesn't not work if you 
          // don't call LdrInitializeThunk to bootstrap it, but if you do that 
          // on a manually mapped copy it causes the process to fail as it tries 
          // to initialize everything twice and causes conflicts.
          // For now, just use the copy that resides in the remote target. It's 
          // guaranteed to be there anyway...
          if (ModuleNameLowerW == L"ntdll.dll")
          {
            Module NtdllMod(m_Memory, L"ntdll.dll");
            CurModBase = NtdllMod.GetHandle();
          }
          else
          {
            std::cout << "Manually mapping dependent DLL. " << ModuleName << "." << std::endl;
            try
            {
              std::cout << "Attempting without path resolution." << std::endl;
              CurModBase = InjectDll(ModuleNameW);
            }
            catch (std::exception const& e)
            {
              std::cout << "Failed to map dependent DLL. " << ModuleName << "." << std::endl;
              std::cout << boost::diagnostic_information(e) << std::endl;
              std::cout << "Attempting with path resolution." << std::endl;
              CurModBase = InjectDll(ModuleNameW, "", InjectFlag_PathResolution);
            }
          }
        }
        
        PeFile DepPeFile(m_Memory, CurModBase);
        
        // Lift export list out of loop to allow for caching
        ExportList Exports(DepPeFile);
        
        ImportThunkList ImportOrigThunks(MyPeFile, I.GetCharacteristics());
        ImportThunkList ImportFirstThunks(MyPeFile, I.GetFirstThunk());
        for (auto j = ImportOrigThunks.cbegin(); j != ImportOrigThunks.cend(); ++j)
        {
          ImportThunk const& T = *j;
          
          if (T.ByOrdinal())
          {
            std::cout << "Function Ordinal: " << T.GetOrdinal() << "." 
              << std::endl;
          }
          else
          {
            std::cout << "Function Name: " << T.GetName() << "." << std::endl;
          }
          
          boost::optional<Export> TargetExport;
          
          if (T.ByOrdinal())
          {
            TargetExport = Export(DepPeFile, T.GetOrdinal());
          }
          // Attempt lookup by hint
          else
          {
            ExportDir DepExportDir(DepPeFile);
            DWORD const ImpHint = T.GetHint();
            DWORD const NumberOfNames = DepExportDir.GetNumberOfNames();
            if (DepExportDir.IsValid() && ImpHint && ImpHint < NumberOfNames)
            {
              try
              {
                DWORD* pNames = static_cast<DWORD*>(DepPeFile.RvaToVa(
                  DepExportDir.GetAddressOfNames()));              
                DWORD const HintNameRva = m_Memory.Read<DWORD>(pNames + ImpHint);
                std::string const HintName = m_Memory.ReadString<std::string>(
                  DepPeFile.RvaToVa(HintNameRva));
                
                if (HintName == T.GetName())
                {
                  std::cout << "Hint matched!" << std::endl;
                    
                  WORD* pOrdinals = static_cast<WORD*>(DepPeFile.RvaToVa(
                    DepExportDir.GetAddressOfNameOrdinals()));
                  
                  WORD const HintOrdinal = m_Memory.Read<WORD>(pOrdinals + 
                    ImpHint);
                  
                  TargetExport = Export(DepPeFile, HintOrdinal + 
                    DepExportDir.GetOrdinalBase());
                  
                  if (TargetExport->GetName() != HintName)
                  {
                    std::cout << "Hint name mismatch!" << std::endl;
                    throw std::exception();
                  }
                }
              }
              catch (std::exception const& /*e*/)
              { }
            }
          }
  
          // If lookup by ordinal or hint failed do a manual lookup
          if (!TargetExport)
          {
            TargetExport = FindExport(DepPeFile, T.GetName());
          }
          
          FARPROC FuncAddr = ResolveExport(*TargetExport);
          
          if (!FuncAddr)
          {
            BOOST_THROW_EXCEPTION(ManualMap::Error() << 
              ErrorFunction("ManualMap::FixImports") << 
              ErrorString("Could not find current import."));
          }
          
          auto ImpThunkFT = ImportFirstThunks.begin();
          std::advance(ImpThunkFT, std::distance(ImportOrigThunks.cbegin(), j));
          ImpThunkFT->SetFunction(reinterpret_cast<DWORD_PTR>(FuncAddr));
        }
      });
  }

  // Fix relocations
  void ManualMap::FixRelocations(PeFile& MyPeFile, PVOID pRemoteBase) const
  {
    NtHeaders const MyNtHeaders(MyPeFile);
    
    DWORD const RelocDirSize = MyNtHeaders.GetDataDirectorySize(NtHeaders::
      DataDir_BaseReloc);
    PIMAGE_BASE_RELOCATION pRelocDir = 
      static_cast<PIMAGE_BASE_RELOCATION>(MyPeFile.RvaToVa(MyNtHeaders.
      GetDataDirectoryVirtualAddress(NtHeaders::DataDir_BaseReloc)));
    if (!RelocDirSize || !pRelocDir)
    {
      std::cout << "Image has no relocations." << std::endl;
      return;
    }
    
    PVOID pRelocDirEnd = reinterpret_cast<PBYTE>(pRelocDir) + RelocDirSize;
    
    std::cout << "Fixing relocations." << std::endl;
    
    ULONG_PTR const ImageBase = MyNtHeaders.GetImageBase();
    
    LONG_PTR const Delta = reinterpret_cast<ULONG_PTR>(pRemoteBase) - 
      ImageBase;
    
    while (pRelocDir < pRelocDirEnd && pRelocDir->SizeOfBlock > 0)
    {
      PBYTE const RelocBase = static_cast<PBYTE>(MyPeFile.RvaToVa(
        pRelocDir->VirtualAddress));
      
      DWORD const NumRelocs = (pRelocDir->SizeOfBlock - sizeof(
        IMAGE_BASE_RELOCATION)) / sizeof(WORD); 
      
      PWORD pRelocData = reinterpret_cast<PWORD>(pRelocDir + 1);
      
      for(DWORD i = 0; i < NumRelocs; ++i, ++pRelocData) 
      {
        BYTE RelocType = *pRelocData >> 12;
        WORD Offset = *pRelocData & 0xFFF;
        
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
      
      pRelocDir = reinterpret_cast<PIMAGE_BASE_RELOCATION>(pRelocData); 
    }
  }
    
  // Perform path resolution
  std::wstring ManualMap::ResolvePath(std::wstring const& Path, bool PathResolution) const
  {
    boost::filesystem::path PathReal(Path);
    
    if (PathResolution)
    {
      if (PathReal.is_relative())
      {
        PathReal = boost::filesystem::absolute(PathReal, 
          Detail::GetSelfDirPath());
      }
      
      // Ensure target file exists
      // Note: Only performing this check when path resolution is enabled, 
      // because otherwise we would need to perform the check in the context 
      // of the remote process, which is not possible to do without 
      // introducing race conditions and other potential problems. So we just 
      // let LoadLibraryW do the check for us.
      if (!boost::filesystem::exists(PathReal))
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("ManualMap::ResolvePath") << 
          ErrorString("Could not find module file."));
      }
    }

    // If path resolution is disabled, replicate the Windows DLL search order 
    // to try and find the target.
    // FIXME: Not a complte implementation of the Windows DLL search order 
    // algorithm. The following conditions need to be supported:
    // 1. If a DLL with the same module name is already loaded in memory, the 
    // system checks only for redirection and a manifest before resolving to 
    // the loaded DLL, no matter which directory it is in. The system does not 
    // search for the DLL.
    // 2. If the DLL is on the list of known DLLs for the version of Windows 
    // on which the application is running, the system uses its copy of the 
    // known DLL (and the known DLL's dependent DLLs, if any) instead of 
    // searching for the DLL. For a list of known DLLs on the current system, 
    // see the following registry key: HKEY_LOCAL_MACHINE\SYSTEM
    // \CurrentControlSet\Control\Session Manager\KnownDLLs.
    // 3. If a DLL has dependencies, the system searches for the dependent 
    // DLLs as if they were loaded with just their module names. This is true 
    // even if the first DLL was loaded by specifying a full path.
    // FIXME: Furthermore, this implementation does not search the 16-bit 
    // system directory, nor does it search the current working directory (as 
    // that is only meaningful in the context of the remote process), lastly, 
    // it does not search in %PATH%.
    if (!PathResolution)
    {
      boost::filesystem::path AppLoadDir = m_Memory.GetProcessPath();
      AppLoadDir = AppLoadDir.parent_path();
      
      boost::filesystem::path SystemDir;
      wchar_t Temp;
      UINT SysDirLen = GetSystemDirectory(&Temp, 1);
      if (!SysDirLen)
      {
        DWORD const LastError = GetLastError();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("ManualMap::ResolvePath") << 
          ErrorString("Could not get length of system dir.") << 
          ErrorCodeWinLast(LastError));
      }
      std::vector<wchar_t> SysDirTemp(SysDirLen);
      if (!GetSystemDirectory(SysDirTemp.data(), SysDirLen))
      {
        DWORD const LastError = GetLastError();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("ManualMap::ResolvePath") << 
          ErrorString("Could not get system dir.") << 
          ErrorCodeWinLast(LastError));
      }
      SystemDir = SysDirTemp.data();
      
      boost::filesystem::path WindowsDir;
      UINT WinDirLen = GetSystemDirectory(&Temp, 1);
      if (!WinDirLen)
      {
        DWORD const LastError = GetLastError();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("ManualMap::ResolvePath") << 
          ErrorString("Could not get length of windows dir.") << 
          ErrorCodeWinLast(LastError));
      }
      std::vector<wchar_t> WinDirTemp(WinDirLen);
      if (!GetSystemDirectory(WinDirTemp.data(), WinDirLen))
      {
        DWORD const LastError = GetLastError();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("ManualMap::ResolvePath") << 
          ErrorString("Could not get windows dir.") << 
          ErrorCodeWinLast(LastError));
      }
      WindowsDir = WinDirTemp.data();
      
      std::vector<boost::filesystem::path> SearchDirList;
      SearchDirList.push_back(AppLoadDir);
      SearchDirList.push_back(SystemDir);
      SearchDirList.push_back(WindowsDir);
      
      boost::filesystem::path ResolvedPath;
      for (auto i = SearchDirList.cbegin(); i != SearchDirList.cend(); ++i)
      {
        boost::filesystem::path const& Current = *i;
        boost::filesystem::path ResolvedPathTemp = boost::filesystem::absolute(
          PathReal, Current);
        if (boost::filesystem::exists(ResolvedPathTemp))
        {
          ResolvedPath = ResolvedPathTemp;
          break;
        }
      }
      
      if (ResolvedPath.empty())
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("ManualMap::ResolvePath") << 
          ErrorString("Could not find module file."));
      }
      
      PathReal = ResolvedPath;
    }
    
    PathReal.make_preferred();
    
    return PathReal.native();
  }
  
  // Resolve export
  FARPROC ManualMap::ResolveExport(Export const& E) const
  {
    if (E.Forwarded())
    {
      std::cout << "Forwarded export detected. Forwarder: " << 
        E.GetForwarder() << "." << std::endl;
      
      std::string ForwarderModuleLower = boost::to_lower_copy(
        E.GetForwarderModule());
      // FIXME: This is a nasty hack. Perform GetModuleHandle style path 
      // checking.
      if (ForwarderModuleLower.find('.') == std::string::npos)
      {
        ForwarderModuleLower += ".dll";
      }
      
      bool IsNtdll = ForwarderModuleLower == "ntdll.dll";
      
      // FIXME: Support both path resolution cases
      std::wstring const ResolvedModulePath = ResolvePath(
        boost::lexical_cast<std::wstring>(ForwarderModuleLower), false);
      auto const ForwardedModIter = m_MappedMods.find(boost::to_lower_copy(
        ResolvedModulePath));
      
      if (ForwardedModIter == m_MappedMods.end() && !IsNtdll)
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("ManualMap::ResolveExport") << 
          ErrorString("Found unknown forwarder module."));
      }
      
      HMODULE NewTarget = nullptr;
      if (IsNtdll)
      {
        Module NtdllMod(m_Memory, L"ntdll.dll");
        NewTarget = NtdllMod.GetHandle();
      }
      else
      {
        NewTarget = ForwardedModIter->second;
      }
      
      std::string const ForwarderFunction = E.GetForwarderFunction();
      bool ForwardedByOrdinal = (ForwarderFunction[0] == '#');
      WORD ForwarderOrdinal = 0;
      if (ForwardedByOrdinal)
      {
        try
        {
          ForwarderOrdinal = boost::lexical_cast<WORD>(ForwarderFunction.substr(1));
        }
        catch (std::exception const& /*e*/)
        {
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("ManualMap::ResolveExport") << 
            ErrorString("Invalid forwarder ordinal detected."));
        }
      }
      
      PeFile NewTargetPe(m_Memory, NewTarget);
      if (ForwardedByOrdinal)
      {
        std::cout << "Resolving forwarded export by ordinal." << std::endl;
        return ResolveExport(Export(NewTargetPe, ForwarderOrdinal));
      }
      else
      {
        std::cout << "Resolving forwarded export by name." << std::endl;
        return ResolveExport(FindExport(NewTargetPe, ForwarderFunction));
      }
    }
    else
    {
      return reinterpret_cast<FARPROC>(reinterpret_cast<DWORD_PTR>(E.GetVa()));
    }
  }
  
  // Find export by name
  Export ManualMap::FindExport(PeFile const& MyPeFile, 
    std::string const& Name) const
  {
    std::cout << "FindExport - " << Name << "." << std::endl;
      
    Export Target(MyPeFile, Name);
    
    if (Target.GetName() != Name)
    {
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorFunction("ManualMap::FindExport") << 
        ErrorString("Name mismatch."));
    }
    
    return Target;
  }
  
  // Equality operator
  bool ManualMap::operator==(ManualMap const& Rhs) const
  {
    return m_Memory == Rhs.m_Memory;
  }
  
  // Inequality operator
  bool ManualMap::operator!=(ManualMap const& Rhs) const
  {
    return !(*this == Rhs);
  }
}
