// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "headers.hpp"

#include <iostream>

#include <hadesmem/config.hpp>
#include <hadesmem/pelib/dos_header.hpp>
#include <hadesmem/pelib/nt_headers.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>

#include "main.hpp"

// TODO: Detect when the file has a writable PE header (both methods). See
// "Writable PE header" in the ReversingLabs "Undocumented PECOFF" whitepaper.

// TODO: Detect using relocations as an obfuscation mechanism. See "File
// encryption via relocations" in ReversingLabs "Undocumented PECOFF" whitepaper
// for more information.

namespace
{

void DumpDosHeader(hadesmem::Process const& process,
                   hadesmem::PeFile const& pe_file)
{
  std::wcout << "\n\tDOS Header:\n";

  hadesmem::DosHeader dos_hdr(process, pe_file);
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
  std::wcout << std::dec << "\n";
  // ReservedWords1 is officially defined as reserved and should be null.
  // However, if non-null it overrides OS version values in the PEB after
  // loading.
  // Sample: winver.exe (Corkami PE Corpus)
  if (std::find_if(std::begin(reserved_words_1),
                   std::end(reserved_words_1),
                   [](WORD w)
  { return !!w; }) != std::end(reserved_words_1))
  {
    std::wcout << "\t\tWARNING! Detected non-zero data in ReservedWords1.\n";
    WarnForCurrentFile(WarningType::kSuspicious);
  }
  std::wcout << "\t\tOEMID: " << std::hex << dos_hdr.GetOEMID() << std::dec
             << "\n";
  std::wcout << "\t\tOEMInfo: " << std::hex << dos_hdr.GetOEMInfo() << std::dec
             << "\n";
  std::wcout << "\t\tReservedWords2:" << std::hex;
  auto const reserved_words_2 = dos_hdr.GetReservedWords2();
  for (auto const r : reserved_words_2)
  {
    std::wcout << L' ' << r;
  }
  std::wcout << std::dec << "\n";
  // TODO: Detect when e_lfanew is in the overlay and will not be mapped when
  // loaded into memory. See "Self-destructing PE header" in the ReversingLabs
  // "Undocumented PECOFF" whitepaper. Also investigate the second part of that
  // trick in regards to FileAlignment and NtSizeOfHeaders.
  // TODO: Detect when e_lfanew is set in a way that will cause the NT headers
  // to overlap physical and virtual parts of the file, causing an 'on disk'
  // header and an 'in memory' header. See "Dual PE header" in the ReversingLabs
  // "Undocumented PECOFF" whitepaper.
  // TODO: Warn on a non-standard value.
  std::wcout << "\t\tNewHeaderOffset: " << std::hex
             << dos_hdr.GetNewHeaderOffset() << std::dec << "\n";
}

std::wstring GetDataDirName(DWORD num)
{
  switch (static_cast<hadesmem::PeDataDir>(num))
  {
  case hadesmem::PeDataDir::Export:
    return L"Export";

  case hadesmem::PeDataDir::Import:
    return L"Import";

  case hadesmem::PeDataDir::Resource:
    return L"Resource";

  case hadesmem::PeDataDir::Exception:
    return L"Exception";

  case hadesmem::PeDataDir::Security:
    return L"Security";

  case hadesmem::PeDataDir::BaseReloc:
    return L"BaseReloc";

  case hadesmem::PeDataDir::Debug:
    return L"Debug";

  case hadesmem::PeDataDir::Architecture:
    return L"Architecture";

  case hadesmem::PeDataDir::GlobalPTR:
    return L"GlobalPTR";

  case hadesmem::PeDataDir::TLS:
    return L"TLS";

  case hadesmem::PeDataDir::LoadConfig:
    return L"LoadConfig";

  case hadesmem::PeDataDir::BoundImport:
    return L"BoundImport";

  case hadesmem::PeDataDir::IAT:
    return L"IAT";

  case hadesmem::PeDataDir::DelayImport:
    return L"DelayImport";

  case hadesmem::PeDataDir::COMDescriptor:
    return L"COMDescriptor";

  case hadesmem::PeDataDir::Reserved:
    return L"Reserved";

  default:
    HADESMEM_DETAIL_ASSERT(false);
    return L"UKNOWN";
  }
}

void DumpNtHeaders(hadesmem::Process const& process,
                   hadesmem::PeFile const& pe_file)
{
  std::wcout << "\n\tNT Headers:\n";

  hadesmem::NtHeaders nt_hdrs(process, pe_file);
  std::wcout << "\n";
  std::wcout << "\t\tSignature: " << std::hex << nt_hdrs.GetSignature()
             << std::dec << "\n";
  std::wcout << "\t\tMachine: " << std::hex << nt_hdrs.GetMachine() << std::dec
             << "\n";
  std::wcout << "\t\tNumberOfSections: " << std::hex
             << nt_hdrs.GetNumberOfSections() << std::dec << "\n";
  std::wcout << "\t\tTimeDateStamp: " << std::hex << nt_hdrs.GetTimeDateStamp()
             << std::dec << "\n";
  std::wcout << "\t\tPointerToSymbolTable: " << std::hex
             << nt_hdrs.GetPointerToSymbolTable() << std::dec << "\n";
  std::wcout << "\t\tNumberOfSymbols: " << std::hex
             << nt_hdrs.GetNumberOfSymbols() << std::dec << "\n";
  // TODO: Detect when SizeOfOptionalHeader has been set to put the section
  // table in unmapped space (e.g. the overlay).
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
  // TODO: Detect EP outside of the file (i.e. pointing to another non-relocated
  // module). See "AddressOfEntryPoint" in ReversingLabs "Undocumented PECOFF"
  // for more inforamtion. Also see AddressOfEntryPoint in Corkami PE info.
  // TODO: For valid EPs inside the file, dump the section that it is in, and
  // also disassemble the first N instructions (for some reasonable value of N).
  // TODO: Detect virtual overlap EP. (Sample: virtEP.exe from Corkami)
  DWORD const addr_of_ep = nt_hdrs.GetAddressOfEntryPoint();
  std::wcout << "\t\tAddressOfEntryPoint: " << std::hex << addr_of_ep
             << std::dec << "\n";
  // Entry point can be null. For DLLs this is fine, because it simply means the
  // EP is not called, but for non-DLLs it means that execution starts at
  // ImageBase, executing 'MZ' as 'dec ebp/pop edx'.
  // Sample: nullEP.exe (Corkami PE Corpus).
  // The EP can also be null in the case where it is 'patched' via TLS, but this
  // applies to all cases not just when the EP is null (it's just more likely in
  // the case where it's null).
  if (!addr_of_ep && !!(nt_hdrs.GetCharacteristics() & IMAGE_FILE_DLL))
  {
    std::wcout << "\t\tWARNING! Detected zero EP in non-DLL PE.\n";
    WarnForCurrentFile(WarningType::kSuspicious);
  }
  if (addr_of_ep && !RvaToVa(process, pe_file, addr_of_ep))
  {
    // Not actually unsupported, we just want to identify potential samples for
    // now.
    std::wcout << "\t\tWARNING! Unable to resolve EP to file offset.\n";
    WarnForCurrentFile(WarningType::kUnsupported);
  }
  std::wcout << "\t\tBaseOfCode: " << std::hex << nt_hdrs.GetBaseOfCode()
             << std::dec << "\n";
#if defined(HADESMEM_DETAIL_ARCH_X86)
  std::wcout << "\t\tBaseOfData: " << std::hex << nt_hdrs.GetBaseOfData()
             << std::dec << "\n";
#endif
  ULONG_PTR const image_base = nt_hdrs.GetImageBase();
  std::wcout << "\t\tImageBase: " << std::hex << image_base << std::dec << "\n";
  // ImageBase can be null under XP. In this case the binary is relocated to
  // 0x10000.
  // Sample: ibnullXP.exe (Corkami PE corpus).
  if (!image_base)
  {
    std::wcout << "\t\tWARNING! Detected zero ImageBase.\n";
    WarnForCurrentFile(WarningType::kSuspicious);
  }
  // If ImageBase is in the kernel address range it's relocated to 0x1000.
  // Sample: ibkernel.exe (Corkami PE corpus).
  if (nt_hdrs.GetMachine() == IMAGE_FILE_MACHINE_I386 && image_base > 0x80000000UL)
  {
    std::wcout << "\t\tWARNING! Detected kernel space ImageBase.\n";
    WarnForCurrentFile(WarningType::kSuspicious);
  }
  // Not sure if this is actually possible under x64.
  // TODO: Check whether the image is allowed to load (similar to x86) in this
  // case.
  else if (nt_hdrs.GetMachine() == IMAGE_FILE_MACHINE_AMD64 && image_base > 0x80000000ULL)
  {
    std::wcout << "\t\tWARNING! Detected kernel space ImageBase.\n";
    WarnForCurrentFile(WarningType::kUnsupported);
  }
  // ImageBase must be a multiple of 0x10000
  if (!!(image_base & 0xFFFF))
  {
    std::wcout << "\t\tWARNING! Detected invalid ImageBase (not a multiple of "
                  "0x10000).\n";
    WarnForCurrentFile(WarningType::kSuspicious);
  }
  DWORD const section_alignment = nt_hdrs.GetSectionAlignment();
  std::wcout << "\t\tSectionAlignment: " << std::hex << section_alignment << std::dec << "\n";
  // Sample: bigalign.exe (Corkami PE corpus).
  // Sample: nosection*.exe (Corkami PE corpus).
  if (section_alignment < 0x200 || section_alignment > 0x1000)
  {
    std::wcout << "\t\tWARNING! Unusual section alignment.\n";
    WarnForCurrentFile(WarningType::kSuspicious);
  }
  DWORD const file_alignment = nt_hdrs.GetFileAlignment();
  std::wcout << "\t\tFileAlignment: " << std::hex << file_alignment << std::dec << "\n";
  // Sample: bigalign.exe (Corkami PE corpus).
  // Sample: nosection*.exe (Corkami PE corpus).
  if (file_alignment < 0x200 || file_alignment > 0x1000)
  {
    std::wcout << "\t\tWARNING! Unusual file alignment.\n";
    WarnForCurrentFile(WarningType::kSuspicious);
  }
  if (section_alignment < 0x800 && section_alignment != file_alignment)
  {
    std::wcout << "\t\tWARNING! Invalid alignment.\n";
    WarnForCurrentFile(WarningType::kUnsupported);
  }
  if (file_alignment > section_alignment)
  {
    std::wcout << "\t\tWARNING! Invalid alignment.\n";
    WarnForCurrentFile(WarningType::kUnsupported);
  }
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
  if (nt_hdrs.GetMajorSubsystemVersion() < 3 ||
      (nt_hdrs.GetMajorSubsystemVersion() == 3 &&
       nt_hdrs.GetMinorSubsystemVersion() < 10))
  {
    std::wcout << "\t\tWARNING! Invalid subsystem version.\n";
    WarnForCurrentFile(WarningType::kSuspicious);
  }
  std::wcout << "\t\tWin32VersionValue: " << nt_hdrs.GetWin32VersionValue()
             << "\n";
  std::wcout << "\t\tSizeOfImage: " << std::hex << nt_hdrs.GetSizeOfImage()
             << std::dec << "\n";
  std::wcout << "\t\tSizeOfHeaders: " << std::hex << nt_hdrs.GetSizeOfHeaders()
             << std::dec << "\n";
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
  DWORD const num_dirs = nt_hdrs.GetNumberOfRvaAndSizes();
  std::wcout << "\t\tNumberOfRvaAndSizes: " << nt_hdrs.GetNumberOfRvaAndSizes()
             << "\n";
  DWORD const num_dirs_clamped = GetNumberOfRvaAndSizesClamped(nt_hdrs);
  std::wcout << "\t\tNumberOfRvaAndSizes (Clamped): " << num_dirs_clamped
             << "\n";
  if (num_dirs > num_dirs_clamped)
  {
    std::wcout << "\t\tWARNING! Detected an invalid number of data directories.\n";
    WarnForCurrentFile(WarningType::kSuspicious);
  }
  for (DWORD i = 0; i < num_dirs_clamped; ++i)
  {
    auto const data_dir_va = nt_hdrs.GetDataDirectoryVirtualAddress(
      static_cast<hadesmem::PeDataDir>(i));
    auto const data_dir_size =
      nt_hdrs.GetDataDirectorySize(static_cast<hadesmem::PeDataDir>(i));
    auto const data_dir_name = GetDataDirName(i);
    std::wcout << "\t\tData Directory RVA: " << std::hex << data_dir_va << " ("
               << data_dir_name << ")\n";
    std::wcout << "\t\tData Directory Size: " << std::hex << data_dir_size
               << " (" << data_dir_name << ")\n";
  }
}
}

void DumpHeaders(hadesmem::Process const& process,
                 hadesmem::PeFile const& pe_file)
{
  DumpDosHeader(process, pe_file);

  DumpNtHeaders(process, pe_file);
}
