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
#include "hadesmem/process.hpp"

// Boost.Test causes the following warning under GCC:
// error: base class 'struct boost::unit_test::ut_detail::nil_t' has a 
// non-virtual destructor [-Werror=effc++]
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic ignored "-Weffc++"
#endif // #if defined(HADESMEM_GCC)

// TODO: Patcher constructor tests.
// TODO: Improve hook target checks.
// TODO: Fix annoying UAC popup (with manifest).
// TODO: Fix test under Clang and ICC. (Convert to use functions written 
// in assembly to ensure it's not caused by compiler-specific call conv 
// issues triggering incomplete parts of Patcher such as relative instruction 
// conversion.)

namespace
{
  
DWORD HookMe()
{
  std::string const foo("Foo");
  BOOST_CHECK_EQUAL(foo, "Foo");
  return 0x1234;
}

std::shared_ptr<hadesmem::PatchDetour> g_ptr_detour_1;

DWORD HookMe_Hook()
{
  BOOST_CHECK(g_ptr_detour_1->GetTrampoline() != nullptr);
  auto const ptr_orig = reinterpret_cast<DWORD (*)()>(
    reinterpret_cast<DWORD_PTR>(g_ptr_detour_1->GetTrampoline()));
  BOOST_CHECK_EQUAL(ptr_orig(), static_cast<DWORD>(0x1234));
  return 0x1337;
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
  auto const orig_1 = hadesmem::ReadVector<BYTE>(process, test_mem_1.GetBase(), 
    5);
  patch_1.Apply();
  auto const apply_1 = hadesmem::ReadVector<BYTE>(process, test_mem_1.GetBase(), 
    5);
  patch_1.Remove();
  auto const remove_1 = hadesmem::ReadVector<BYTE>(process, 
    test_mem_1.GetBase(), 5);
  BOOST_CHECK(orig_1 == remove_1);
  BOOST_CHECK(orig_1 != apply_1);
  BOOST_CHECK(data_1 == apply_1);

  BOOST_CHECK_EQUAL(HookMe(), static_cast<DWORD>(0x1234));
  DWORD_PTR const ptr_hook_me = reinterpret_cast<DWORD_PTR>(&HookMe);
  DWORD_PTR const ptr_hook_me_hook = reinterpret_cast<DWORD_PTR>(&HookMe_Hook);
  g_ptr_detour_1.reset(new hadesmem::PatchDetour(&process, 
    reinterpret_cast<PVOID>(ptr_hook_me), 
    reinterpret_cast<PVOID>(ptr_hook_me_hook)));
  BOOST_CHECK_EQUAL(HookMe(), static_cast<DWORD>(0x1234));
  g_ptr_detour_1->Apply();
  BOOST_CHECK_EQUAL(HookMe(), static_cast<DWORD>(0x1337));
  g_ptr_detour_1->Remove();
  BOOST_CHECK_EQUAL(HookMe(), static_cast<DWORD>(0x1234));
  g_ptr_detour_1->Apply();
  BOOST_CHECK_EQUAL(HookMe(), static_cast<DWORD>(0x1337));
  g_ptr_detour_1->Remove();
  BOOST_CHECK_EQUAL(HookMe(), static_cast<DWORD>(0x1234));
}
