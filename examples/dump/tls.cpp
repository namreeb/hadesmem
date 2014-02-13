// Copyright (C) 2010-2014 Joshua Boyce.
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
#include "print.hpp"
#include "warning.hpp"

void DumpTls(hadesmem::Process const& process, hadesmem::PeFile const& pe_file)
{
  std::unique_ptr<hadesmem::TlsDir const> tls_dir;
  try
  {
    tls_dir = std::make_unique<hadesmem::TlsDir>(process, pe_file);
  }
  catch (std::exception const& /*e*/)
  {
    return;
  }

  std::wostream& out = std::wcout;

  WriteNewline(out);
  WriteNormal(out, L"TLS:", 1);

  WriteNewline(out);
  WriteNamedHex(
    out, L"StartAddressOfRawData", tls_dir->GetStartAddressOfRawData(), 2);
  WriteNamedHex(
    out, L"EndAddressOfRawData", tls_dir->GetEndAddressOfRawData(), 2);
  WriteNamedHex(out, L"AddressOfIndex", tls_dir->GetAddressOfIndex(), 2);
  WriteNamedHex(
    out, L"AddressOfCallBacks", tls_dir->GetAddressOfCallBacks(), 2);
  if (tls_dir->GetAddressOfCallBacks())
  {
    std::vector<PIMAGE_TLS_CALLBACK> callbacks;
    try
    {
      tls_dir->GetCallbacks(std::back_inserter(callbacks));
    }
    catch (std::exception const& /*e*/)
    {
      WriteNormal(out, L"WARNING! TLS callbacks are inavlid.", 2);
      WarnForCurrentFile(WarningType::kSuspicious);
    }
    for (auto const c : callbacks)
    {
      WriteNamedHex(out, L"Callback", reinterpret_cast<DWORD_PTR>(c), 2);
    }
  }
  WriteNamedHex(out, L"SizeOfZeroFill", tls_dir->GetSizeOfZeroFill(), 2);
  WriteNamedHex(out, L"Characteristics", tls_dir->GetCharacteristics(), 2);
}
