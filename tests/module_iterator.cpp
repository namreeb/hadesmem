// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/module_iterator.hpp"

#define BOOST_TEST_MODULE module_iterator
#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/module.hpp"
#include "hadesmem/process.hpp"

// Boost.Test causes the following warning under GCC:
// error: base class 'struct boost::unit_test::ut_detail::nil_t' has a 
// non-virtual destructor [-Werror=effc++]
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic ignored "-Weffc++"
#endif // #if defined(HADESMEM_GCC)

BOOST_AUTO_TEST_CASE(module_iterator)
{
  hadesmem::Process const process(::GetCurrentProcessId());
  
  auto iter = hadesmem::ModuleIterator(process);
  hadesmem::Module const this_mod(process, nullptr);
  BOOST_CHECK(iter != hadesmem::ModuleIterator());
  BOOST_CHECK(*iter == this_mod);
  hadesmem::Module const ntdll_mod(process, L"NtDll.DlL");
  BOOST_CHECK(++iter != hadesmem::ModuleIterator());
  BOOST_CHECK(*iter == ntdll_mod);
  hadesmem::Module const kernel32_mod(process, L"kernel32.dll");
  BOOST_CHECK(++iter != hadesmem::ModuleIterator());
  BOOST_CHECK(*iter == kernel32_mod);
  
  std::for_each(hadesmem::ModuleIterator(process), hadesmem::ModuleIterator(), 
    [] (hadesmem::Module const& module)
    {
      BOOST_CHECK(module.GetHandle() != nullptr);
    });
  
  BOOST_CHECK(std::find_if(hadesmem::ModuleIterator(process), 
    hadesmem::ModuleIterator(), 
    [] (hadesmem::Module const& module)
    {
      return module.GetHandle() == GetModuleHandle(L"user32.dll");
    }) != hadesmem::ModuleIterator());
}
