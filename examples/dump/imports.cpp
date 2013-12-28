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
      std::wcout << "\t\tName: " << dir.GetName().c_str() << "\n";
    }
    catch (std::exception const& /*e*/)
    {
      std::wcout << "\t\tWARNING! Failed to read name.\n";
      WarnForCurrentFile();
    }
    std::wcout << "\t\tFirstThunk: " << std::hex << dir.GetFirstThunk()
               << std::dec << "\n";

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
