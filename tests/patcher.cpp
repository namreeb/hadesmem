// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/patcher.hpp"

#define BOOST_TEST_MODULE patcher
#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/test/unit_test.hpp>
#include <boost/assign/std/vector.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"
using namespace boost::assign;

#include "hadesmem/alloc.hpp"
#include "hadesmem/error.hpp"
#include "hadesmem/read.hpp"
#include "hadesmem/module.hpp"
#include "hadesmem/process.hpp"

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

// TODO: Patcher constructor tests.
// TODO: Improve hook target checks.
// TODO: Fix annoying UAC popup (with manifest).
// TODO: Test Patcher on functions using 'manually' generated code to 
// ensure all cases are covered, and that the tests are consistent (which 
// the Windows API testing is not...).

namespace
{

#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif // #if defined(HADESMEM_CLANG)  

std::shared_ptr<hadesmem::PatchDetour> g_ptr_detour_1;

#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_CLANG)  

DWORD GetCurrentProcessId_Hook()
{
  BOOST_CHECK(g_ptr_detour_1->GetTrampoline() != nullptr);
  auto const ptr_orig = reinterpret_cast<DWORD (WINAPI*)()>(
    reinterpret_cast<DWORD_PTR>(g_ptr_detour_1->GetTrampoline()));
  return ptr_orig() * 2;
}

}

BOOST_AUTO_TEST_CASE(patcher)
{
  hadesmem::Process const process(::GetCurrentProcessId());

  hadesmem::Allocator const test_mem_1(&process, 0x1000);
  std::vector<BYTE> data_1;
  data_1 += 0x00, 0x11, 0x22, 0x33, 0x44;
  BOOST_REQUIRE(data_1.size() == 5);
  hadesmem::PatchRaw patch_1(&process, test_mem_1.GetBase(), data_1);
  auto const orig_1 = hadesmem::ReadVector<BYTE>(process, 
    test_mem_1.GetBase(), 5);
  patch_1.Apply();
  auto const apply_1 = hadesmem::ReadVector<BYTE>(process, 
    test_mem_1.GetBase(), 5);
  patch_1.Remove();
  auto const remove_1 = hadesmem::ReadVector<BYTE>(process, 
    test_mem_1.GetBase(), 5);
  BOOST_CHECK(orig_1 == remove_1);
  BOOST_CHECK(orig_1 != apply_1);
  BOOST_CHECK(data_1 == apply_1);

  DWORD const proc_id = GetCurrentProcessId();
  hadesmem::Module const kernel32_mod(&process, L"kernel32.dll");
  auto const get_current_process_id = 
    reinterpret_cast<DWORD (WINAPI *)()>(reinterpret_cast<DWORD_PTR>(
    hadesmem::FindProcedure(kernel32_mod, "GetCurrentProcessId")));
  BOOST_CHECK_EQUAL(get_current_process_id(), GetCurrentProcessId());
  g_ptr_detour_1.reset(new hadesmem::PatchDetour(&process, 
    reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    get_current_process_id)), 
    reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    &GetCurrentProcessId_Hook))));
  BOOST_CHECK_EQUAL(GetCurrentProcessId(), proc_id);
  g_ptr_detour_1->Apply();
  BOOST_CHECK_EQUAL(GetCurrentProcessId(), proc_id * 2);
  g_ptr_detour_1->Remove();
  BOOST_CHECK_EQUAL(GetCurrentProcessId(), proc_id);
  g_ptr_detour_1->Apply();
  BOOST_CHECK_EQUAL(GetCurrentProcessId(), proc_id * 2);
  g_ptr_detour_1->Remove();
  BOOST_CHECK_EQUAL(GetCurrentProcessId(), proc_id);
}
