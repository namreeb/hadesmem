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

namespace
{

void DumpImportThunk(hadesmem::ImportThunk const& thunk)
{
  std::wcout << "\n";
  std::wcout << "\t\t\tAddressOfData: " << std::hex << thunk.GetAddressOfData()
             << std::dec << "\n";
  std::wcout << "\t\t\tOrdinalRaw: " << thunk.GetOrdinalRaw() << "\n";
  try
  {
    if (thunk.ByOrdinal())
    {
      std::wcout << "\t\t\tOrdinal: " << thunk.GetOrdinal() << "\n";
    }
    else
    {
      std::wcout << "\t\t\tHint: " << thunk.GetHint() << "\n";
      std::wcout << "\t\t\tName: " << thunk.GetName().c_str() << "\n";
    }
  }
  catch (std::exception const& /*e*/)
  {
    std::wcout << "\t\t\tWARNING! Invalid ordinal or name.\n";
    WarnForCurrentFile();
  }
  std::wcout << "\t\t\tFunction: " << std::hex << thunk.GetFunction()
             << std::dec << "\n";
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

    // TODO: Come up with a better solution to this.
    if (num_import_dirs++ == 1000)
    {
      std::wcout << "\t\tWARNING! Processed 1000 import dirs. Stopping early "
                    "to avoid resource exhaustion attacks. Check PE file for "
                    "TLS AOI trick, virtual terminator trick, or other similar "
                    "attacks.\n";
      WarnForCurrentFile();
      break;
    }

    std::wcout << "\t\tOriginalFirstThunk: " << std::hex
               << dir.GetOriginalFirstThunk() << std::dec << "\n";
    std::wcout << "\t\tTimeDateStamp: " << std::hex << dir.GetTimeDateStamp()
               << std::dec << "\n";
    std::wcout << "\t\tForwarderChain: " << std::hex << dir.GetForwarderChain()
               << std::dec << "\n";
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
      WarnForCurrentFile();
    }
    std::wcout << "\t\tFirstThunk: " << std::hex << dir.GetFirstThunk()
               << std::dec << "\n";

    // TODO: According to "Import table" section of ReversingLabs "Undocumented
    // PECOFF" whitepaper, the IAT is never optional. Investigate this and
    // improve the code and/or add detection where appropriate.

    // Certain information gets destroyed by the Windows PE loader in
    // some circumstances. Nothing we can do but ignore it or resort
    // to reading the original data from disk.
    if (pe_file.GetType() == hadesmem::PeFileType::Image)
    {
      // Images without an INT/ILT are valid, but after an image
      // like this is loaded it is impossible to recover the name
      // table.
      if (!dir.GetOriginalFirstThunk())
      {
        std::wcout << "\n\t\tWARNING! No INT for this module.\n";
        WarnForCurrentFile();
        continue;
      }

      // Some modules (packed modules are the only ones I've found
      // so far) have import directories where the IAT RVA is the
      // same as the INT/ILT RVA which effectively means there is
      // no INT/ILT once the module is loaded.
      if (dir.GetOriginalFirstThunk() == dir.GetFirstThunk())
      {
        std::wcout << "\n\t\tWARNING! IAT is same as INT for this module.\n";
        WarnForCurrentFile();
        continue;
      }
    }

    hadesmem::ImportThunkList ilt_thunks(process,
                                         pe_file,
                                         dir.GetOriginalFirstThunk()
                                           ? dir.GetOriginalFirstThunk()
                                           : dir.GetFirstThunk());
    if (std::begin(ilt_thunks) == std::end(ilt_thunks))
    {
      if (dir.GetOriginalFirstThunk())
      {
        std::wcout << "\n\t\tWARNING! ILT is empty or invalid.\n";
        WarnForCurrentFile();
      }
      else
      {
        std::wcout << "\n\t\tWARNING! IAT is empty or invalid.\n";
        WarnForCurrentFile();
      }
    }
    else
    {
      if (dir.GetOriginalFirstThunk())
      {
        std::wcout << "\n\t\tImport Thunks (ILT):\n";
      }
      else
      {
        std::wcout << "\n\t\tImport Thunks (IAT):\n";
      }
    }

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
        WarnForCurrentFile();
        break;
      }

      DumpImportThunk(thunk);
    }

    // Windows will load PE files that have an invalid RVA for the ILT (lies
    // outside of the virtual space), and will fall back to the IAT in this
    // case.
    bool const ilt_valid =
      !!hadesmem::RvaToVa(process, pe_file, dir.GetOriginalFirstThunk());
    if (dir.GetFirstThunk() && (dir.GetOriginalFirstThunk() || !ilt_valid))
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
          WarnForCurrentFile();
          break;
        }

        DumpImportThunk(thunk);
      }
    }
  }
}
