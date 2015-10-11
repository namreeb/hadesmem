// Copyright (C) 2010-2015 Joshua Boyce
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

#include "disassemble.hpp"
#include "main.hpp"
#include "print.hpp"
#include "warning.hpp"

void DumpExports(hadesmem::Process const& process,
                 hadesmem::PeFile const& pe_file)
{
  std::unique_ptr<hadesmem::ExportDir const> export_dir;
  try
  {
    export_dir = std::make_unique<hadesmem::ExportDir const>(process, pe_file);
  }
  catch (std::exception const& /*e*/)
  {
    return;
  }

  std::wostream& out = GetOutputStreamW();

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
    HandleLongOrUnprintableString(
      L"Name", L"export module name", 2, WarningType::kSuspicious, name);
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

  hadesmem::ExportList const exports(process, pe_file);
  if (std::begin(exports) != std::end(exports))
  {
    WriteNewline(out);
    WriteNormal(out, L"Exports:", 2);
  }
  else
  {
    // Legitimate DLLs have an export dir with no exports for some reason (e.g.
    // visintl.dll from Office 15).
    if (export_dir->GetNumberOfFunctions() == 0)
    {
      WriteNewline(out);
      WriteNormal(out, L"WARNING! Empty export list.", 2);
      WarnForCurrentFile(WarningType::kSuspicious);
    }
    // Legitimate DLLs have an export dir with seemingly garbage values (e.g.
    // Xvid-1.3.2-20110601.exe from Helldorado, efte.exe from Exodus from the
    // Earth). Not yet sure why they do this, but it's allowed because the
    // export directory isn't used by the PE loader.
    else
    {
      WriteNewline(out);
      WriteNormal(out, L"WARNING! Invalid export list.", 2);
      WarnForCurrentFile(WarningType::kSuspicious);
    }
  }

  // TODO: Detect and warn when export names are not lexicographically ordered.
  std::uint32_t num_exports = 0U;
  for (auto const& e : exports)
  {
    WriteNewline(out);

    // Some legitimate PE files have well over 10000 exports (e.g.
    // libgnat-4.9.dll).
    if (num_exports++ == 100000)
    {
      WriteNormal(
        out,
        L"WARNING! Processed 100000 exports. Stopping early to avoid resource "
        L"exhaustion attacks.",
        2);
      WarnForCurrentFile(WarningType::kUnsupported);
      break;
    }

    if (e.ByName())
    {
      auto const name = e.GetName();
      // Sample: dllweirdexp.dll
      HandleLongOrUnprintableString(
        L"Name", L"export name", 3, WarningType::kSuspicious, name);

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
        // TODO: Fix case where we're disassembling an image, which may have its
        // EAT hooked, so our calculations on the number of bytes to read until
        // the end of file is actually wrong.
        DisassembleEp(process, pe_file, ep_rva, ep_va, 4);
      }
      else if (!e.IsVirtualVa())
      {
        WriteNormal(out, L"WARNING! Export VA is invalid.", 3);
        WarnForCurrentFile(WarningType::kSuspicious);
      }
    }
  }
}
