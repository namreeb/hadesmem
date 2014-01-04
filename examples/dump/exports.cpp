// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "exports.hpp"

#include <iostream>
#include <memory>
#include <set>

#include <hadesmem/detail/str_conv.hpp>
#include <hadesmem/pelib/export.hpp>
#include <hadesmem/pelib/export_dir.hpp>
#include <hadesmem/pelib/export_list.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>

#include "main.hpp"

// TODO: Fix this for the case where the export dir is seemingly
// corrupted/invalid, but actually has legitimate (and working) exports.
// Sample: dllord.dll (Corkami PE Corpus)

void DumpExports(hadesmem::Process const& process,
                 hadesmem::PeFile const& pe_file)
{
  std::unique_ptr<hadesmem::ExportDir> export_dir;
  try
  {
    export_dir = std::make_unique<hadesmem::ExportDir>(process, pe_file);
  }
  catch (std::exception const& /*e*/)
  {
    return;
  }

  std::wcout << "\n\tExport Dir:\n";

  std::wcout << "\n";
  std::wcout << "\t\tCharacteristics: " << std::hex
             << export_dir->GetCharacteristics() << std::dec << "\n";
  std::wcout << "\t\tTimeDateStamp: " << std::hex
             << export_dir->GetTimeDateStamp() << std::dec << "\n";
  std::wcout << "\t\tMajorVersion: " << std::hex
             << export_dir->GetMajorVersion() << std::dec << "\n";
  std::wcout << "\t\tMinorVersion: " << std::hex
             << export_dir->GetMinorVersion() << std::dec << "\n";
  std::wcout << "\t\tName (Raw): " << std::hex << export_dir->GetNameRaw()
             << std::dec << "\n";
  // Name is not guaranteed to be valid?
  // TODO: Find a sample for this before relaxing the warining back to
  // kSuspicious.
  try
  {
    // Export dir name does not need to consist of only printable characters, as
    // long as it is zero-terminated.
    // Sample: dllweirdexp.dll (Corkami PE Corpus)
    // TODO: Find a solution to the above case, and perhaps use a vector<char>
    // instead of a string in the cases where the name isn't printable.
    // TODO: Detect and handle the case where the string is terminated
    // virtually.
    // TODO: Detect and handle the case where the string is terminated by EOF
    // (virtual termination special case).
    std::wcout << "\t\tName: " << export_dir->GetName().c_str() << "\n";
  }
  catch (std::exception const& /*e*/)
  {
    std::wcout << "\t\tWARNING! Failed to read export dir name.\n";
    WarnForCurrentFile(WarningType::kUnsupported);
  }
  std::wcout << "\t\tOrdinalBase: " << std::hex << export_dir->GetOrdinalBase()
             << std::dec << "\n";
  std::wcout << "\t\tNumberOfFunctions: " << std::hex
             << export_dir->GetNumberOfFunctions() << std::dec << "\n";
  std::wcout << "\t\tNumberOfNames: " << std::hex
             << export_dir->GetNumberOfNames() << std::dec << "\n";
  std::wcout << "\t\tAddressOfFunctions: " << std::hex
             << export_dir->GetAddressOfFunctions() << std::dec << "\n";
  std::wcout << "\t\tAddressOfNames: " << std::hex
             << export_dir->GetAddressOfNames() << std::dec << "\n";
  std::wcout << "\t\tAddressOfNameOrdinals: " << std::hex
             << export_dir->GetAddressOfNameOrdinals() << std::dec << "\n";

  std::set<std::string> export_names;

  hadesmem::ExportList exports(process, pe_file);
  if (std::begin(exports) != std::end(exports))
  {
    std::wcout << "\n\tExports:\n";
  }
  else
  {
    std::wcout << "\n\tWARNING! Empty or invalid export list.\n";
    WarnForCurrentFile(WarningType::kUnsupported);
  }
  for (auto const& e : exports)
  {
    std::wcout << "\n";
    std::wcout << "\t\tRVA: " << std::hex << e.GetRva() << std::dec << "\n";
    std::wcout << "\t\tVA: " << hadesmem::detail::PtrToHexString(e.GetVa())
               << "\n";

    if (e.ByName())
    {
      // Export names do not need to consist of only printable characters, as
      // long as they are zero-terminated.
      // Sample: dllweirdexp.dll
      // TODO: Find a solution to the above case, and perhaps use a vector<char>
      // instead of a string in the cases where the name isn't printable.
      // TODO: Detect and handle the case where the string is terminated
      // virtually.
      // TODO: Detect and handle the case where the string is EOF terminated.
      std::wcout << "\t\tName: " << e.GetName().c_str() << "\n";
      // PE files can have duplicate exported function names (or even have them
      // all identical) because the import hint is used to check the name first
      // before performing a search.
      // Sample: None ("Import name hint" section of "Undocumented PECOFF"
      // whitepaper).
      if (!export_names.insert(e.GetName()).second)
      {
        std::wcout << "\t\tWARNING! Detected duplicate export name.\n";
        WarnForCurrentFile(WarningType::kSuspicious);
      }
    }
    else if (e.ByOrdinal())
    {
      std::wcout << "\t\tProcedureNumber: " << std::hex
                 << e.GetProcedureNumber() << std::dec << "\n";
      std::wcout << "\t\tOrdinalNumber: " << std::hex << e.GetOrdinalNumber()
                 << std::dec << "\n";
    }
    else
    {
      std::wcout << "\t\tWARNING! Entry not exported by name or ordinal.\n";
      WarnForCurrentFile(WarningType::kUnsupported);
    }

    if (e.IsForwarded())
    {
      std::wcout << "\t\tForwarder: " << e.GetForwarder().c_str() << "\n";
      std::wcout << "\t\tForwarderModule: " << e.GetForwarderModule().c_str()
                 << "\n";
      std::wcout << "\t\tForwarderFunction: "
                 << e.GetForwarderFunction().c_str() << "\n";
      std::wcout << "\t\tIsForwardedByOrdinal: " << e.IsForwardedByOrdinal()
                 << "\n";
      if (e.IsForwardedByOrdinal())
      {
        try
        {
          std::wcout << "\t\tForwarderOrdinal: " << std::hex
                     << e.GetForwarderOrdinal() << std::dec << "\n";
        }
        catch (std::exception const& /*e*/)
        {
          std::wcout << "\t\tWARNING! ForwarderOrdinal invalid.\n";
          WarnForCurrentFile(WarningType::kSuspicious);
        }
      }
    }
  }
}
