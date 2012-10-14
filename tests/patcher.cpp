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
#include "hadesmem/detail/union_cast.hpp"

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
std::shared_ptr<hadesmem::PatchDetour> g_ptr_detour_2;

#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_CLANG)  

DWORD WINAPI GetProcessId_Hook(HANDLE process)
{
  BOOST_CHECK_EQUAL(process, GetCurrentProcess());
  BOOST_CHECK(g_ptr_detour_1->GetTrampoline() != nullptr);
  auto const ptr_orig = reinterpret_cast<DWORD (WINAPI*)(HANDLE)>(
    reinterpret_cast<DWORD_PTR>(g_ptr_detour_1->GetTrampoline()));
  return ptr_orig(process) * 2;
}

// See below for comments on why this is currently MSVC-only.
#if defined(HADESMEM_MSVC)
class HookThis
{
public:
  HookThis(int i)
    : i_(i)
  { }

  bool HookMe(int i)
  {
    BOOST_CHECK_EQUAL(i_, 42);
    return i == i_;
  }

private:
  int i_;
};

class HookThisHook
{
public:
  bool HookMe_Hook(int i)
  {
    // Intentionally thrash ECX on x86
    std::vector<int> myvec;
    myvec.push_back(1234);

    BOOST_CHECK_EQUAL(i, 5);
    BOOST_CHECK(g_ptr_detour_2->GetTrampoline() != nullptr);
    hadesmem::detail::UnionCast<LPCVOID, bool (HookThisHook::*)(int)> mfn(
      g_ptr_detour_2->GetTrampoline());
    auto const ptr_orig = mfn.GetTo();
    return (this->*ptr_orig)(42);
  }
};
#endif // #if defined(HADESMEM_MSVC)

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

  DWORD const proc_id = GetProcessId(GetCurrentProcess());
  hadesmem::Module const kernel32_mod(&process, L"kernel32.dll");
  auto const get_process_id = 
    reinterpret_cast<DWORD (WINAPI *)(HANDLE)>(reinterpret_cast<DWORD_PTR>(
    hadesmem::FindProcedure(kernel32_mod, "GetProcessId")));
  BOOST_CHECK_EQUAL(get_process_id(GetCurrentProcess()), GetProcessId(
    GetCurrentProcess()));
  g_ptr_detour_1.reset(new hadesmem::PatchDetour(&process, 
    reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    get_process_id)), 
    reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    &GetProcessId_Hook)), 
    hadesmem::DetourFlags::kNone));
  BOOST_CHECK_EQUAL(GetProcessId(GetCurrentProcess()), proc_id);
  g_ptr_detour_1->Apply();
  BOOST_CHECK_EQUAL(GetProcessId(GetCurrentProcess()), proc_id * 2);
  g_ptr_detour_1->Remove();
  BOOST_CHECK_EQUAL(GetProcessId(GetCurrentProcess()), proc_id);
  g_ptr_detour_1->Apply();
  BOOST_CHECK_EQUAL(GetProcessId(GetCurrentProcess()), proc_id * 2);
  g_ptr_detour_1->Remove();
  BOOST_CHECK_EQUAL(GetProcessId(GetCurrentProcess()), proc_id);

  // MSVC is the only compiler which lets me get away with such crazy amounts 
  // of hackery. (The other compilers with compile the code, but the tests will 
  // crash and burn due to the perfectly valid assumption by the compiler 
  // that the code isn't self-modifying.)
  // TODO: Rewrite this using a second module or JIT assembly to avoid relying 
  // on behavior of the optimizer which could change at any time. Also, 
  // it is important that the other compilers receive full test coverage.
#if defined(HADESMEM_MSVC)
  auto hook_me_pfn = &HookThis::HookMe;
  hadesmem::detail::UnionCast<bool (HookThis::*)(int), PVOID> hook_me(
    hook_me_pfn);
  auto hook_me_hook_pfn = &HookThisHook::HookMe_Hook;
  hadesmem::detail::UnionCast<bool (HookThisHook::*)(int), PVOID> hook_me_hook(
    hook_me_hook_pfn);
  g_ptr_detour_2.reset(new hadesmem::PatchDetour(&process, 
    hook_me.GetTo(), 
    hook_me_hook.GetTo(), 
    hadesmem::DetourFlags::kNone));
  HookThis hook_this(42);
  BOOST_CHECK_EQUAL((hook_this.*hook_me_pfn)(5), false);
  BOOST_CHECK_EQUAL((hook_this.*hook_me_pfn)(42), true);
  g_ptr_detour_2->Apply();
  BOOST_CHECK_EQUAL((hook_this.*hook_me_pfn)(5), true);
  g_ptr_detour_2->Remove();
  BOOST_CHECK_EQUAL((hook_this.*hook_me_pfn)(5), false);
  g_ptr_detour_2->Apply();
  BOOST_CHECK_EQUAL((hook_this.*hook_me_pfn)(5), true);
  g_ptr_detour_2->Remove();
  BOOST_CHECK_EQUAL((hook_this.*hook_me_pfn)(5), false);
#endif // #if defined(HADESMEM_MSVC)
}
