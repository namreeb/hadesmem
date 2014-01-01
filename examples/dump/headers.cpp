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
  std::wcout << std::dec << "\n";
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
  std::wcout << "\t\tNewHeaderOffset: " << std::hex
             << dos_hdr.GetNewHeaderOffset() << std::dec << "\n";
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
  std::wcout << "\t\tAddressOfEntryPoint: " << std::hex
             << nt_hdrs.GetAddressOfEntryPoint() << std::dec << "\n";
  // Entry point can be null. For DLLs this is fine, because it simply means the
  // EP is not called, but for non-DLLs it means that execution starts at
  // ImageBase, executing 'MZ' as 'dec ebp/pop edx'.
  // Sample: nullEP.exe (Corkami PE Corpus).
  if (!nt_hdrs.GetAddressOfEntryPoint() &&
      !!(nt_hdrs.GetCharacteristics() & IMAGE_FILE_DLL))
  {
    std::wcout << "\t\tWARNING! Detected zero EP in non-DLL PE.\n";
    WarnForCurrentFile(WarningType::kSuspicious);
  }
  if (!RvaToVa(process, pe_file, nt_hdrs.GetAddressOfEntryPoint()))
  {
    std::wcout << "\t\tWARNING! Unable to resolve EP to file offset.\n";
    WarnForCurrentFile(WarningType::kSuspicious);
  }
  std::wcout << "\t\tBaseOfCode: " << std::hex << nt_hdrs.GetBaseOfCode()
             << std::dec << "\n";
#if defined(HADESMEM_DETAIL_ARCH_X86)
  std::wcout << "\t\tBaseOfData: " << std::hex << nt_hdrs.GetBaseOfData()
             << std::dec << "\n";
#endif
  std::wcout << "\t\tImageBase: " << std::hex << nt_hdrs.GetImageBase()
             << std::dec << "\n";
  // ImageBase can be null under XP. In this case the binary is relocated to
  // 0x10000.
  // Sample: ibnullXP.exe (Corkami PE corpus).
  if (!nt_hdrs.GetImageBase())
  {
    std::wcout << "\t\tWARNING! Detected zero ImageBase.\n";
    WarnForCurrentFile(WarningType::kSuspicious);
  }
  // If ImageBase is in the kernel address range it's relocated to 0x1000.
  // Sample: ibkernel.exe (Corkami PE corpus).
  // TODO: Check whether this also occurs under x64, and add an equivalent check
  // if applicable.
  if (nt_hdrs.GetImageBase() > 0x80000000UL &&
      nt_hdrs.GetMachine() == IMAGE_FILE_MACHINE_I386)
  {
    std::wcout << "\t\tWARNING! Detected kernel space ImageBase.\n";
    WarnForCurrentFile(WarningType::kSuspicious);
  }
  // ImageBase must be a multiple of 0x10000
  if (!!(nt_hdrs.GetImageBase() & 0xFFFF))
  {
    std::wcout << "\t\tWARNING! Detected invalid ImageBase (not a multiple of "
                  "0x10000).\n";
    WarnForCurrentFile(WarningType::kSuspicious);
  }
  std::wcout << "\t\tSectionAlignment: " << std::hex
             << nt_hdrs.GetSectionAlignment() << std::dec << "\n";
  // Sample: bigalign.exe (Corkami PE corpus).
  // Sample: nosection*.exe (Corkami PE corpus).
  if (nt_hdrs.GetSectionAlignment() < 0x200 ||
      nt_hdrs.GetSectionAlignment() > 0x1000)
  {
    std::wcout << "\t\tWARNING! Unusual section alignment.\n";
    WarnForCurrentFile(WarningType::kSuspicious);
  }
  std::wcout << "\t\tFileAlignment: " << std::hex << nt_hdrs.GetFileAlignment()
             << std::dec << "\n";
  // Sample: bigalign.exe (Corkami PE corpus).
  // Sample: nosection*.exe (Corkami PE corpus).
  if (nt_hdrs.GetFileAlignment() < 0x200 || nt_hdrs.GetFileAlignment() > 0x1000)
  {
    std::wcout << "\t\tWARNING! Unusual file alignment.\n";
    WarnForCurrentFile(WarningType::kSuspicious);
  }
  if (nt_hdrs.GetSectionAlignment() < 0x800 &&
      nt_hdrs.GetSectionAlignment() != nt_hdrs.GetFileAlignment())
  {
    std::wcout << "\t\tWARNING! Invalid alignment.\n";
    WarnForCurrentFile(WarningType::kUnsupported);
  }
  if (nt_hdrs.GetFileAlignment() > nt_hdrs.GetSectionAlignment())
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
  std::wcout << "\t\tNumberOfRvaAndSizes: " << nt_hdrs.GetNumberOfRvaAndSizes()
             << "\n";
  DWORD num_dirs = GetNumberOfRvaAndSizesClamped(nt_hdrs);
  std::wcout << "\t\tNumberOfRvaAndSizes (Clamped): " << num_dirs << "\n";
  for (DWORD i = 0; i < num_dirs; ++i)
  {
    std::wcout << "\t\tDataDirectoryVirtualAddress: " << std::hex
               << nt_hdrs.GetDataDirectoryVirtualAddress(
                    static_cast<hadesmem::PeDataDir>(i)) << std::dec << "\n";
    std::wcout << "\t\tDataDirectorySize: " << std::hex
               << nt_hdrs.GetDataDirectorySize(
                    static_cast<hadesmem::PeDataDir>(i)) << std::dec << "\n";
  }
}
}

void DumpHeaders(hadesmem::Process const& process,
                 hadesmem::PeFile const& pe_file)
{
  DumpDosHeader(process, pe_file);

  DumpNtHeaders(process, pe_file);
}
