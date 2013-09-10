// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <algorithm>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/locale.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <windows.h>

#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/region.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/module_list.hpp>
#include <hadesmem/region_list.hpp>
#include <hadesmem/thread_list.hpp>
#include <hadesmem/pelib/export.hpp>
#include <hadesmem/process_list.hpp>
#include <hadesmem/thread_entry.hpp>
#include <hadesmem/process_entry.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/pelib/section.hpp>
#include <hadesmem/pelib/tls_dir.hpp>
#include <hadesmem/pelib/dos_header.hpp>
#include <hadesmem/pelib/export_dir.hpp>
#include <hadesmem/pelib/import_dir.hpp>
#include <hadesmem/pelib/nt_headers.hpp>
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/detail/initialize.hpp>
#include <hadesmem/pelib/export_list.hpp>
#include <hadesmem/detail/make_unique.hpp>
#include <hadesmem/pelib/import_thunk.hpp>
#include <hadesmem/pelib/section_list.hpp>
#include <hadesmem/pelib/import_dir_list.hpp>
#include <hadesmem/pelib/import_thunk_list.hpp>

namespace
{

std::wstring PtrToString(void const* const ptr)
{
  std::wostringstream str;
  str.imbue(std::locale::classic());
  str << std::hex << reinterpret_cast<DWORD_PTR>(ptr);
  return str.str();
}

void DumpRegions(hadesmem::Process const& process)
{
  std::wcout << "\nRegions:\n";

  hadesmem::RegionList const regions(process);
  for (auto const& region : regions)
  {
    std::wcout << "\n";
    std::wcout << "\tBase: " << PtrToString(region.GetBase()) << "\n";
    std::wcout << "\tAllocation Base: " 
      << PtrToString(region.GetAllocBase()) << "\n";
    std::wcout << "\tAllocation Protect: " << std::hex 
      << region.GetAllocProtect() << std::dec << "\n";
    std::wcout << "\tSize: " << std::hex << region.GetSize() << std::dec 
      << "\n";
    std::wcout << "\tState: " << std::hex << region.GetState() << std::dec 
      << "\n";
    std::wcout << "\tProtect: " << std::hex << region.GetProtect() 
      << std::dec << "\n";
    std::wcout << "\tType: " << std::hex << region.GetType() << std::dec 
      << "\n";
  }
}

void DumpDosHeader(hadesmem::Process const& process, 
  hadesmem::PeFile const& pe_file)
{
  std::wcout << "\n\tDOS Header:\n";

  hadesmem::DosHeader dos_hdr(process, pe_file);
  std::wcout << std::boolalpha;
  std::wcout << "\n";
  std::wcout << "\t\tMagic: " << std::hex << dos_hdr.GetMagic() << std::dec 
    << "\n";
  std::wcout << "\t\tBytesOnLastPage: " << std::hex 
    << dos_hdr.GetBytesOnLastPage() << std::dec << "\n";
  std::wcout << "\t\tPagesInFile: " << std::hex << dos_hdr.GetPagesInFile() 
    << std::dec << "\n";
  std::wcout << "\t\tRelocations: " << std::hex << dos_hdr.GetRelocations() 
    << std::dec << "\n";
  std::wcout << "\t\tSizeOfHeaderInParagraphs: " << std::hex 
    << dos_hdr.GetSizeOfHeaderInParagraphs() << std::dec << "\n";
  std::wcout << "\t\tMinExtraParagraphs: " << std::hex 
    << dos_hdr.GetMinExtraParagraphs() << std::dec << "\n";
  std::wcout << "\t\tMaxExtraParagraphs: " << std::hex 
    << dos_hdr.GetMaxExtraParagraphs() << std::dec << "\n";
  std::wcout << "\t\tInitialSS: " << std::hex << dos_hdr.GetInitialSS() 
    << std::dec << "\n";
  std::wcout << "\t\tInitialSP: " << std::hex << dos_hdr.GetInitialSP() 
    << std::dec << "\n";
  std::wcout << "\t\tChecksum: " << std::hex << dos_hdr.GetChecksum() 
    << std::dec << "\n";
  std::wcout << "\t\tInitialIP: " << std::hex << dos_hdr.GetInitialIP() 
    << std::dec << "\n";
  std::wcout << "\t\tInitialCS: " << std::hex << dos_hdr.GetInitialCS() 
    << std::dec << "\n";
  std::wcout << "\t\tRelocTableFileAddr: " << std::hex 
    << dos_hdr.GetRelocTableFileAddr() << std::dec << "\n";
  std::wcout << "\t\tOverlayNum: " << std::hex << dos_hdr.GetOverlayNum() 
    << std::dec << "\n";
  std::wcout << "\t\tReservedWords1:" << std::hex;
  auto const reserved_words_1 = dos_hdr.GetReservedWords1();
  for (auto const r : reserved_words_1)
  {
    std::wcout << L' ' << r;
  }
  std::wcout  << std::dec << "\n";
  std::wcout << "\t\tOEMID: " << std::hex << dos_hdr.GetOEMID() << std::dec 
    << "\n";
  std::wcout << "\t\tOEMInfo: " << std::hex << dos_hdr.GetOEMInfo() 
    << std::dec << "\n";
  std::wcout << "\t\tReservedWords2:" << std::hex;
  auto const reserved_words_2 = dos_hdr.GetReservedWords2();
  for (auto const r : reserved_words_2)
  {
    std::wcout << L' ' << r;
  }
  std::wcout  << std::dec << "\n";
  std::wcout << "\t\tNewHeaderOffset: " << std::hex 
    << dos_hdr.GetNewHeaderOffset() << std::dec << "\n";
  std::wcout << std::noboolalpha;
}

void DumpNtHeaders(hadesmem::Process const& process, 
  hadesmem::PeFile const& pe_file)
{
  std::wcout << "\n\tNT Headers:\n";

  hadesmem::NtHeaders nt_hdrs(process, pe_file);
  std::wcout << std::boolalpha;
  std::wcout << "\n";
  std::wcout << "\t\tSignature: " << std::hex << nt_hdrs.GetSignature() 
    << std::dec << "\n";
  std::wcout << "\t\tMachine: " << std::hex 
    << nt_hdrs.GetMachine() << std::dec << "\n";
  std::wcout << "\t\tNumberOfSections: " << std::hex 
    << nt_hdrs.GetNumberOfSections() << std::dec << "\n";
  std::wcout << "\t\tTimeDateStamp: " << std::hex 
    << nt_hdrs.GetTimeDateStamp() << std::dec << "\n";
  std::wcout << "\t\tPointerToSymbolTable: " << std::hex 
    << nt_hdrs.GetPointerToSymbolTable() << std::dec << "\n";
  std::wcout << "\t\tNumberOfSymbols: " << std::hex 
    << nt_hdrs.GetNumberOfSymbols() << std::dec << "\n";
  std::wcout << "\t\tSizeOfOptionalHeader: " << std::hex 
    << nt_hdrs.GetSizeOfOptionalHeader() << std::dec << "\n";
  std::wcout << "\t\tCharacteristics: " << std::hex 
    << nt_hdrs.GetCharacteristics() << std::dec << "\n";
  std::wcout << "\t\tMagic: " << std::hex << nt_hdrs.GetMagic() << std::dec 
    << "\n";
  std::wcout << "\t\tMajorLinkerVersion: " << nt_hdrs.GetMajorLinkerVersion() 
    << "\n";
  std::wcout << "\t\tMinorLinkerVersion: " << nt_hdrs.GetMinorLinkerVersion() 
    << "\n";
  std::wcout << "\t\tSizeOfCode: " << std::hex << nt_hdrs.GetSizeOfCode() 
    << std::dec << "\n";
  std::wcout << "\t\tSizeOfInitializedData: " << std::hex 
    << nt_hdrs.GetSizeOfInitializedData() << std::dec << "\n";
  std::wcout << "\t\tSizeOfUninitializedData: " << std::hex 
    << nt_hdrs.GetSizeOfUninitializedData() << std::dec << "\n";
  std::wcout << "\t\tAddressOfEntryPoint: " << std::hex 
    << nt_hdrs.GetAddressOfEntryPoint() << std::dec << "\n";
  std::wcout << "\t\tBaseOfCode: " << std::hex << nt_hdrs.GetBaseOfCode() 
    << std::dec << "\n";
#if defined(HADESMEM_DETAIL_ARCH_X86) 
  std::wcout << "\t\tBaseOfData: " << std::hex << nt_hdrs.GetBaseOfData() 
    << std::dec << "\n";
#endif
  std::wcout << "\t\tImageBase: " << std::hex << nt_hdrs.GetImageBase() 
    << std::dec << "\n";
  std::wcout << "\t\tSectionAlignment: " << std::hex 
    << nt_hdrs.GetSectionAlignment() << std::dec << "\n";
  std::wcout << "\t\tFileAlignment: " << std::hex 
    << nt_hdrs.GetFileAlignment() << std::dec << "\n";
  std::wcout << "\t\tMajorOperatingSystemVersion: " 
    << nt_hdrs.GetMajorOperatingSystemVersion() << "\n";
  std::wcout << "\t\tMinorOperatingSystemVersion: " 
    << nt_hdrs.GetMinorOperatingSystemVersion() << "\n";
  std::wcout << "\t\tMajorImageVersion: " << nt_hdrs.GetMajorImageVersion() 
    << "\n";
  std::wcout << "\t\tMinorImageVersion: " << nt_hdrs.GetMinorImageVersion() 
    << "\n";
  std::wcout << "\t\tMajorSubsystemVersion: " 
    << nt_hdrs.GetMajorSubsystemVersion() << "\n";
  std::wcout << "\t\tMinorSubsystemVersion: " 
    << nt_hdrs.GetMinorSubsystemVersion() << "\n";
  std::wcout << "\t\tWin32VersionValue: " << nt_hdrs.GetWin32VersionValue() 
    << "\n";
  std::wcout << "\t\tSizeOfImage: " << std::hex << nt_hdrs.GetSizeOfImage() 
    << std::dec << "\n";
  std::wcout << "\t\tSizeOfHeaders: " << std::hex 
    << nt_hdrs.GetSizeOfHeaders() << std::dec << "\n";
  std::wcout << "\t\tCheckSum: " << std::hex << nt_hdrs.GetCheckSum() 
    << std::dec << "\n";
  std::wcout << "\t\tSubsystem: " << std::hex << nt_hdrs.GetSubsystem() 
    << std::dec << "\n";
  std::wcout << "\t\tDllCharacteristics: " << std::hex 
    << nt_hdrs.GetDllCharacteristics() << std::dec << "\n";
  std::wcout << "\t\tSizeOfStackReserve: " << std::hex 
    << nt_hdrs.GetSizeOfStackReserve() << std::dec << "\n";
  std::wcout << "\t\tSizeOfStackCommit: " << std::hex 
    << nt_hdrs.GetSizeOfStackCommit() << std::dec << "\n";
  std::wcout << "\t\tSizeOfHeapReserve: " << std::hex 
    << nt_hdrs.GetSizeOfHeapReserve() << std::dec << "\n";
  std::wcout << "\t\tSizeOfHeapCommit: " << std::hex 
    << nt_hdrs.GetSizeOfHeapCommit() << std::dec << "\n";
  std::wcout << "\t\tLoaderFlags: " << std::hex << nt_hdrs.GetLoaderFlags() 
    << std::dec << "\n";
  std::wcout << "\t\tNumberOfRvaAndSizes: " 
    << nt_hdrs.GetNumberOfRvaAndSizes() << "\n";
  DWORD num_dirs = GetNumberOfRvaAndSizesClamped(nt_hdrs);
  std::wcout << "\t\tNumberOfRvaAndSizes (Clamped): " << num_dirs << "\n";
  for (DWORD i = 0; i < num_dirs; ++i)
  {
    std::wcout << "\t\tDataDirectoryVirtualAddress: " << std::hex 
      << nt_hdrs.GetDataDirectoryVirtualAddress(
      static_cast<hadesmem::PeDataDir>(i)) << std::dec << "\n";
    std::wcout << "\t\tDataDirectorySize: " << std::hex 
      << nt_hdrs.GetDataDirectorySize(static_cast<hadesmem::PeDataDir>(i)) 
      << std::dec << "\n";
  }
  std::wcout << std::noboolalpha;
}

void DumpHeaders(hadesmem::Process const& process, 
  hadesmem::PeFile const& pe_file)
{
  DumpDosHeader(process, pe_file);
  
  DumpNtHeaders(process, pe_file);
}

void DumpSections(hadesmem::Process const& process, 
  hadesmem::PeFile const& pe_file)
{
  std::wcout << "\n\tSections:\n";

  hadesmem::SectionList sections(process, pe_file);
  for (auto const& s : sections)
  {
    std::wcout << std::boolalpha;
    std::wcout << "\n";
    std::wcout << "\t\tName: " << s.GetName().c_str() << "\n";
    std::wcout << "\t\tVirtualAddress: " << std::hex << s.GetVirtualAddress() 
      << std::dec << "\n";
    std::wcout << "\t\tVirtualSize: " << std::hex << s.GetVirtualSize() 
      << std::dec << "\n";
    std::wcout << "\t\tPointerToRawData: " << std::hex 
      << s.GetPointerToRawData() << std::dec << "\n";
    std::wcout << "\t\tSizeOfRawData: " << std::hex << s.GetSizeOfRawData() 
      << std::dec << "\n";
    std::wcout << "\t\tPointerToRelocations: " << std::hex 
      << s.GetPointerToRelocations() << std::dec << "\n";
    std::wcout << "\t\tPointerToLinenumbers: " << std::hex 
      << s.GetPointerToLinenumbers() << std::dec << "\n";
    std::wcout << "\t\tNumberOfRelocations: " << std::hex 
      << s.GetNumberOfRelocations() << std::dec << "\n";
    std::wcout << "\t\tNumberOfLinenumbers: " << std::hex 
      << s.GetNumberOfLinenumbers() << std::dec << "\n";
    std::wcout << "\t\tCharacteristics: " << std::hex 
      << s.GetCharacteristics() << std::dec << "\n";
    std::wcout << std::noboolalpha;
  }
}

void DumpTls(hadesmem::Process const& process, 
  hadesmem::PeFile const& pe_file)
{
  std::unique_ptr<hadesmem::TlsDir> tls_dir;
  try
  {
    tls_dir = hadesmem::detail::make_unique<hadesmem::TlsDir>(
      process, pe_file);
  }
  catch (std::exception const& /*e*/)
  {
    return;
  }

  std::wcout << "\n\tTLS:\n";

  std::wcout << std::boolalpha;
  std::wcout << "\n";
  std::wcout << "\t\tStartAddressOfRawData: " << std::hex 
    << tls_dir->GetStartAddressOfRawData() << std::dec << "\n";
  std::wcout << "\t\tEndAddressOfRawData: " << std::hex 
    << tls_dir->GetEndAddressOfRawData() << std::dec << "\n";
  std::wcout << "\t\tAddressOfIndex: " << std::hex 
    << tls_dir->GetAddressOfIndex() << std::dec << "\n";
  std::wcout << "\t\tAddressOfCallBacks: " << std::hex 
    << tls_dir->GetAddressOfCallBacks() << std::dec << "\n";
  if (tls_dir->GetAddressOfCallBacks())
  {
    std::vector<PIMAGE_TLS_CALLBACK> callbacks;
    tls_dir->GetCallbacks(std::back_inserter(callbacks));
    for (auto const c : callbacks)
    {
      std::wcout << "\t\tCallback: " << std::hex 
        << reinterpret_cast<DWORD_PTR>(c) << std::dec << "\n";
    }
  }
  std::wcout << "\t\tSizeOfZeroFill: " << std::hex 
    << tls_dir->GetSizeOfZeroFill() << std::dec << "\n";
  std::wcout << "\t\tCharacteristics: " << std::hex 
    << tls_dir->GetCharacteristics() << std::dec << "\n";
  std::wcout << std::noboolalpha;
}

void DumpExports(hadesmem::Process const& process, 
  hadesmem::PeFile const& pe_file)
{
  std::unique_ptr<hadesmem::ExportDir> export_dir;
  try
  {
    export_dir = hadesmem::detail::make_unique<hadesmem::ExportDir>(process, 
      pe_file);
  }
  catch (std::exception const& /*e*/)
  {
    return;
  }

  std::wcout << "\n\tExport Dir:\n";

  std::wcout << std::boolalpha;
  std::wcout << "\n";
  std::wcout << "\t\tGetCharacteristics: " << std::hex 
    << export_dir->GetCharacteristics() << std::dec << "\n";
  std::wcout << "\t\tGetTimeDateStamp: " << std::hex 
    << export_dir->GetTimeDateStamp() << std::dec << "\n";
  std::wcout << "\t\tGetMajorVersion: " << std::hex 
    << export_dir->GetMajorVersion() << std::dec << "\n";
  std::wcout << "\t\tGetMinorVersion: " << std::hex 
    << export_dir->GetMinorVersion() << std::dec << "\n";
  std::wcout << "\t\tGetName: " << std::hex 
    << export_dir->GetName().c_str() << std::dec << "\n";
  std::wcout << "\t\tGetOrdinalBase: " << std::hex 
    << export_dir->GetOrdinalBase() << std::dec << "\n";
  std::wcout << "\t\tGetNumberOfFunctions: " << std::hex 
    << export_dir->GetNumberOfFunctions() << std::dec << "\n";
  std::wcout << "\t\tGetNumberOfNames: " << std::hex 
    << export_dir->GetNumberOfNames() << std::dec << "\n";
  std::wcout << "\t\tGetAddressOfFunctions: " << std::hex 
    << export_dir->GetAddressOfFunctions() << std::dec << "\n";
  std::wcout << "\t\tGetAddressOfNames: " << std::hex 
    << export_dir->GetAddressOfNames() << std::dec << "\n";
  std::wcout << "\t\tGetAddressOfNameOrdinals: " << std::hex 
    << export_dir->GetAddressOfNameOrdinals() << std::dec << "\n";
  std::wcout << std::noboolalpha;

  std::wcout << "\n\tExports:\n";

  hadesmem::ExportList exports(process, pe_file);
  for (auto const& e : exports)
  {
    std::wcout << std::boolalpha;
    std::wcout << "\n";
    std::wcout << "\t\tRVA: " << std::hex << e.GetRva() << std::dec << "\n";
    std::wcout << "\t\tVA: " << PtrToString(e.GetVa()) << "\n";
    if (e.ByName())
    {
      std::wcout << "\t\tName: " << e.GetName().c_str() << "\n";
    }
    else if (e.ByOrdinal())
    {
      std::wcout << "\t\tOrdinal: " << e.GetOrdinal() << "\n";
    }
    else
    {
      std::wcout << "\t\tWARNING! Entry not exported by name or ordinal.\n";
    }
    if (e.IsForwarded())
    {
      std::wcout << "\t\tForwarder: " << e.GetForwarder().c_str() << "\n";
      std::wcout << "\t\tForwarderModule: " << e.GetForwarderModule().c_str() 
        << "\n";
      std::wcout << "\t\tForwarderFunction: " << 
        e.GetForwarderFunction().c_str() << "\n";
      std::wcout << "\t\tIsForwardedByOrdinal: " << e.IsForwardedByOrdinal() << 
        "\n";
      if (e.IsForwardedByOrdinal())
      {
        try
        {
          std::wcout << "\t\tForwarderOrdinal: " << e.GetForwarderOrdinal() << 
            "\n";
        }
        catch (std::exception const& /*e*/)
        {
          std::wcout << "\t\tForwarderOrdinal Invalid.\n";
        }
      }
    }
    std::wcout << std::noboolalpha;
  }
}

void DumpImports(hadesmem::Process const& process, 
  hadesmem::PeFile const& pe_file)
{
  hadesmem::ImportDirList import_dirs(process, pe_file);

  if (std::begin(import_dirs) != std::end(import_dirs))
  {
    std::wcout << "\n\tImport Dirs:\n";
  }

  for (auto const& dir : import_dirs)
  {
    std::wcout << std::boolalpha;

    std::wcout << "\n";
    std::wcout << "\t\tOriginalFirstThunk: " << std::hex << 
      dir.GetOriginalFirstThunk() << std::dec << "\n";
    std::wcout << "\t\tTimeDateStamp: " << std::hex 
      << dir.GetTimeDateStamp() << std::dec << "\n";
    std::wcout << "\t\tForwarderChain: " << std::hex 
      << dir.GetForwarderChain() << std::dec << "\n";
    std::wcout << "\t\tName (Raw): " << std::hex << dir.GetNameRaw() << 
      std::dec << "\n";
    std::wcout << "\t\tName: " << dir.GetName().c_str() << "\n";
    std::wcout << "\t\tFirstThunk: " << std::hex << dir.GetFirstThunk() << 
      std::dec << "\n";

    std::wcout << "\n\t\tImport Thunks:\n";

    // Certain information gets destroyed by the Windows PE loader in 
    // some circumstances. Nothing we can do but ignore it or resort to 
    // reading the original data from disk.
    if (pe_file.GetType() == hadesmem::PeFileType::Image)
    {
      // Images without an INT/ILT are valid, but after an image like this is 
      // loaded it is impossible to recover the name table.
      if (!dir.GetOriginalFirstThunk())
      {
        std::wcout << "\n\t\t\tWARNING! No INT for this module.\n";
        continue;
      }

      // Some modules (packed modules are the only ones I've found so far) 
      // have import directories where the IAT RVA is the same as the INT/ILT 
      // RVA which effectively means there is no INT/ILT once the module is 
      // loaded.
      if (dir.GetOriginalFirstThunk() == dir.GetFirstThunk())
      {
        std::wcout << "\n\t\t\tWARNING! IAT is same as INT for this module.\n";
        continue;
      }
    }
    
    hadesmem::ImportThunkList import_thunks(process, pe_file, 
      dir.GetOriginalFirstThunk() ? dir.GetOriginalFirstThunk() : 
      dir.GetFirstThunk());
    for (auto const& thunk : import_thunks)
    {
      std::wcout << "\n";
      std::wcout << "\t\t\tAddressOfData: " << std::hex << 
        thunk.GetAddressOfData() << std::dec << "\n";
      std::wcout << "\t\t\tOrdinalRaw: " << thunk.GetOrdinalRaw() << "\n";
      if (thunk.ByOrdinal())
      {
        std::wcout << "\t\t\tOrdinal: " << thunk.GetOrdinal() << "\n";
      }
      else
      {
        std::wcout << "\t\t\tHint: " << thunk.GetHint() << "\n";
        std::wcout << "\t\t\tName: " << thunk.GetName().c_str() << "\n";
      }
      std::wcout << "\t\t\tFunction: " << std::hex << thunk.GetFunction() << 
        std::dec << "\n";
    }

    std::wcout << std::noboolalpha;
  }
}

void DumpModules(hadesmem::Process const& process)
{
  std::wcout << "\nModules:\n";

  hadesmem::ModuleList const modules(process);
  for (auto const& module : modules)
  {
    std::wcout << "\n";
    std::wcout << "\tHandle: " << PtrToString(module.GetHandle()) << "\n";
    std::wcout << "\tSize: " << std::hex << module.GetSize() << std::dec 
      << "\n";
    std::wcout << "\tName: " << module.GetName() << "\n";
    std::wcout << "\tPath: " << module.GetPath() << "\n";

    hadesmem::PeFile const pe_file(process, module.GetHandle(), 
      hadesmem::PeFileType::Image);

    try
    {
      hadesmem::DosHeader const dos_header(process, pe_file);
      hadesmem::NtHeaders const nt_headers(process, pe_file);
    }
    catch (std::exception const& /*e*/)
    {
      std::wcout << "\n";
      std::wcout << "\tWARNING! Not a valid PE file or architecture.\n";
      continue;
    }

    DumpHeaders(process, pe_file);

    DumpSections(process, pe_file);

    DumpTls(process, pe_file);

    DumpExports(process, pe_file);

    DumpImports(process, pe_file);
  }
}

void DumpThreadEntry(hadesmem::ThreadEntry const& thread_entry)
{
  std::wcout << "\n";
  std::wcout << "Usage: " << thread_entry.GetUsage() << "\n";
  std::wcout << "ID: " << thread_entry.GetId() << "\n";
  std::wcout << "Owner ID: " << thread_entry.GetOwnerId() << "\n";
  std::wcout << "Base Priority: " << thread_entry.GetBasePriority() << "\n";
  std::wcout << "Delta Priority: " << thread_entry.GetDeltaPriority() << "\n";
  std::wcout << "Flags: " << thread_entry.GetFlags() << "\n";
}

void DumpThreads(DWORD pid)
{
  std::wcout << "\nThreads:\n";

  hadesmem::ThreadList threads(pid);
  for (auto const& thread_entry : threads)
  {
    DumpThreadEntry(thread_entry);
  }
}

void DumpProcessEntry(hadesmem::ProcessEntry const& process_entry)
{
  std::wcout << "\n";
  std::wcout << "ID: " << process_entry.GetId() << "\n";
  std::wcout << "Threads: " << process_entry.GetThreads() << "\n";
  std::wcout << "Parent: " << process_entry.GetParentId() << "\n";
  std::wcout << "Priority: " << process_entry.GetPriority() << "\n";
  std::wcout << "Name: " << process_entry.GetName() << "\n";

  DumpThreads(process_entry.GetId());

  std::unique_ptr<hadesmem::Process> process;
  try
  {
    process = hadesmem::detail::make_unique<hadesmem::Process>(
      process_entry.GetId());
  }
  catch (std::exception const& /*e*/)
  {
    std::wcout << "\nCould not open process for further inspection.\n\n";
    return;
  }

  std::wcout << "Path: " << hadesmem::GetPath(*process) << "\n";
  std::wcout << "WoW64: " << (hadesmem::IsWoW64(*process) ? "Yes" : "No") 
    << "\n";

  DumpModules(*process);

  DumpRegions(*process);
}

void DumpProcesses()
{
  std::wcout << "\nProcesses:\n";

  hadesmem::ProcessList const processes;
  for (auto const& process_entry : processes)
  {
    DumpProcessEntry(process_entry);
  }
}

// TODO: Cleanup.
void DumpFile(boost::filesystem::path const& path)
{
#if defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
  std::ifstream file(path.native(), std::ios::binary | std::ios::ate);
#else // #if defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
  // TODO: Fix this for compilers other than MSVC and ICC.
  std::ifstream file(path.string<std::string>(), 
    std::ios::binary | std::ios::ate);
#endif // #if defined(HADESMEM_MSVC) || defined(HADESMEM_INTEL)
  if (!file)
  {
    std::wcout << "\nFailed to open file.\n";
    return;
  }

  std::streampos const size = file.tellg();
  if (!size || size < 0)
  {
    std::wcout << "\nEmpty or invalid file.\n";
    return;
  }

  if (!file.seekg(0, std::ios::beg))
  {
    std::wcout << "\nWARNING! Seeking to beginning of file failed (1).\n";
    return;
  }

  // Peek for the MZ header before reading the whole file.
  std::vector<char> mz_buf(2);
  if (!file.read(mz_buf.data(), 2))
  {
    std::wcout << "\nWARNING! Failed to read header signature.\n";
    return;
  }

  // Check for MZ signature
  if (mz_buf[0] != 'M' && mz_buf[1] != 'Z')
  {
    std::wcout << "\nNot a PE file (Pass 1).\n";
    return;
  }

  if (!file.seekg(0, std::ios::beg))
  {
    std::wcout << "\nWARNING! Seeking to beginning of file failed (2).\n";
    return;
  }

  // TODO: Fix all the unsafe integer downcasting.
  std::vector<char> buf(static_cast<std::size_t>(size));

  if (!file.read(buf.data(), static_cast<std::streamsize>(size)))
  {
    std::wcout << "\nWARNING! Failed to read file data.\n";
    return;
  }

  hadesmem::Process const process(GetCurrentProcessId());
  
  hadesmem::PeFile const pe_file(process, buf.data(), 
    hadesmem::PeFileType::Data);
  
  try
  {
    hadesmem::NtHeaders const nt_hdr(process, pe_file);
  }
  catch (std::exception const& /*e*/)
  {
    std::wcout << "\nNot a PE file or wrong architecture (Pass 2).\n";
    return;
  }

  DumpHeaders(process, pe_file);

  DumpSections(process, pe_file);

  DumpTls(process, pe_file);

  DumpExports(process, pe_file);

  DumpImports(process, pe_file);
}

// Doing directory recursion 'manually' because recursive_directory_iterator 
// throws on increment, even when you construct it as no-throw.
void DumpDir(boost::filesystem::path const& path)
{
  std::wcout << "\nEntering dir: " << path << ".\n";

  boost::system::error_code ec;
  boost::filesystem::directory_iterator iter(path, ec);
  boost::filesystem::directory_iterator end;

  while (iter != end && !ec)
  {
    auto const& cur_path = *iter;

    std::wcout << "\nCurrent path: " << cur_path << ".\n";
    
    if (boost::filesystem::is_directory(cur_path) && 
      !boost::filesystem::is_symlink(cur_path))
    {
      DumpDir(cur_path);
    }

    if (boost::filesystem::is_regular_file(cur_path))
    {
      DumpFile(cur_path);
    }

    ++iter;
  }
}

}

int main(int /*argc*/, char* /*argv*/[]) 
{
  try
  {
    hadesmem::detail::InitializeAll();

    std::cout << "HadesMem Dumper\n";

    boost::program_options::options_description opts_desc(
      "General options");
    opts_desc.add_options()
      ("help", "produce help message")
      ("pid", boost::program_options::value<DWORD>(), "target process id")
      ("file", boost::program_options::wvalue<std::wstring>(), "target file")
      ("dir", boost::program_options::wvalue<std::wstring>(), "target dir")
      ;

    std::vector<std::wstring> const args = boost::program_options::
      split_winmain(GetCommandLine());
    boost::program_options::variables_map var_map;
    boost::program_options::store(boost::program_options::wcommand_line_parser(
      args).options(opts_desc).run(), var_map);
    boost::program_options::notify(var_map);

    if (var_map.count("help"))
    {
      std::cout << '\n' << opts_desc << '\n';
      return 1;
    }

    try
    {
      hadesmem::GetSeDebugPrivilege();

      std::wcout << "\nAcquired SeDebugPrivilege.\n";
    }
    catch (std::exception const& /*e*/)
    {
      std::wcout << "\nFailed to acquire SeDebugPrivilege.\n";
    }

    if (var_map.count("pid"))
    {
      DWORD pid = var_map["pid"].as<DWORD>();

      hadesmem::ProcessList const processes;
      auto iter = std::find_if(std::begin(processes), std::end(processes), 
        [pid] (hadesmem::ProcessEntry const& process_entry)
        {
          return process_entry.GetId() == pid;
        });
      if (iter != std::end(processes))
      {
        DumpProcessEntry(*iter);
      }
      else
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error() << 
          hadesmem::ErrorString("Failed to find requested process."));
      }
    }
    else if (var_map.count("file"))
    {
      DumpFile(var_map["file"].as<std::wstring>());
    }
    else if (var_map.count("dir"))
    {
      DumpDir(var_map["dir"].as<std::wstring>());
    }
    else
    {
      DumpThreads(static_cast<DWORD>(-1));

      DumpProcesses();

      std::wcout << "\nFiles:\n";

      boost::filesystem::path const self_path = 
        hadesmem::detail::GetSelfPath();
      boost::filesystem::path const root_dir = self_path.root_path();
      DumpDir(root_dir);
  }

    return 0;
  }
  catch (std::exception const& e)
  {
    std::cerr << "\nError!\n";
    std::cerr << boost::diagnostic_information(e) << '\n';

    return 1;
  }
}
