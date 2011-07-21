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
#include <HadesMemory/Module.hpp>
#include <HadesMemory/MemoryMgr.hpp>

// C++ Standard Library
#include <algorithm>

// Boost
#define BOOST_TEST_MODULE ModuleTest
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(ConstructorsTest)
{
  // Create memory manager for self
  HadesMem::MemoryMgr MyMemory(GetCurrentProcessId());
      
  // Open module by handle
  HadesMem::Module SelfModule(MyMemory, NULL);
  BOOST_CHECK_EQUAL(SelfModule.GetHandle(), GetModuleHandle(NULL));
  HadesMem::Module NewSelfModule(MyMemory, SelfModule.GetHandle());
  BOOST_CHECK(SelfModule == NewSelfModule);
      
  // Open module by name
  HadesMem::Module K32Module(MyMemory, L"kernel32.dll");
  BOOST_CHECK_EQUAL(K32Module.GetHandle(), GetModuleHandle(L"kernel32.dll"));
  HadesMem::Module NewK32Module(MyMemory, K32Module.GetName());
  BOOST_CHECK(K32Module == NewK32Module);
  HadesMem::Module NewNewK32Module(MyMemory, K32Module.GetPath());
  BOOST_CHECK(K32Module == NewNewK32Module);
  
  // Test inequality
  BOOST_CHECK(SelfModule != K32Module);
  
  // Test module handle failure
  BOOST_CHECK_THROW(HadesMem::Module InvalidModuleHandle(MyMemory, 
    reinterpret_cast<HMODULE>(-1)), HadesMem::HadesMemError);
  
  // Test module name failure
  BOOST_CHECK_THROW(HadesMem::Module InvalidModuleName(MyMemory, 
    L"InvalidModuleXYZQQ.dll"), HadesMem::HadesMemError);
      
  // Test copying, assignement, and moving
  HadesMem::Module OtherSelfModule(SelfModule);
  BOOST_CHECK(OtherSelfModule == SelfModule);
  SelfModule = OtherSelfModule;
  BOOST_CHECK(OtherSelfModule == SelfModule);
  HadesMem::Module MovedSelfModule(std::move(OtherSelfModule));
  BOOST_CHECK(MovedSelfModule == SelfModule);
  SelfModule = std::move(MovedSelfModule);
  BOOST_CHECK_EQUAL(SelfModule.GetHandle(), GetModuleHandle(NULL));
}

BOOST_AUTO_TEST_CASE(DataTest)
{
  // Create memory manager for self
  HadesMem::MemoryMgr MyMemory(GetCurrentProcessId());
      
  // Open module by name
  HadesMem::Module K32Module(MyMemory, L"kernel32.dll");
  
  // Test GetHandle
  BOOST_CHECK_EQUAL(K32Module.GetHandle(), GetModuleHandle(L"kernel32.dll"));
  
  // Test GetSize
  BOOST_CHECK(K32Module.GetSize() != 0);
  
  // Test GetName
  BOOST_CHECK(K32Module.GetName() == L"kernel32.dll");
  
  // Test GetPath
  BOOST_CHECK(!K32Module.GetPath().empty());
  BOOST_CHECK(K32Module.GetPath().find(L"kernel32.dll") != std::wstring::npos);
}

BOOST_AUTO_TEST_CASE(ProcedureTest)
{
  // Create memory manager for self
  HadesMem::MemoryMgr MyMemory(GetCurrentProcessId());
      
  // Open NTDLL module by name
  HadesMem::Module NtdllMod(MyMemory, L"ntdll.dll");
  
  // Find NtQueryInformationProcess
  FARPROC pQueryInfo = NtdllMod.FindProcedure(
    "NtQueryInformationProcess");
  BOOST_CHECK_EQUAL(pQueryInfo, GetProcAddress(GetModuleHandle(
    L"ntdll.dll"), "NtQueryInformationProcess"));
    
  // Find ordinal 0
  FARPROC pOrdinal0 = NtdllMod.FindProcedure(1);
  BOOST_CHECK_EQUAL(pOrdinal0, GetProcAddress(GetModuleHandle(
    L"ntdll.dll"), MAKEINTRESOURCEA(1)));
}

BOOST_AUTO_TEST_CASE(IteratorTest)
{
  // Create memory manager for self
  HadesMem::MemoryMgr MyMemory(GetCurrentProcessId());
  
  // Test non-const module iterator
  HadesMem::ModuleList Modules(MyMemory);
  BOOST_CHECK(Modules.begin() != Modules.end());
  std::for_each(Modules.begin(), Modules.end(), 
    [&] (HadesMem::Module& M)
    {
      // Ensure module APIs execute without exception and return valid data
      BOOST_CHECK(M.GetHandle() != 0);
      BOOST_CHECK(M.GetSize() != 0);
      BOOST_CHECK(!M.GetName().empty());
      BOOST_CHECK(!M.GetPath().empty());
      
      // Ensure GetRemoteModuleHandle works as expected
      // Note: The module name check could possibly fail if multiple modules 
      // with the same name but a different path are loaded in the process, 
      // but this is currently not the case with any of the testing binaries.
      BOOST_CHECK(M == HadesMem::GetRemoteModule(MyMemory, M.GetName().c_str()));
      BOOST_CHECK(M == HadesMem::GetRemoteModule(MyMemory, M.GetPath().c_str()));
    });
  
  // Test implicit const module iteratator
  HadesMem::ModuleList const ModulesImpC(MyMemory);
  BOOST_CHECK(ModulesImpC.begin() != ModulesImpC.end());
  std::for_each(ModulesImpC.begin(), ModulesImpC.end(), 
    [&] (HadesMem::Module const& M)
    {
      // Ensure module APIs execute without exception and return valid data
      BOOST_CHECK(M.GetHandle() != 0);
      BOOST_CHECK(M.GetSize() != 0);
      BOOST_CHECK(!M.GetName().empty());
      BOOST_CHECK(!M.GetPath().empty());
      
      // Ensure GetRemoteModuleHandle works as expected
      // Note: The module name check could possibly fail if multiple modules 
      // with the same name but a different path are loaded in the process, 
      // but this is currently not the case with any of the testing binaries.
      BOOST_CHECK(M == HadesMem::GetRemoteModule(MyMemory, M.GetName().c_str()));
      BOOST_CHECK(M == HadesMem::GetRemoteModule(MyMemory, M.GetPath().c_str()));
    });
  
  // Test explicit const module iteratator
  HadesMem::ModuleList ModulesExpC(MyMemory);
  BOOST_CHECK(ModulesExpC.cbegin() != ModulesExpC.cend());
  std::for_each(ModulesExpC.cbegin(), ModulesExpC.cend(), 
    [&] (HadesMem::Module const& M)
    {
      // Ensure module APIs execute without exception and return valid data
      BOOST_CHECK(M.GetHandle() != 0);
      BOOST_CHECK(M.GetSize() != 0);
      BOOST_CHECK(!M.GetName().empty());
      BOOST_CHECK(!M.GetPath().empty());
      
      // Ensure GetRemoteModuleHandle works as expected
      // Note: The module name check could possibly fail if multiple modules 
      // with the same name but a different path are loaded in the process, 
      // but this is currently not the case with any of the testing binaries.
      BOOST_CHECK(M == HadesMem::GetRemoteModule(MyMemory, M.GetName().c_str()));
      BOOST_CHECK(M == HadesMem::GetRemoteModule(MyMemory, M.GetPath().c_str()));
    });
}
