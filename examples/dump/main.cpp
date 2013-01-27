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
#include <hadesmem/pelib/export.hpp>
#include <hadesmem/process_list.hpp>
#include <hadesmem/process_entry.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/pelib/section.hpp>
#include <hadesmem/pelib/tls_dir.hpp>
#include <hadesmem/pelib/dos_header.hpp>
#include <hadesmem/pelib/import_dir.hpp>
#include <hadesmem/pelib/nt_headers.hpp>
#include <hadesmem/detail/self_path.hpp>
#include <hadesmem/detail/initialize.hpp>
#include <hadesmem/pelib/export_list.hpp>
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

void DumpHeaders(hadesmem::Process const& process, PVOID module, 
  hadesmem::PeFileType type)
{
  std::wcout << "\n\tDOS Header:\n";

  hadesmem::PeFile pe_file(process, module, type);

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
#if defined(HADESMEM_ARCH_X86) 
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
  DWORD const num_dirs = nt_hdrs.GetNumberOfRvaAndSizes();
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

void DumpSections(hadesmem::Process const& process, PVOID module, 
  hadesmem::PeFileType type)
{
  std::wcout << "\n\tSections:\n";

  hadesmem::PeFile pe_file(process, module, type);
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

void DumpTls(hadesmem::Process const& process, PVOID module, 
  hadesmem::PeFileType type)
{
  hadesmem::PeFile pe_file(process, module, type);
  std::unique_ptr<hadesmem::TlsDir> tls_dir;
  try
  {
    tls_dir.reset(new hadesmem::TlsDir(process, pe_file));
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
  auto const callbacks = tls_dir->GetCallbacks();
  for (auto const c : callbacks)
  {
    std::wcout << "\t\tCallback: " << std::hex 
      << reinterpret_cast<DWORD_PTR>(c) << std::dec << "\n";
  }
  std::wcout << "\t\tSizeOfZeroFill: " << std::hex 
    << tls_dir->GetSizeOfZeroFill() << std::dec << "\n";
  std::wcout << "\t\tCharacteristics: " << std::hex 
    << tls_dir->GetCharacteristics() << std::dec << "\n";
  std::wcout << std::noboolalpha;
}

void DumpExports(hadesmem::Process const& process, PVOID module, 
  hadesmem::PeFileType type)
{
  std::wcout << "\n\tExports:\n";

  hadesmem::PeFile pe_file(process, module, type);
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

void DumpImports(hadesmem::Process const& process, PVOID module, 
  hadesmem::PeFileType type)
{
  std::wcout << "\n\tImport Dirs:\n";

  hadesmem::PeFile pe_file(process, module, type);
  hadesmem::ImportDirList import_dirs(process, pe_file);
  for (auto const& dir : import_dirs)
  {
    std::wcout << std::boolalpha;

    std::wcout << "\n";
    std::wcout << "\t\tOriginalFirstThunk: " << std::hex << 
      dir.GetOriginalFirstThunk() << std::dec << "\n";
    std::wcout << "\t\tTimeDateStamp: " << dir.GetTimeDateStamp() << "\n";
    std::wcout << "\t\tForwarderChain: " << dir.GetForwarderChain() << "\n";
    std::wcout << "\t\tName (Raw): " << std::hex << dir.GetNameRaw() << 
      std::dec << "\n";
    std::wcout << "\t\tName: " << dir.GetName().c_str() << "\n";
    std::wcout << "\t\tFirstThunk: " << std::hex << dir.GetFirstThunk() << 
      std::dec << "\n";

    std::wcout << "\n\t\tImport Thunks:\n";

    // Certain information gets destroyed by the Windows PE loader in 
    // some circumstances. Nothing we can do but ignore it or resort to 
    // reading the original data from disk.
    if (type == hadesmem::PeFileType::Image)
    {
      // Images without an INT are valid, but after an image like this is 
      // loaded it is impossible to recover the name table.
      if (!dir.GetOriginalFirstThunk())
      {
        std::wcout << "\n\t\t\tWARNING! No INT for this module.\n";
        continue;
      }

      // Some modules (packed modules are the only ones I've found so far) 
      // have import directories where the IAT RVA is the same as the INT RVA 
      // which effectively means there is no INT once the module is loaded.
      if (dir.GetOriginalFirstThunk() == dir.GetFirstThunk())
      {
        std::wcout << "\n\t\t\tWARNING! IAT is same as INT for this module.\n";
        continue;
      }
    }
    
    hadesmem::ImportThunkList import_thunks(process, pe_file, 
      dir.GetOriginalFirstThunk());
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

    DumpHeaders(process, module.GetHandle(), hadesmem::PeFileType::Image);

    DumpSections(process, module.GetHandle(), hadesmem::PeFileType::Image);

    DumpTls(process, module.GetHandle(), hadesmem::PeFileType::Image);

    DumpExports(process, module.GetHandle(), hadesmem::PeFileType::Image);

    DumpImports(process, module.GetHandle(), hadesmem::PeFileType::Image);
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

  std::unique_ptr<hadesmem::Process> process;
  try
  {
    process.reset(new hadesmem::Process(process_entry.GetId()));
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

// TODO: Cleanup.
void DumpFile(boost::filesystem::path const& path)
{
  std::string const path_utf8 = boost::locale::conv::utf_to_utf<char>(
    path.native());
  std::ifstream file(path_utf8.c_str(), std::ios::binary | std::ios::ate);
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

  try
  {
    hadesmem::PeFile const pe_file(process, buf.data(), 
      hadesmem::PeFileType::Data);
    hadesmem::NtHeaders const nt_hdr(process, pe_file);
#if defined(HADESMEM_ARCH_X86)
    if (nt_hdr.GetMachine() != IMAGE_FILE_MACHINE_I386)
#elif defined(HADESMEM_ARCH_X64)
    if (nt_hdr.GetMachine() != IMAGE_FILE_MACHINE_AMD64)
#else
#error [HadesMem] Unsupported architecture.
#endif
    {
      std::wcout << "\nArchitecture mismatch.\n";
      return;
    }
  }
  catch (std::exception const& /*e*/)
  {
    std::wcout << "\nNot a PE file (Pass 2).\n";
    return;
  }

  DumpHeaders(process, buf.data(), hadesmem::PeFileType::Data);

  DumpSections(process, buf.data(), hadesmem::PeFileType::Data);

  DumpTls(process, buf.data(), hadesmem::PeFileType::Data);

  DumpExports(process, buf.data(), hadesmem::PeFileType::Data);

  DumpImports(process, buf.data(), hadesmem::PeFileType::Data);
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
    hadesmem::detail::DisableUserModeCallbackExceptionFilter();
    hadesmem::detail::EnableCrtDebugFlags();
    hadesmem::detail::EnableTerminationOnHeapCorruption();
    hadesmem::detail::EnableBottomUpRand();
    hadesmem::detail::ImbueAllDefault();

    std::cout << "HadesMem Dumper\n";

    boost::program_options::options_description opts_desc(
      "General options");
    opts_desc.add_options()
      ("help", "produce help message")
      ("pid", boost::program_options::value<DWORD>(), "process id")
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

    DWORD pid = 0;
    if (var_map.count("pid"))
    {
      pid = var_map["pid"].as<DWORD>();
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

    if (pid)
    {
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
        HADESMEM_THROW_EXCEPTION(hadesmem::Error() << 
          hadesmem::ErrorString("Failed to find requested process."));
      }
    }
    else
    {
      std::wcout << "\nProcesses:\n";

      hadesmem::ProcessList const processes;
      for (auto const& process_entry : processes)
      {
        DumpProcessEntry(process_entry);
      }

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
