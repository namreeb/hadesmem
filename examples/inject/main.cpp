// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include <memory>
#include <string>
#include <vector>
#include <iostream>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/program_options.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/injector.hpp>

#include "../common/initialize.hpp"

namespace
{

std::wstring PtrToString(void const* const ptr)
{
  std::wostringstream str;
  str.imbue(std::locale::classic());
  str << std::hex << reinterpret_cast<DWORD_PTR>(ptr);
  return str.str();
}

}

int main()
{
  try
  {
    DisableUserModeCallbackExceptionFilter();
    EnableCrtDebugFlags();
    EnableTerminationOnHeapCorruption();
    EnableBottomUpRand();
    ImbueAllDefault();

    std::cout << "HadesMem Injector\n";

    boost::program_options::options_description opts_desc(
      "General options");
    opts_desc.add_options()
      ("help", "produce help message")
      ("pid", boost::program_options::value<DWORD>(), "process id")
      ("path", boost::program_options::wvalue<std::wstring>(), "module path")
      ("path-resolution", "perform path resolution")
      ("export", boost::program_options::value<std::string>(), "export name")
      ;

    std::vector<std::wstring> const args = boost::program_options::
      split_winmain(GetCommandLine());
    boost::program_options::variables_map var_map;
    boost::program_options::store(boost::program_options::wcommand_line_parser(
      args).options(opts_desc).run(), var_map);
    boost::program_options::notify(var_map);

    if (var_map.count("help"))
    {
      std::cout << '\n' << opts_desc << '\n';
      return 1;
    }

    if (!var_map.count("pid"))
    {
      std::cerr << "Error! Process ID must be specified.\n";
      return 1;
    }

    if (!var_map.count("path"))
    {
      std::cerr << "Error! Module path must be specified.\n";
      return 1;
    }

    DWORD const pid = var_map["pid"].as<DWORD>();

    try
    {
      hadesmem::GetSeDebugPrivilege();

      std::wcout << "\nAcquired SeDebugPrivilege.\n";
    }
    catch (std::exception const& /*e*/)
    {
      std::wcout << "\nFailed to acquire SeDebugPrivilege.\n";
    }

    hadesmem::Process const process(pid);
    std::wstring const path = var_map["path"].as<std::wstring>();
    int const flags = var_map.count("path-resolution") ? 
      hadesmem::InjectFlags::kPathResolution : 
      hadesmem::InjectFlags::kNone;
    HMODULE const module = hadesmem::InjectDll(process, path, flags);

    std::wcout << "\nSuccessfully injected module at base address " << 
      PtrToString(module) << ".\n";

    if (var_map.count("export"))
    {
      std::string const export_name = var_map["export"].as<std::string>();
      std::pair<DWORD_PTR, DWORD> const export_ret = hadesmem::CallExport(
        process, module, export_name, module);

      std::wcout << "Successfully called module export.\n";
      std::wcout << "Return: " << export_ret.first << ".\n";
      std::wcout << "LastError: " << export_ret.second << ".\n";
    }
  }
  catch (std::exception const& e)
  {
    std::cerr << "Error!\n";
    std::cerr << boost::diagnostic_information(e) << "\n";

    return 1;
  }
}
