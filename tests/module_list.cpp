// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/module_list.hpp>

#include <utility>

#define BOOST_TEST_MODULE module_list
#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/concept_check.hpp>
#include <boost/test/unit_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/module.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/process.hpp>

// Boost.Test causes the following warning under GCC:
// error: base class 'struct boost::unit_test::ut_detail::nil_t' has a 
// non-virtual destructor [-Werror=effc++]
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic ignored "-Weffc++"
#endif // #if defined(HADESMEM_GCC)

// Boost.Test causes the following warning under Clang:
// error: declaration requires a global constructor 
// [-Werror,-Wglobal-constructors]
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#endif // #if defined(HADESMEM_CLANG)

BOOST_TEST_DONT_PRINT_LOG_VALUE(hadesmem::ModuleList::iterator)
BOOST_TEST_DONT_PRINT_LOG_VALUE(hadesmem::ModuleList::const_iterator)

BOOST_AUTO_TEST_CASE(module_list)
{
  BOOST_CONCEPT_ASSERT((boost::InputIterator<hadesmem::ModuleList::iterator>));
  BOOST_CONCEPT_ASSERT((boost::InputIterator<hadesmem::ModuleList::
    const_iterator>));
  
  hadesmem::Process const process(::GetCurrentProcessId());
  
  hadesmem::ModuleList const module_list_1(process);
  hadesmem::ModuleList module_list_2(module_list_1);
  hadesmem::ModuleList module_list_3(std::move(module_list_2));
  module_list_2 = std::move(module_list_3);
  BOOST_CHECK_NE(std::begin(module_list_2), std::end(module_list_2));
  
  auto iter = std::begin(module_list_1);
  hadesmem::Module const this_mod(process, nullptr);
  BOOST_CHECK_NE(iter, std::end(module_list_1));
  BOOST_CHECK_EQUAL(*iter, this_mod);
  hadesmem::Module const ntdll_mod(process, L"NtDll.DlL");
  BOOST_CHECK_NE(++iter, std::end(module_list_1));
  BOOST_CHECK_EQUAL(*iter, ntdll_mod);
}

BOOST_AUTO_TEST_CASE(module_list_algorithm)
{
  hadesmem::Process const process(::GetCurrentProcessId());
  
  hadesmem::ModuleList const module_list_1(process);
  
  for (auto const& module : module_list_1)
  {
    BOOST_CHECK_NE(module.GetHandle(), static_cast<void*>(nullptr));
    BOOST_CHECK_NE(module.GetSize(), 0U);
    BOOST_CHECK(!module.GetName().empty());
    BOOST_CHECK(!module.GetPath().empty());
  }
  
  auto const user32_iter = std::find_if(std::begin(module_list_1), 
    std::end(module_list_1), 
    [] (hadesmem::Module const& module)
    {
      return module.GetHandle() == GetModuleHandle(L"user32.dll");
    });
  BOOST_CHECK_NE(user32_iter, std::end(module_list_1));
}
