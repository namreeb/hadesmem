// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "sections.hpp"

#include <iostream>
#include <iterator>

#include <hadesmem/pelib/section.hpp>
#include <hadesmem/pelib/section_list.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>

#include "main.hpp"

// TODO: Detect when a file has no sections.

// TODO: Detect when a file has more than 96 sections (allowed on Vista+, but
// technically outside of the spec).

void DumpSections(hadesmem::Process const& process,
                  hadesmem::PeFile const& pe_file)
{
  hadesmem::SectionList sections(process, pe_file);

  if (std::begin(sections) != std::end(sections))
  {
    std::wcout << "\n\tSections:\n";
  }

  for (auto const& s : sections)
  {
    std::wcout << "\n";
    if (s.IsVirtual())
    {
      std::wcout << "\t\tWARNING! Section is virtual.\n";
      WarnForCurrentFile(WarningType::kSuspicious);
    }
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
    std::wcout << "\t\tCharacteristics: " << std::hex << s.GetCharacteristics()
               << std::dec << "\n";
  }
}
