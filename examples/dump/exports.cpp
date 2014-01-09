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

  std::wostream& out = std::wcout;

  WriteNewline(out);
  WriteNormal(out, L"Export Dir:", 1);
  WriteNewline(out);

  WriteNamedHex(out, L"Characteristics", export_dir->GetCharacteristics(), 2);
  WriteNamedHex(out, L"TimeDateStamp", export_dir->GetTimeDateStamp(), 2);
  WriteNamedHex(out, L"MajorVersion", export_dir->GetMajorVersion(), 2);
  WriteNamedHex(out, L"MinorVersion", export_dir->GetMinorVersion(), 2);
  WriteNamedHex(out, L"Name (Raw)", export_dir->GetNameRaw(), 2);
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
    WriteNamedNormal(out, L"Name", export_dir->GetName().c_str(), 2);
  }
  catch (std::exception const& /*e*/)
  {
    WriteNormal(out, L"WARNING! Failed to read export dir name.", 2);
    WarnForCurrentFile(WarningType::kUnsupported);
  }
  WriteNamedHex(out, L"OrdinalBase", export_dir->GetOrdinalBase(), 2);
  WriteNamedHex(
    out, L"NumberOfFunctions", export_dir->GetNumberOfFunctions(), 2);
  WriteNamedHex(out, L"NumberOfNames", export_dir->GetNumberOfNames(), 2);
  WriteNamedHex(
    out, L"AddressOfFunctions", export_dir->GetAddressOfFunctions(), 2);
  WriteNamedHex(out, L"AddressOfNames", export_dir->GetAddressOfNames(), 2);
  WriteNamedHex(
    out, L"AddressOfNameOrdinals", export_dir->GetAddressOfNameOrdinals(), 2);

  std::set<std::string> export_names;

  hadesmem::ExportList exports(process, pe_file);
  if (std::begin(exports) != std::end(exports))
  {
    WriteNewline(out);
    WriteNormal(out, L"Exports:", 2);
  }
  else
  {
    WriteNewline(out);
    WriteNormal(out, L"WARNING! Empty or invalid export list.", 2);
    WarnForCurrentFile(WarningType::kUnsupported);
  }
  for (auto const& e : exports)
  {
    WriteNewline(out);
    WriteNamedHex(out, L"RVA", e.GetRva(), 3);
    WriteNamedHex(out, L"VA", reinterpret_cast<std::uintptr_t>(e.GetVa()), 3);
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
      // TODO: Detect and warn when export names are not lexicographically
      // ordered.
      WriteNamedNormal(out, L"Name", e.GetName().c_str(), 3);
      // PE files can have duplicate exported function names (or even have them
      // all identical) because the import hint is used to check the name first
      // before performing a search.
      // Sample: None ("Import name hint" section of "Undocumented PECOFF"
      // whitepaper).
      if (!export_names.insert(e.GetName()).second)
      {
        WriteNormal(out, L"WARNING! Detected duplicate export name.", 3);
        WarnForCurrentFile(WarningType::kSuspicious);
      }
    }
    else if (e.ByOrdinal())
    {
      WriteNamedHex(out, L"ProcedureNumber", e.GetProcedureNumber(), 3);
      WriteNamedHex(out, L"OrdinalNumber", e.GetOrdinalNumber(), 3);
    }
    else
    {
      WriteNormal(out, L"WARNING! Entry not exported by name or ordinal.", 3);
      WarnForCurrentFile(WarningType::kUnsupported);
    }

    if (e.IsForwarded())
    {
      WriteNamedNormal(out, L"Forwarder", e.GetForwarder().c_str(), 3);
      WriteNamedNormal(
        out, L"ForwarderModule", e.GetForwarderModule().c_str(), 3);
      WriteNamedNormal(
        out, L"ForwarderFunction", e.GetForwarderFunction().c_str(), 3);
      WriteNamedNormal(
        out, L"IsForwardedByOrdinal", e.IsForwardedByOrdinal(), 3);
      if (e.IsForwardedByOrdinal())
      {
        try
        {
          WriteNamedHex(out, L"ForwarderOrdinal", e.GetForwarderOrdinal(), 3);
        }
        catch (std::exception const& /*e*/)
        {
          WriteNormal(out, L"WARNING! ForwarderOrdinal invalid.", 3);
          WarnForCurrentFile(WarningType::kSuspicious);
        }
      }
    }
  }
}
