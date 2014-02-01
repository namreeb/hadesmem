// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "imports.hpp"

#include <iostream>
#include <iterator>

#include <hadesmem/pelib/bound_import_desc.hpp>
#include <hadesmem/pelib/bound_import_desc_list.hpp>
#include <hadesmem/pelib/bound_import_fwd_ref.hpp>
#include <hadesmem/pelib/bound_import_fwd_ref_list.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>

#include "main.hpp"
#include "print.hpp"
#include "warning.hpp"

namespace
{

bool HasBoundImportDir(hadesmem::Process const& process,
                       hadesmem::PeFile const& pe_file)
{
  hadesmem::NtHeaders const nt_headers(process, pe_file);
  // Intentionally not checking whether the RVA is valid, because we will detect
  // an empty list in that case, at which point we want to warn because an
  // invalid RVA is suspicious (even though it won't stop the file from
  // loading).
  return (nt_headers.GetNumberOfRvaAndSizes() >
            static_cast<int>(hadesmem::PeDataDir::BoundImport) &&
          nt_headers.GetDataDirectoryVirtualAddress(
            hadesmem::PeDataDir::BoundImport));
}
}

void DumpBoundImports(hadesmem::Process const& process,
                      hadesmem::PeFile const& pe_file,
                      bool has_new_bound_imports_any)
{
  std::wostream& out = std::wcout;

  if (!HasBoundImportDir(process, pe_file))
  {
    // Sample: dllmaxvals.dll (Corkami PE Corpus)
    if (has_new_bound_imports_any)
    {
      WriteNewline(out);
      WriteNormal(
        out,
        L"WARNING! No bound import directory on file with an import dir "
        L"indicating the presence of a bound import dir.",
        1);
      WarnForCurrentFile(WarningType::kSuspicious);
    }

    return;
  }

  if (!has_new_bound_imports_any)
  {
    WriteNewline(out);
    WriteNormal(
      out,
      L"WARNING! Seemingly valid bound import directory on file with an "
      L"import dir indicating no new bound import dir.",
      1);
    WarnForCurrentFile(WarningType::kSuspicious);
    return;
  }

  hadesmem::BoundImportDescriptorList const bound_import_descs(process,
                                                               pe_file);

  if (std::begin(bound_import_descs) != std::end(bound_import_descs))
  {
    WriteNewline(out);
    WriteNormal(out, L"Bound Import Descriptors:", 1);
  }
  else
  {
    WriteNewline(out);
    WriteNormal(out, L"WARNING! Empty or invalid bound import directory.", 1);
    WarnForCurrentFile(WarningType::kSuspicious);
  }

  std::uint32_t num_descs = 0U;
  for (auto const& desc : bound_import_descs)
  {
    WriteNewline(out);

    if (num_descs++ == 1000)
    {
      WriteNormal(out,
                  L"WARNING! Processed 1000 bound import descriptors. Stopping "
                  L"early to avoid resource exhaustion attacks.",
                  2);
      WarnForCurrentFile(WarningType::kUnsupported);
      break;
    }

    DWORD const time_date_stamp = desc.GetTimeDateStamp();
    std::wstring time_date_stamp_str;
    if (!ConvertTimeStamp(time_date_stamp, time_date_stamp_str))
    {
      WriteNormal(out, L"WARNING! Invalid timestamp.", 2);
      WarnForCurrentFile(WarningType::kSuspicious);
    }
    WriteNamedHexSuffix(
      out, L"TimeDateStamp", time_date_stamp, time_date_stamp_str, 2);
    WriteNamedHex(out, L"OffsetModuleName", desc.GetOffsetModuleName(), 2);
    WriteNamedNormal(out, L"ModuleName", desc.GetModuleName().c_str(), 2);
    WriteNamedHex(out,
                  L"NumberOfModuleForwarderRefs",
                  desc.GetNumberOfModuleForwarderRefs(),
                  2);
    hadesmem::BoundImportForwarderRefList const forwarder_refs(
      process, pe_file, desc);
    if (std::begin(forwarder_refs) != std::end(forwarder_refs))
    {
      WriteNewline(out);
      WriteNormal(out, L"Module Forwarder Refs:", 2);
    }
    for (auto const& forwarder : forwarder_refs)
    {
      WriteNewline(out);

      DWORD const fwd_time_date_stamp = forwarder.GetTimeDateStamp();
      std::wstring fwd_time_date_stamp_str;
      if (!ConvertTimeStamp(fwd_time_date_stamp, fwd_time_date_stamp_str))
      {
        WriteNormal(out, L"WARNING! Invalid timestamp.", 3);
        WarnForCurrentFile(WarningType::kSuspicious);
      }
      WriteNamedHexSuffix(
        out, L"TimeDateStamp", fwd_time_date_stamp, fwd_time_date_stamp_str, 3);
      WriteNamedHex(
        out, L"OffsetModuleName", forwarder.GetOffsetModuleName(), 3);
      WriteNamedNormal(
        out, L"ModuleName", forwarder.GetModuleName().c_str(), 3);
      WriteNamedHex(out, L"Reserved", forwarder.GetReserved(), 3);
    }
  }
}
