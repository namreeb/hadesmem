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
  DWORD const time_date_stamp = export_dir->GetTimeDateStamp();
  std::wstring time_date_stamp_str;
  if (!ConvertTimeStamp(time_date_stamp, time_date_stamp_str))
  {
    WriteNormal(out, L"WARNING! Invalid timestamp.", 2);
    WarnForCurrentFile(WarningType::kSuspicious);
  }
  WriteNamedHexSuffix(
    out, L"TimeDateStamp", time_date_stamp, time_date_stamp_str, 2);
  WriteNamedHex(out, L"MajorVersion", export_dir->GetMajorVersion(), 2);
  WriteNamedHex(out, L"MinorVersion", export_dir->GetMinorVersion(), 2);
  WriteNamedHex(out, L"Name (Raw)", export_dir->GetNameRaw(), 2);
  // Name is not guaranteed to be valid.
  // Sample: dllord.dll (Corkami PE Corpus)
  try
  {
    auto name = export_dir->GetName();
    // Export module names do not need to consist of only printable characters.
    if (!IsPrintableClassicLocale(name))
    {
      // TODO: Truncate instead of using an empty name.
      WriteNormal(out,
                  L"WARNING! Detected unprintable export module name. Using "
                  L"empty name instead.",
                  2);
      WarnForCurrentFile(WarningType::kSuspicious);
      name = "";
    }
    // Export module names are mostly unused, and so can be anything. Treat
    // anything longer than 1KB as invalid.
    else if (name.size() > 1024)
    {
      // TODO: Truncate instead of using an empty name.
      WriteNormal(out,
                  L"WARNING! Export module name is suspiciously long. Using "
                  L"empty name instead.",
                  2);
      WarnForCurrentFile(WarningType::kSuspicious);
      name = "";
    }
    WriteNamedNormal(out, L"Name", name.c_str(), 2);
  }
  catch (std::exception const& /*e*/)
  {
    WriteNormal(out, L"WARNING! Failed to read export dir name.", 2);
    WarnForCurrentFile(WarningType::kSuspicious);
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

  std::uint32_t num_exports = 0U;
  for (auto const& e : exports)
  {
    WriteNewline(out);

    // TODO: Come up with a better solution to this.
    if (num_exports++ == 1000)
    {
      WriteNormal(
        out,
        L"WARNING! Processed 1000 exports. Stopping early to avoid resource "
        L"exhaustion attacks.",
        2);
      WarnForCurrentFile(WarningType::kUnsupported);
      break;
    }

    if (e.ByName())
    {
      // TODO: Detect and warn when export names are not lexicographically
      // ordered.

      auto const name = e.GetName();
      // Export names do not need to consist of only printable characters, as
      // long as they are zero-terminated.
      // Sample: dllweirdexp.dll
      if (!IsPrintableClassicLocale(name))
      {
        // TODO: Truncate instead of using an empty name.
        WriteNormal(out,
                    L"WARNING! Detected unprintable export "
                    L"name. Using empty name instead.",
                    3);
        WarnForCurrentFile(WarningType::kSuspicious);
        WriteNamedNormal(out, L"Name", "", 3);
      }
      // Export names are mostly unused, and so can be anything. Treat anything
      // longer than 1KB as invalid.
      // Sample: dllweirdexp.dll
      else if (name.size() > 1024)
      {
        // TODO: Truncate instead of using an empty name.
        WriteNormal(out,
                    L"WARNING! Export name is suspiciously "
                    L"long. Using empty name instead.",
                    3);
        WarnForCurrentFile(WarningType::kSuspicious);
        WriteNamedNormal(out, L"Name", "", 3);
      }
      else
      {
        WriteNamedNormal(out, L"Name", name.c_str(), 3);
      }

      // PE files can have duplicate exported function names (or even have them
      // all identical) because the import hint is used to check the name first
      // before performing a search.
      // Sample: None ("Import name hint" section of "Undocumented PECOFF"
      // whitepaper).
      if (!export_names.insert(name).second)
      {
        WriteNormal(out, L"WARNING! Detected duplicate export name.", 3);
        WarnForCurrentFile(WarningType::kSuspicious);
      }
    }

    WriteNamedHex(out, L"ProcedureNumber", e.GetProcedureNumber(), 3);
    WriteNamedHex(out, L"OrdinalNumber", e.GetOrdinalNumber(), 3);

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
    else
    {
      auto const ep_rva = e.GetRva();
      WriteNamedHex(out, L"RVA", e.GetRva(), 3);
      auto const ep_va = e.GetVa();
      WriteNamedHex(out, L"VA", reinterpret_cast<std::uintptr_t>(ep_va), 3);

      if (ep_va)
      {
        DisassembleEp(process, pe_file, ep_rva, ep_va, 4);
      }
      else
      {
        WriteNormal(out, L"WARNING! Export VA is invalid.", 3);
        WarnForCurrentFile(WarningType::kSuspicious);
      }
    }
  }
}
