// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "exports.hpp"

#include <iostream>
#include <memory>

#include <hadesmem/detail/str_conv.hpp>
#include <hadesmem/pelib/export.hpp>
#include <hadesmem/pelib/export_dir.hpp>
#include <hadesmem/pelib/export_list.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>

#include "main.hpp"

// TODO: Detect multiple exports with the same name (used as part of import hint
// trick).

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
  // Name is not guaranteed to be valid.
  try
  {
    std::wcout << "\t\tName: " << export_dir->GetName().c_str() << "\n";
  }
  catch (std::exception const& /*e*/)
  {
    std::wcout << "\t\tWARNING! Name is invalid.\n";
    WarnForCurrentFile(WarningType::kSuspicious);
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

  std::wcout << "\n\tExports:\n";

  hadesmem::ExportList exports(process, pe_file);
  for (auto const& e : exports)
  {
    std::wcout << "\n";
    std::wcout << "\t\tRVA: " << std::hex << e.GetRva() << std::dec << "\n";
    std::wcout << "\t\tVA: " << hadesmem::detail::PtrToHexString(e.GetVa())
               << "\n";
    if (e.ByName())
    {
      std::wcout << "\t\tName: " << e.GetName().c_str() << "\n";
    }
    else if (e.ByOrdinal())
    {
      std::wcout << "\t\tProcedureNumber: " << e.GetProcedureNumber() << "\n";
      std::wcout << "\t\tOrdinalNumber: " << e.GetOrdinalNumber() << "\n";
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
          std::wcout << "\t\tForwarderOrdinal: " << e.GetForwarderOrdinal()
                     << "\n";
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
