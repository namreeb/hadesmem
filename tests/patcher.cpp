// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/patcher.hpp>
#include <hadesmem/patcher.hpp>

#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <asmjit/asmjit.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/union_cast.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>

namespace
{

std::unique_ptr<hadesmem::PatchDetour>& GetDetour1()
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour>& GetDetour2()
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

hadesmem::Process& GetThisProcess()
{
  static hadesmem::Process process(::GetCurrentProcessId());
  return process;
}
}

std::uint32_t __cdecl HookMe(std::int32_t i1,
                             std::int32_t i2,
                             std::int32_t i3,
                             std::int32_t i4,
                             std::int32_t i5,
                             std::int32_t i6,
                             std::int32_t i7,
                             std::int32_t i8)
{
  std::string const foo("Foo");
  BOOST_TEST_EQ(foo, "Foo");

  BOOST_TEST_EQ(i1, 1);
  BOOST_TEST_EQ(i2, 2);
  BOOST_TEST_EQ(i3, 3);
  BOOST_TEST_EQ(i4, 4);
  BOOST_TEST_EQ(i5, 5);
  BOOST_TEST_EQ(i6, 6);
  BOOST_TEST_EQ(i7, 7);
  BOOST_TEST_EQ(i8, 8);

  return 0x1234;
}

extern "C" std::uint32_t __cdecl HookMeHk(std::int32_t i1,
                                          std::int32_t i2,
                                          std::int32_t i3,
                                          std::int32_t i4,
                                          std::int32_t i5,
                                          std::int32_t i6,
                                          std::int32_t i7,
                                          std::int32_t i8)
{
  auto& detour_1 = GetDetour1();
  BOOST_TEST(detour_1->GetTrampoline() != nullptr);
  auto const orig = detour_1->GetTrampoline<decltype(&HookMe)>();
  BOOST_TEST_EQ(orig(i1, i2, i3, i4, i5, i6, i7, i8), 0x1234UL);
  return 0x1337;
}

extern "C" std::uint32_t __cdecl HookMeHk2(std::int32_t i1,
                                           std::int32_t i2,
                                           std::int32_t i3,
                                           std::int32_t i4,
                                           std::int32_t i5,
                                           std::int32_t i6,
                                           std::int32_t i7,
                                           std::int32_t i8)
{
  auto& detour_2 = GetDetour2();
  BOOST_TEST(detour_2->GetTrampoline() != nullptr);
  auto const orig = detour_2->GetTrampoline<decltype(&HookMe)>();
  BOOST_TEST_EQ(orig(i1, i2, i3, i4, i5, i6, i7, i8), 0x1337UL);
  return 0x5678;
}

void TestPatchRaw()
{
  hadesmem::Process const& process = GetThisProcess();

  hadesmem::Allocator const test_mem(process, 0x1000);

  std::vector<BYTE> const data = {0x00, 0x11, 0x22, 0x33, 0x44};
  BOOST_TEST_EQ(data.size(), 5UL);

  hadesmem::PatchRaw patch(process, test_mem.GetBase(), data);

  auto const orig = hadesmem::ReadVector<BYTE>(process, test_mem.GetBase(), 5);

  patch.Apply();

  auto const apply = hadesmem::ReadVector<BYTE>(process, test_mem.GetBase(), 5);

  patch.Remove();

  auto const remove =
    hadesmem::ReadVector<BYTE>(process, test_mem.GetBase(), 5);

  BOOST_TEST(orig == remove);
  BOOST_TEST(orig != apply);
  BOOST_TEST(data == apply);
}

void TestPatchDetour()
{
  using HookMeFuncBuilderT = asmjit::FuncBuilder8<std::uint32_t,
                                                  std::int32_t,
                                                  std::int32_t,
                                                  std::int32_t,
                                                  std::int32_t,
                                                  std::int32_t,
                                                  std::int32_t,
                                                  std::int32_t,
                                                  std::int32_t>;

  asmjit::JitRuntime runtime;
  asmjit::host::Compiler c(&runtime);
#if defined(HADESMEM_DETAIL_ARCH_X64)
  auto const call_conv = asmjit::host::kFuncConvHost;
#elif defined(HADESMEM_DETAIL_ARCH_X86)
  auto const call_conv = asmjit::host::kFuncConvCDecl;
#else
#error "[HadesMem] Unsupported architecture."
#endif
  c.addFunc(call_conv, HookMeFuncBuilderT());

  c.nop();
  c.nop();
  c.nop();
  c.nop();
  c.nop();
  c.nop();
  c.nop();
  c.nop();
  c.nop();
  c.nop();
  c.nop();

  asmjit::host::GpVar a1(c, asmjit::kVarTypeInt32);
  c.setArg(0, a1);
  asmjit::host::GpVar a2(c, asmjit::kVarTypeInt32);
  c.setArg(1, a2);
  asmjit::host::GpVar a3(c, asmjit::kVarTypeInt32);
  c.setArg(2, a3);
  asmjit::host::GpVar a4(c, asmjit::kVarTypeInt32);
  c.setArg(3, a4);
  asmjit::host::GpVar a5(c, asmjit::kVarTypeInt32);
  c.setArg(4, a5);
  asmjit::host::GpVar a6(c, asmjit::kVarTypeInt32);
  c.setArg(5, a6);
  asmjit::host::GpVar a7(c, asmjit::kVarTypeInt32);
  c.setArg(6, a7);
  asmjit::host::GpVar a8(c, asmjit::kVarTypeInt32);
  c.setArg(7, a8);

  asmjit::host::GpVar address(c.newGpVar());
  c.mov(address, asmjit::imm_u(reinterpret_cast<std::uintptr_t>(&HookMe)));

  asmjit::host::GpVar var(c.newGpVar());
  asmjit::host::X86X64CallNode* ctx =
    c.call(address, call_conv, HookMeFuncBuilderT());
  ctx->setArg(0, a1);
  ctx->setArg(1, a2);
  ctx->setArg(2, a3);
  ctx->setArg(3, a4);
  ctx->setArg(4, a5);
  ctx->setArg(5, a6);
  ctx->setArg(6, a7);
  ctx->setArg(7, a8);
  ctx->setRet(0, var);

  c.ret(var);

  c.endFunc();

  auto const free_asmjit_func = [](void* func)
  { asmjit::MemoryManager::getGlobal()->release(func); };
  void* const hook_me_wrapper_raw = c.make();
  std::unique_ptr<void, decltype(free_asmjit_func)> const
    hook_me_wrapper_cleanup(hook_me_wrapper_raw, free_asmjit_func);

  auto const volatile hook_me_wrapper =
    hadesmem::detail::UnionCast<decltype(&HookMe)>(hook_me_wrapper_raw);

  auto const hook_me_packaged = [=]()
  { return hook_me_wrapper(1, 2, 3, 4, 5, 6, 7, 8); };
  BOOST_TEST_EQ(hook_me_packaged(), 0x1234UL);

  hadesmem::Process const& process = GetThisProcess();

  auto& detour_1 = GetDetour1();
  detour_1 = std::make_unique<hadesmem::PatchDetour>(
    process,
    hadesmem::detail::UnionCast<PVOID>(hook_me_wrapper),
    hadesmem::detail::UnionCast<PVOID>(&HookMeHk));

  auto& detour_2 = GetDetour2();
  detour_2 = std::make_unique<hadesmem::PatchDetour>(
    process,
    hadesmem::detail::UnionCast<PVOID>(hook_me_wrapper),
    hadesmem::detail::UnionCast<PVOID>(&HookMeHk2));

  BOOST_TEST_EQ(hook_me_packaged(), 0x1234UL);

  detour_1->Apply();

  BOOST_TEST_EQ(hook_me_packaged(), 0x1337UL);

  detour_2->Apply();

  BOOST_TEST_EQ(hook_me_packaged(), 0x5678UL);

  detour_2->Remove();

  BOOST_TEST_EQ(hook_me_packaged(), 0x1337UL);

  detour_1->Remove();

  BOOST_TEST_EQ(hook_me_packaged(), 0x1234UL);

  detour_1->Apply();

  BOOST_TEST_EQ(hook_me_packaged(), 0x1337UL);

  detour_2->Apply();

  BOOST_TEST_EQ(hook_me_packaged(), 0x5678UL);

  detour_2->Remove();

  BOOST_TEST_EQ(hook_me_packaged(), 0x1337UL);

  detour_1->Remove();

  BOOST_TEST_EQ(hook_me_packaged(), 0x1234UL);
}

int main()
{
  TestPatchRaw();
  TestPatchDetour();
  return boost::report_errors();
}
