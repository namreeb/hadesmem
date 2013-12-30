// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "imports.hpp"

#include <iostream>
#include <iterator>

#include <hadesmem/pelib/import_dir.hpp>
#include <hadesmem/pelib/import_dir_list.hpp>
#include <hadesmem/pelib/import_thunk.hpp>
#include <hadesmem/pelib/import_thunk_list.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>

#include "main.hpp"

// TODO: Detect imports which simply point back to exports from the same module
// (also detect if the exports are forwarded, and also detect infinite loops).
// Remember that all exports can have the same name, so we need to use the hint
// first, then only use the name if we fail to find a match using the hint. See
// "Import name table" and "Import name hint" in ReversingLabs "Undocumented
// PECOFF" whitepaper for more information.

// TODO: Detect and handle cases where an import descriptor has a virtual
// terminator. See imports_vterm.exe from Corkami or "Import directory layout"
// in ReversingLabs "Undocumented PECOFF" whitepaper for more information.

// TODO: Support bound imports (both old and new style).

// TODO: Support forwarded imports. (Are these even a thing? What is
// ForwarderString?)

namespace
{

void DumpImportThunk(hadesmem::ImportThunk const& thunk, bool is_bound)
{
  std::wcout << "\n";

  bool const by_ordinal = thunk.ByOrdinal();

  if (is_bound && by_ordinal)
  {
    std::wcout << "\t\t\tWARNING! Invalid import data (both bound and also "
                  "imported by ordinal).\n";
    WarnForCurrentFile(WarningType::kUnsupported);
  }

  if (is_bound)
  {
    std::wcout << "\t\t\tFunction: " << std::hex << thunk.GetFunction()
               << std::dec << "\n";
  }
  else if (by_ordinal)
  {
    std::wcout << "\t\t\tOrdinalRaw: " << std::hex << thunk.GetOrdinalRaw()
               << std::dec << "\n";
    std::wcout << "\t\t\tOrdinal: " << thunk.GetOrdinal() << "\n";
  }
  else
  {
    try
    {
      std::wcout << "\t\t\tAddressOfData: " << std::hex
                 << thunk.GetAddressOfData() << std::dec << "\n";
      std::wcout << "\t\t\tHint: " << thunk.GetHint() << "\n";
      std::wcout << "\t\t\tName: " << thunk.GetName().c_str() << "\n";
    }
    catch (std::exception const& /*e*/)
    {
      std::wcout << "\t\t\tWARNING! Invalid import thunk name data.\n";
      WarnForCurrentFile(WarningType::kUnsupported);
    }
  }
}
}

void DumpImports(hadesmem::Process const& process,
                 hadesmem::PeFile const& pe_file)
{
  hadesmem::ImportDirList import_dirs(process, pe_file);

  if (std::begin(import_dirs) != std::end(import_dirs))
  {
    std::wcout << "\n\tImport Dirs:\n";
  }

  std::uint32_t num_import_dirs = 0U;
  for (auto const& dir : import_dirs)
  {
    std::wcout << "\n";

    {
      // If the IAT is empty then the descriptor is skipped, and the name can
      // be invalid because it's ignored. Note that we simply skip here rather
      // than terminate, because it's possible to have such 'invalid' entries
      // in-between real entries.
      hadesmem::ImportThunkList iat_thunks(
        process, pe_file, dir.GetFirstThunk());
      if (std::begin(iat_thunks) == std::end(iat_thunks))
      {
        std::wcout << "\t\tWARNING! Detected an invalid import dir (empty "
                      "IAT). Skipping.\n";
        WarnForCurrentFile(WarningType::kSuspicious);
        continue;
      }
    }

    // TODO: Come up with a better solution to this.
    if (num_import_dirs++ == 1000)
    {
      std::wcout << "\t\tWARNING! Processed 1000 import dirs. Stopping early "
                    "to avoid resource exhaustion attacks. Check PE file for "
                    "TLS AOI trick, virtual terminator trick, or other similar "
                    "attacks.\n";
      WarnForCurrentFile(WarningType::kUnsupported);
      break;
    }

    std::wcout << "\t\tOriginalFirstThunk: " << std::hex
               << dir.GetOriginalFirstThunk() << std::dec << "\n";
    DWORD const time_date_stamp = dir.GetTimeDateStamp();
    std::wcout << "\t\tTimeDateStamp: " << std::hex << time_date_stamp
               << std::dec << "\n";
    if (time_date_stamp != 0 && time_date_stamp != static_cast<DWORD>(-1))
    {
      std::wcout << "\t\tWARNING! Detected old style bound imports. Currently "
                    "unhandled.\n";
      WarnForCurrentFile(WarningType::kUnsupported);
    }
    else if (time_date_stamp == static_cast<DWORD>(-1))
    {
      std::wcout << "\t\tWARNING! Detected new style bound imports. Currently "
                    "unhandled.\n";
      WarnForCurrentFile(WarningType::kUnsupported);
    }
    DWORD const forwarder_chain = dir.GetForwarderChain();
    std::wcout << "\t\tForwarderChain: " << std::hex << forwarder_chain
               << std::dec << "\n";
    if (forwarder_chain != 0 && forwarder_chain != static_cast<DWORD>(-1))
    {
      std::wcout << "\t\tWARNING! Detected old style forwarder chain. "
                    "Currently unhandled.\n";
      WarnForCurrentFile(WarningType::kUnsupported);
    }
    std::wcout << "\t\tName (Raw): " << std::hex << dir.GetNameRaw() << std::dec
               << "\n";
    try
    {
      // Import names don't need to consist of only printable characters, as
      // long as they are zero-terminated.
      // TODO: Find a solution to the above case, and perhaps use a vector<char>
      // instead of a string in the cases where the name isn't printable.
      std::wcout << "\t\tName: " << dir.GetName().c_str() << "\n";
    }
    catch (std::exception const& /*e*/)
    {
      std::wcout << "\t\tWARNING! Failed to read name.\n";
      WarnForCurrentFile(WarningType::kSuspicious);
    }
    std::wcout << "\t\tFirstThunk: " << std::hex << dir.GetFirstThunk()
               << std::dec << "\n";

    // TODO: Parse the IAT and ILT in parallel, in order to easily detect when
    // imports are bound in-memory. This will also mean we no longer need to
    // count the length of the ILT in order to terminate the IAT pass early.

    // TODO: According to "Import table" section of ReversingLabs "Undocumented
    // PECOFF" whitepaper, the IAT is never optional. Investigate this and
    // improve the code and/or add detection where appropriate.
    DWORD const ilt = dir.GetOriginalFirstThunk();
    DWORD const iat = dir.GetFirstThunk();
    bool const use_ilt = !!ilt && ilt != iat;
    hadesmem::ImportThunkList ilt_thunks(process, pe_file, use_ilt ? ilt : iat);
    bool const ilt_empty = std::begin(ilt_thunks) == std::end(ilt_thunks);
    bool const ilt_valid = !!hadesmem::RvaToVa(process, pe_file, ilt);
    if (ilt_empty)
    {
      // Has to be the ILT if we get here because we did a check for an empty/invalid IAT earlier on.
      std::wcout << "\n\t\tWARNING! ILT is " << (ilt_valid ? "empty" : "invalid") << ".\n";

      WarnForCurrentFile(WarningType::kSuspicious);
    }
    else
    {
      std::wcout << "\n\t\tImport Thunks (" << (use_ilt ? "ILT" : "IAT")
                 << "):\n";
    }

    // TODO: Distinguish between new and old binding styles and handle
    // appropriately.
    // TODO: Detect when the import dir says it is bound with the new style, but
    // the file does not have a valid bound import dir. In this case it seems to
    // be ignored by the loader. We should warn for this, but we probably can't
    // change the way we interpret the data, because just because there's no
    // bound import dir doesn't mean the IAT contains legitimate un-bound data,
    // it could just be complete garbage. Need to confirm this though...
    bool const is_bound = !!dir.GetTimeDateStamp();
    // TODO: Support cases where we have a PeFileType of Image, but the module
    // has been loaded with DONT_RESOLVE_DLL_REFERENCES (or equivalent).
    bool const is_memory_bound =
      (pe_file.GetType() == hadesmem::PeFileType::Image) && !use_ilt;
    bool const is_ilt_bound = (is_bound && !use_ilt) || is_memory_bound;
    bool const is_iat_bound =
      is_bound || (pe_file.GetType() == hadesmem::PeFileType::Image);
    std::size_t count = 0U;
    for (auto const& thunk : ilt_thunks)
    {
      // TODO: Come up with a better solution to this.
      if (count++ == 1000)
      {
        std::wcout << "\n\t\t\tWARNING! Processed 1000 import thunks. Stopping "
                      "early to avoid resource exhaustion attacks. Check PE "
                      "file for TLS AOI trick, virtual terminator trick, or "
                      "other similar attacks.\n";
        WarnForCurrentFile(WarningType::kUnsupported);
        break;
      }

      DumpImportThunk(thunk, is_ilt_bound);
    }

    // Windows will load PE files that have an invalid RVA for the ILT (lies
    // outside of the virtual space), and will fall back to the IAT in this
    // case.
    if (iat && (ilt || !ilt_valid) && iat != ilt)
    {
      hadesmem::ImportThunkList iat_thunks(
        process, pe_file, dir.GetFirstThunk());
      if (std::begin(iat_thunks) != std::end(iat_thunks))
      {
        std::wcout << "\n\t\tImport Thunks (IAT):\n";
      }
      for (auto const& thunk : iat_thunks)
      {
        if (ilt_valid && !count--)
        {
          std::wcout << "\n\t\t\tWARNING! IAT size does not match ILT size. "
                        "Stopping IAT enumeration early.\n";
          WarnForCurrentFile(WarningType::kSuspicious);
          break;
        }

        DumpImportThunk(thunk, is_iat_bound);
      }
    }
  }
}
