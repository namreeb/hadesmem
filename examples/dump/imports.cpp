// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "imports.hpp"

#include <iostream>
#include <iterator>

#include <hadesmem/pelib/bound_import_desc_list.hpp>
#include <hadesmem/pelib/import_dir.hpp>
#include <hadesmem/pelib/import_dir_list.hpp>
#include <hadesmem/pelib/import_thunk.hpp>
#include <hadesmem/pelib/import_thunk_list.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>

#include "main.hpp"
#include "print.hpp"
#include "warning.hpp"

// TODO: For the case where an image has new style bound imports, but an invalid
// ILT… Is this allowed? I guess this is legal in the case where you bind a DLL
// that doesn't have an ILT to begin wtih, at which point it won't load if the
// bindings don't match, but we need to confirm this. Warn so we can find
// samples for further investigation.

// TODO: Check whether a new style forwarder chain with old style bound imports
// is actually allowed/loaded.

// TODO: Check whether a new style forwarder chain with no bound imports is
// actually allowed/loaded.

// TODO: Check whether a new style forwarder chain with no bound imports is
// actually allowed/loaded.

// TODO: Parse the IAT and ILT in parallel, in order to easily detect when
// imports are bound in-memory. This will also mean we no longer need to count
// the length of the ILT in order to terminate the IAT pass early.

// TODO: Should probably revert to using 'is_ilt_bound' instead of hardcoding
// false for the initial DumpImportThunk loop, but is it even legal to have a
// module that uses old style bindings with no ILT? Need to investigate, because
// it seems you're allowed to have modules like that when they're not actually
// bound, and the loader simply detects that the TimeDateStamp doesn't match and
// so treats the IAT as unbound? Investigate this further.

// TODO: For the case where we detect an invalid ILT RVA and stop enumeration
// entirely, verify this is correct. Probably easiest just to hot-patch the
// Corkami samples to give them a random invalid RVA and see if they still run.
// (I believe that they do, so our code is wrong!)

// TODO: Check that the logic for the second DumpImportThunk loop is correct.
// ("(is_iat_bound && ilt_valid) || !ilt_empty")

namespace
{
bool HasValidNonEmptyBoundImportDescList(hadesmem::Process const& process,
                                         hadesmem::PeFile const& pe_file)
{
  hadesmem::BoundImportDescriptorList const bound_import_dirs(process, pe_file);

  return (std::begin(bound_import_dirs) != std::end(bound_import_dirs));
}

void DumpImportThunk(hadesmem::ImportThunk const& thunk, bool is_bound)
{
  std::wostream& out = GetOutputStreamW();

  WriteNewline(out);

  bool const by_ordinal = thunk.ByOrdinal();

  // This check needs to be first, because it's possible to have invalid data in
  // the IAT (i.e. -1) which will cause ByOrdinal to be true!
  if (is_bound)
  {
    WriteNamedHex(out, L"Function", thunk.GetFunction(), 3);
  }
  else if (by_ordinal)
  {
    WriteNamedHex(out, L"OrdinalRaw", thunk.GetOrdinalRaw(), 3);
    WriteNamedHex(out, L"Ordinal", thunk.GetOrdinal(), 3);
  }
  else
  {
    try
    {
      WriteNamedHex(out, L"AddressOfData", thunk.GetAddressOfData(), 3);
      WriteNamedHex(out, L"Hint", thunk.GetHint(), 3);
      auto const name = thunk.GetName();
      // Sample: dllweirdexp-ld.exe
      HandleLongOrUnprintableString(
        L"Name", L"import thunk name data", 3, WarningType::kSuspicious, name);
    }
    catch (std::exception const& /*e*/)
    {
      WriteNormal(out, L"WARNING! Invalid import thunk name data.", 3);
      WarnForCurrentFile(WarningType::kUnsupported);
    }
  }
}
}

void DumpImports(hadesmem::Process const& process,
                 hadesmem::PeFile const& pe_file,
                 bool& has_new_bound_imports_any)
{
  std::wostream& out = GetOutputStreamW();

  hadesmem::ImportDirList const import_dirs(process, pe_file);

  if (std::begin(import_dirs) != std::end(import_dirs))
  {
    WriteNewline(out);
    WriteNormal(out, L"Import Dirs:", 1);
  }
  else
  {
    hadesmem::NtHeaders const nt_headers{process, pe_file};
    // Only warn if the file actually has an import directory RVA (and hence it
    // appears that the RVA resolution fails).
    // Don't check size because Windows ignores it.
    if (nt_headers.GetNumberOfRvaAndSizesClamped() >
          static_cast<DWORD>(hadesmem::PeDataDir::Import) &&
        nt_headers.GetDataDirectoryVirtualAddress(hadesmem::PeDataDir::Import))
    {
      // This is probably a good thing to use to quickly identify files with
      // broken RVA resolution (because broken RVA resolution is far more
      // common than actual files with no imports).
      WriteNewline(out);
      WriteNormal(out, L"WARNING! Empty or invalid import directory.", 1);
      WarnForCurrentFile(WarningType::kUnsupported);
    }
  }

  std::uint32_t num_import_dirs = 0U;
  for (auto const& dir : import_dirs)
  {
    WriteNewline(out);

    if (dir.IsVirtualTerminated())
    {
      WriteNormal(
        out,
        L"WARNING! Detected virtual termination trick. Stopping enumeration.",
        2);
      WarnForCurrentFile(WarningType::kSuspicious);
      break;
    }

    if (dir.IsTlsAoiTerminated())
    {
      WriteNormal(out,
                  L"WARNING! Detected TLS AOI trick! Assuming a "
                  L"Windows 7 style loader and stopping enumeration early.",
                  2);
      WarnForCurrentFile(WarningType::kSuspicious);
      break;
    }

    if (dir.IsVirtualBegin())
    {
      WriteNormal(
        out, L"WARNING! Detected virtual descriptor overlap trick.", 2);
      WarnForCurrentFile(WarningType::kSuspicious);
    }

    if (num_import_dirs++ == 1000)
    {
      WriteNormal(
        out,
        L"WARNING! Processed 1000 import dirs. Stopping early to avoid "
        L"resource exhaustion attacks.",
        2);
      WarnForCurrentFile(WarningType::kUnsupported);
      break;
    }

    DWORD const iat = dir.GetFirstThunk();
    bool const iat_valid = !!hadesmem::RvaToVa(process, pe_file, iat);
    DWORD const ilt = dir.GetOriginalFirstThunk();
    bool const ilt_valid = !!hadesmem::RvaToVa(process, pe_file, ilt);

    {
      // If the IAT is empty then the descriptor is skipped, and the name can
      // be invalid because it's ignored. Note that we simply skip here rather
      // than terminate, because it's possible to have such 'invalid' entries
      // in-between real entries.
      hadesmem::ImportThunkList const iat_thunks(process, pe_file, iat);
      if (std::begin(iat_thunks) == std::end(iat_thunks))
      {
        if (!ilt_valid)
        {
          WriteNormal(out,
                      L"WARNING! IAT is " +
                        std::wstring(iat_valid ? L"empty" : L"invalid") +
                        L". Skipping directory.",
                      2);
          WarnForCurrentFile(iat_valid ? WarningType::kSuspicious
                                       : WarningType::kUnsupported);
          continue;
        }

        // Scylla generates files like this (xcoronahost.exe for 64-bit XignCode
        // used on BDO KR) and IDA is able to parse them correctly. Although
        // files like this won't run (I think??) we should still support reading
        // them for the purpose of reverse-engineering dump files which may
        // contain malformed data.
        // TODO: Figure out the correct way to fix this.
        WriteNormal(out,
                    L"WARNING! IAT is " +
                      std::wstring(iat_valid ? L"empty" : L"invalid") +
                      L". Proceeding due to seemingly valid ILT.",
                    2);
      }
    }

    bool const use_ilt = !!ilt && ilt != iat;
    hadesmem::ImportThunkList const ilt_thunks(
      process, pe_file, use_ilt ? ilt : iat);
    bool const ilt_empty = std::begin(ilt_thunks) == std::end(ilt_thunks);

    // Apparently it's okay for the ILT to be invalid and 0xFFFFFFFF or 0. This
    // is handled below in our ILT valid/empty checks (after dumping the dir
    // data, but before dumping the thunks).
    // Sample: maxvals.exe (Corkami PE Corpus)
    // Sample: dllmaxvals.dll (Corkami PE Corpus)
    // For anything else though treat the directory as invalid and stop.
    if (!ilt_valid && ilt != 0xFFFFFFFF && ilt != 0)
    {
      WriteNormal(
        out, L"WARNING! ILT is extra invalid. Stopping enumeration.", 2);
      WarnForCurrentFile(WarningType::kUnsupported);
      break;
    }

    WriteNamedHex(out, L"OriginalFirstThunk", dir.GetOriginalFirstThunk(), 2);
    DWORD const time_date_stamp = dir.GetTimeDateStamp();
    std::wstring time_date_stamp_str;
    if (!ConvertTimeStamp(time_date_stamp, time_date_stamp_str))
    {
      WriteNormal(out, L"WARNING! Invalid timestamp.", 2);
      WarnForCurrentFile(WarningType::kSuspicious);
    }
    WriteNamedHexSuffix(
      out, L"TimeDateStamp", time_date_stamp, time_date_stamp_str, 2);

    bool const has_new_bound_imports =
      (time_date_stamp == static_cast<DWORD>(-1));
    has_new_bound_imports_any = has_new_bound_imports;
    if (has_new_bound_imports)
    {
      // Don't just check whether the ILT is invalid, but also ensure that
      // there's a valid bound import dir. In the case where the bound import
      // dir is invalid we just treat the IAT as the ILT on disk. See
      // dllmaxvals.dll for a PE file which has TimeDateStamp of 0xFFFFFFFF, no
      // ILT, and no bound import dir.
      if (!ilt_valid && HasValidNonEmptyBoundImportDescList(process, pe_file))
      {
        WriteNormal(
          out,
          L"WARNING! Detected new style bound imports with an invalid ILT.",
          2);
        WarnForCurrentFile(WarningType::kUnsupported);
      }
    }

    // TODO: Add support for dumping old style bound imports.

    WriteNamedHex(out, L"ForwarderChain", dir.GetForwarderChain(), 2);

    WriteNamedHex(out, L"Name (Raw)", dir.GetNameRaw(), 2);

    try
    {
      auto imp_desc_name = dir.GetName();
      HandleLongOrUnprintableString(L"Name",
                                    L"import descriptor name",
                                    2,
                                    WarningType::kSuspicious,
                                    imp_desc_name);
    }
    catch (std::exception const& /*e*/)
    {
      WriteNormal(out, L"WARNING! Failed to read import dir name.", 2);
      WarnForCurrentFile(WarningType::kUnsupported);
    }

    WriteNamedHex(out, L"FirstThunk", dir.GetFirstThunk(), 2);

    if (ilt_empty)
    {
      // Has to be the ILT if we get here because we did a check for an
      // empty/invalid IAT earlier on.

      if (!ilt_valid)
      {
        WriteNewline(out);
        WriteNormal(out, L"WARNING! ILT is invalid.", 2);
        WarnForCurrentFile(WarningType::kSuspicious);
      }
      else
      {
        WriteNewline(out);
        WriteNormal(out, L"WARNING! ILT is empty.", 2);
        WarnForCurrentFile(WarningType::kUnsupported);
      }
    }
    else
    {
      WriteNewline(out);
      WriteNormal(out,
                  L"Import Thunks (" + std::wstring(use_ilt ? L"ILT" : L"IAT") +
                    L")",
                  2);
    }

    bool const is_bound = !!dir.GetTimeDateStamp();
    // Assume that any PE files mapped as images in memory have had their
    // imports resolved.
    bool const is_memory_bound =
      (pe_file.GetType() == hadesmem::PeFileType::kImage) && !use_ilt;
    bool const is_ilt_bound = (is_bound && !use_ilt) || is_memory_bound;
    bool const is_iat_bound =
      is_bound || (pe_file.GetType() == hadesmem::PeFileType::kImage);
    std::size_t count = 0U;
    for (auto const& thunk : ilt_thunks)
    {
      // Some legitimate PE files have well over 1000 imports from a single
      // module (e.g. idaq64.exe importing QtGui4.dll).
      if (count++ == 10000)
      {
        WriteNewline(out);
        WriteNormal(
          out,
          L"WARNING! Processed 10000 import thunks. Stopping early to "
          L"avoid resource exhaustion attacks.",
          2);
        WarnForCurrentFile(WarningType::kUnsupported);
        break;
      }

      (void)is_ilt_bound;
      // In the case where we have an already mapped image with no ILT, the
      // original name/ordinal inforamtion is long gone so all we have to work
      // from in the IAT (which is bound).
      bool const is_image_iat =
        (pe_file.GetType() == hadesmem::PeFileType::kImage && !use_ilt);
      DumpImportThunk(thunk, is_image_iat);
    }

    // Windows will load PE files that have an invalid RVA for the ILT (lies
    // outside of the virtual space), and will fall back to the IAT in this
    // case.
    if (use_ilt && iat)
    {
      hadesmem::ImportThunkList const iat_thunks(
        process, pe_file, dir.GetFirstThunk());
      if (std::begin(iat_thunks) != std::end(iat_thunks))
      {
        WriteNewline(out);
        WriteNormal(out, L"Import Thunks (IAT)", 2);
      }
      for (auto const& thunk : iat_thunks)
      {
        if (ilt_valid && !count--)
        {
          WriteNewline(out);
          WriteNormal(out,
                      L"WARNING! IAT size does not match ILT size. Stopping "
                      L"IAT enumeration early.",
                      2);
          // Apparently some legitimate (but strange) files do this. Probably in
          // order to save some space because you seemingly don't need the null
          // padding in practice as the Windows loader enumerates both
          // directories in parallel and will stop when it reaches the end of
          // the ILT, it doesn't actually care if the IAT is terminated or not.
          // Sample: pdfinfo.exe (from Git)
          // TODO: Investigate further and double-check the above assumption,
          // and also figure out whether there's some way we can narrow the
          // scope of the warning to let through legitimate files while still
          // warning on suspicious ones.
          WarnForCurrentFile(WarningType::kSuspicious);
          break;
        }

        // If the ILT is not empty (empty includes invalid) we simply treat the
        // IAT as bound, regardless of whether it actually is. This is because
        // apparently as long as you have a valid ILT you can put whatever the
        // hell you want in the IAT, because it's going to be overwitten anyway.
        // See tinynet.exe from the Corkami PE corpus for an example.
        // Furthermore, we only treat the IAT as bound if the ILT is also valid.
        // Not sure if this is correct, but apparently it's possible to have a
        // module with the TimeDateStamp set, indicating that the module is
        // bound, even though it actually isn't (and XP will apparently load
        // such a module). See tinygui.exe from the Corkami PE corpus for an
        // example.
        DumpImportThunk(thunk, (is_iat_bound && ilt_valid) || !ilt_empty);
      }
    }
  }
}
