// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include <iostream>

#include "initialize.hpp"

int main()
{
  std::cout << "HadesMem MemHack.\n";

  std::cout << "Initializing.\n";

  DisableUserModeCallbackExceptionFilter();
  EnableCrtDebugFlags();
  EnableTerminationOnHeapCorruption();
  EnableBottomUpRand();
  ImbueAllDefault();

  std::cout << "MemHack initialized\n";
}
