// Copyright Joshua Boyce 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

// Hades
#include <HadesMemory/MemoryMgr.hpp>
#include <HadesMemory/Experimental/ManualMap.hpp>

// Boost
#define BOOST_TEST_MODULE ManualMapTest
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(ConstructorsTest)
{
  // Create memory manager for self
  HadesMem::MemoryMgr MyMemory(GetCurrentProcessId());
    
  // Create manual mapper
  HadesMem::ManualMap MyManualMap(MyMemory);
      
  // Test copying, assignement, and moving
  HadesMem::ManualMap MyOtherManualMap(MyManualMap);
  BOOST_CHECK(MyManualMap == MyOtherManualMap);
  MyManualMap = MyOtherManualMap;
  BOOST_CHECK(MyManualMap == MyOtherManualMap);
  HadesMem::ManualMap MovedManualMap(std::move(MyOtherManualMap));
  BOOST_CHECK(MovedManualMap == MyManualMap);
  HadesMem::ManualMap NewTestManualMap(MyManualMap);
  MyManualMap = std::move(NewTestManualMap);
  BOOST_CHECK(MyManualMap == MovedManualMap);
}

BOOST_AUTO_TEST_CASE(MappingTest)
{
  // Create memory manager for self
  HadesMem::MemoryMgr const MyMemory(GetCurrentProcessId());
  
  // Create manual mapper
  HadesMem::ManualMap const MyManualMap(MyMemory);
  
  // Manually map mscoree.dll
  MyManualMap.InjectDll(L"mscoree.dll");
  
  // Todo: Test path resolution
  // Todo: Test export calling
}
