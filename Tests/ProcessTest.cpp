/*
This file is part of HadesMem.
Copyright (C) 2011 Joshua Boyce (a.k.a. RaptorFactor).
<http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

HadesMem is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

HadesMem is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with HadesMem.  If not, see <http://www.gnu.org/licenses/>.
*/

// Hades
#include <HadesMemory/Process.hpp>

// Boost
#define BOOST_TEST_MODULE ProcessTest
#include <boost/test/unit_test.hpp>

// Process component tests
BOOST_AUTO_TEST_CASE(BOOST_TEST_MODULE)
{
  // Create process manager for self
  HadesMem::Process MyProcess(GetCurrentProcessId());
  HadesMem::Process OtherProcess(MyProcess);
  MyProcess = OtherProcess;
  BOOST_CHECK_THROW(HadesMem::Process InvalidProc(static_cast<DWORD>(-1)), 
    HadesMem::HadesMemError);
    
  // Check process APIs for predictable values where possible, otherwise just 
  // ensure they run without exception
  BOOST_CHECK(MyProcess.GetHandle() != 0);
  BOOST_CHECK(MyProcess.GetID() != 0);
  BOOST_CHECK(!MyProcess.GetPath().empty());
  
  // Test Process::IsWoW64
#if defined(_M_AMD64) 
  BOOST_CHECK_EQUAL(MyProcess.IsWoW64(), false);
#elif defined(_M_IX86) 
  BOOL Wow64Process = FALSE;
  BOOST_REQUIRE(IsWow64Process(MyProcess.GetHandle(), &Wow64Process));
  BOOST_CHECK_EQUAL(MyProcess.IsWoW64(), (Wow64Process ? true : false));
#else 
#error "[HadesMem] Unsupported architecture."
#endif
}
