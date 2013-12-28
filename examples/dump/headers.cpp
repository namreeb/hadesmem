// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "headers.hpp"

#include <iostream>

#include <hadesmem/config.hpp>
#include <hadesmem/pelib/dos_header.hpp>
#include <hadesmem/pelib/nt_headers.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>

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
  std::wcout << "\t\tFileAlignment: " << std::hex << nt_hdrs.GetFileAlignment()
             << std::dec << "\n";
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
