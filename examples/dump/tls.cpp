// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include "tls.hpp"

#include <iostream>
#include <iterator>
#include <memory>
#include <vector>

#include <hadesmem/pelib/tls_dir.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/process.hpp>

#include "main.hpp"

void DumpTls(hadesmem::Process const& process, hadesmem::PeFile const& pe_file)
{
  std::unique_ptr<hadesmem::TlsDir> tls_dir;
  try
  {
    tls_dir = std::make_unique<hadesmem::TlsDir>(process, pe_file);
  }
  catch (std::exception const& /*e*/)
  {
    return;
  }

  std::wcout << "\n\tTLS:\n";

  std::wcout << "\n";
  std::wcout << "\t\tStartAddressOfRawData: " << std::hex
             << tls_dir->GetStartAddressOfRawData() << std::dec << "\n";
  std::wcout << "\t\tEndAddressOfRawData: " << std::hex
             << tls_dir->GetEndAddressOfRawData() << std::dec << "\n";
  std::wcout << "\t\tAddressOfIndex: " << std::hex
             << tls_dir->GetAddressOfIndex() << std::dec << "\n";
  std::wcout << "\t\tAddressOfCallBacks: " << std::hex
             << tls_dir->GetAddressOfCallBacks() << std::dec << "\n";
  if (tls_dir->GetAddressOfCallBacks())
  {
    std::vector<PIMAGE_TLS_CALLBACK> callbacks;
    try
    {
      tls_dir->GetCallbacks(std::back_inserter(callbacks));
    }
    catch (std::exception const& /*e*/)
    {
      std::wcout << "\t\tWARNING! TLS callbacks are inavlid.\n";
      WarnForCurrentFile();
    }
    for (auto const c : callbacks)
    {
      std::wcout << "\t\tCallback: " << std::hex
                 << reinterpret_cast<DWORD_PTR>(c) << std::dec << "\n";
    }
  }
  std::wcout << "\t\tSizeOfZeroFill: " << std::hex
             << tls_dir->GetSizeOfZeroFill() << std::dec << "\n";
  std::wcout << "\t\tCharacteristics: " << std::hex
             << tls_dir->GetCharacteristics() << std::dec << "\n";
}
