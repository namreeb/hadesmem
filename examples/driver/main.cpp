// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <windows.h>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <tclap/CmdLine.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/driver.hpp>

int main(int argc, char* argv[])
{
  try
  {
    std::cout << "HadesMem Driver Loader [" << HADESMEM_VERSION_STRING << "]\n";

    TCLAP::CmdLine cmd{"Driver loader", ' ', HADESMEM_VERSION_STRING};
    TCLAP::ValueArg<std::string> name_arg{
      "", "name", "Driver name", true, "", "string", cmd};
    TCLAP::ValueArg<std::string> path_arg{
      "", "path", "Driver path", false, "", "string", cmd};
    TCLAP::SwitchArg load_arg{"", "load", "Load driver"};
    TCLAP::SwitchArg unload_arg{"", "unload", "Unload driver"};
    std::vector<TCLAP::Arg*> xor_args{&load_arg, &unload_arg};
    cmd.xorAdd(xor_args);
    cmd.parse(argc, argv);

    if (load_arg.isSet() && !path_arg.isSet())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error() << hadesmem::ErrorString("Path to driver required."));
    }

    hadesmem::GetSeLoadDriverPrivilege();

    auto const driver_name =
      hadesmem::detail::MultiByteToWideChar(name_arg.getValue());
    auto const driver_path =
      hadesmem::detail::MultiByteToWideChar(path_arg.getValue());

    if (load_arg.isSet())
    {
      hadesmem::LoadDriver(driver_name, driver_path);
    }
    else
    {
      hadesmem::UnloadDriver(driver_name);
    }

    std::cout << "\nDone.\n";

    return 0;
  }
  catch (...)
  {
    std::cerr << "\nError!\n";
    std::cerr << boost::current_exception_diagnostic_information() << '\n';

    return 1;
  }
}
