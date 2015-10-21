// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include <hadesmem/patcher.hpp>
#include <hadesmem/patcher.hpp>

#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <asmjit/asmjit.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/alias_cast.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>

// TODO: Patcher constructor tests

// TODO: Address tests.

// TODO: Stream overload tests.

// TODO: Fix the code so the Clang warning disabling hack can be removed.

// TODO: Generate different kinds of code to test all instruction resolution
// code and ensure we're covering all the important cases.

// TODO: Test different calling conventions etc.

hadesmem::Process& GetThisProcess()
{
  static hadesmem::Process process(::GetCurrentProcessId());
  return process;
}

__declspec(noinline) std::uint32_t __cdecl HookMe(std::int32_t i1,
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

std::unique_ptr<hadesmem::PatchDetour<decltype(&HookMe)>>& GetDetour1()
{
  static std::unique_ptr<hadesmem::PatchDetour<decltype(&HookMe)>> detour;
  return detour;
}

std::unique_ptr<hadesmem::PatchDetour<decltype(&HookMe)>>& GetDetour2()
{
  static std::unique_ptr<hadesmem::PatchDetour<decltype(&HookMe)>> detour;
  return detour;
}

extern "C" std::uint32_t __cdecl HookMeHk(hadesmem::PatchDetourBase* patch,
                                          std::int32_t i1,
                                          std::int32_t i2,
                                          std::int32_t i3,
                                          std::int32_t i4,
                                          std::int32_t i5,
                                          std::int32_t i6,
                                          std::int32_t i7,
                                          std::int32_t i8)
{
  BOOST_TEST(patch->GetTrampoline() != nullptr);
  auto const orig = patch->GetTrampolineT<decltype(&HookMe)>();
  BOOST_TEST_EQ(orig(i1, i2, i3, i4, i5, i6, i7, i8), 0x1234UL);
  return 0x1337;
}

extern "C" std::uint32_t __cdecl HookMeHk2(hadesmem::PatchDetourBase* patch,
                                           std::int32_t i1,
                                           std::int32_t i2,
                                           std::int32_t i3,
                                           std::int32_t i4,
                                           std::int32_t i5,
                                           std::int32_t i6,
                                           std::int32_t i7,
                                           std::int32_t i8)
{
  BOOST_TEST(patch->GetTrampoline() != nullptr);
  auto const orig = patch->GetTrampolineT<decltype(&HookMe)>();
  BOOST_TEST_EQ(orig(i1, i2, i3, i4, i5, i6, i7, i8), 0x1337UL);
  return 0x5678;
}

extern "C" __declspec(noinline) int __stdcall Scratch(int a, float b, void* c)
{
  BOOST_TEST_EQ(a, -42);
  BOOST_TEST_EQ(b, 2.f);
  BOOST_TEST_EQ(c, static_cast<void*>(nullptr));
  return 0x1337;
}

std::unique_ptr<hadesmem::PatchDetour<decltype(Scratch)>>& GetDetour3()
{
  static std::unique_ptr<hadesmem::PatchDetour<decltype(Scratch)>> detour;
  return detour;
}

extern "C" int __cdecl ScratchDetour(hadesmem::PatchDetourBase* patch,
                                     int a,
                                     float b,
                                     void* c)
{
  BOOST_TEST_EQ(*static_cast<void**>(patch->GetContext()), &GetThisProcess());
  BOOST_TEST(patch->GetTrampoline() != nullptr);
  auto const orig = patch->GetTrampolineT<decltype(&Scratch)>();
  BOOST_TEST_EQ(orig(a, b, c), 0x1337);
  return 0x42424242;
}

void TestPatchDetour2()
{
  auto volatile const scratch_fn = &Scratch;
  BOOST_TEST_EQ(scratch_fn(-42, 2.f, nullptr), 0x1337);
  auto& detour_3 = GetDetour3();
  detour_3 = std::make_unique<hadesmem::PatchDetour<decltype(Scratch)>>(
    GetThisProcess(), scratch_fn, &ScratchDetour, &GetThisProcess());
  detour_3->Apply();
  BOOST_TEST_EQ(scratch_fn(-42, 2.f, nullptr), 0x42424242);
  detour_3->Remove();
  BOOST_TEST_EQ(scratch_fn(-42, 2.f, nullptr), 0x1337);
  detour_3->Apply();
  BOOST_TEST_EQ(scratch_fn(-42, 2.f, nullptr), 0x42424242);
  detour_3->Remove();
  BOOST_TEST_EQ(scratch_fn(-42, 2.f, nullptr), 0x1337);
  detour_3 = nullptr;
  BOOST_TEST_EQ(scratch_fn(-42, 2.f, nullptr), 0x1337);

  bool test_var = true;
  auto const scratch_detour =
    [&](hadesmem::PatchDetourBase* patch, int a, float b, void* c)
  {
    BOOST_TEST_EQ(test_var, true);
    BOOST_TEST_EQ(*static_cast<void**>(patch->GetContext()), &test_var);
    BOOST_TEST(patch->GetTrampoline() != nullptr);
    auto const orig = patch->GetTrampolineT<decltype(&Scratch)>();
    BOOST_TEST_EQ(orig(a, b, c), 0x1337);
    return 0xDEADBEEF;
  };
  detour_3 = std::make_unique<hadesmem::PatchDetour<decltype(Scratch)>>(
    GetThisProcess(), scratch_fn, scratch_detour, &test_var);
  detour_3->Apply();
  BOOST_TEST_EQ(scratch_fn(-42, 2.f, nullptr), 0xDEADBEEF);
  detour_3->Remove();
  BOOST_TEST_EQ(scratch_fn(-42, 2.f, nullptr), 0x1337);
  detour_3->Apply();
  BOOST_TEST_EQ(scratch_fn(-42, 2.f, nullptr), 0xDEADBEEF);
  detour_3->Remove();
  BOOST_TEST_EQ(scratch_fn(-42, 2.f, nullptr), 0x1337);
  detour_3 = nullptr;
  BOOST_TEST_EQ(scratch_fn(-42, 2.f, nullptr), 0x1337);
}

void TestPatchRaw()
{
  hadesmem::Process const& process = GetThisProcess();

  hadesmem::Allocator const test_mem{process, 0x1000};

  std::vector<BYTE> const data = {0x00, 0x11, 0x22, 0x33, 0x44};
  BOOST_TEST_EQ(data.size(), 5UL);

  hadesmem::PatchRaw patch{process, test_mem.GetBase(), data};

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

void GenerateBasicCall(asmjit::X86Compiler& c)
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

#if defined(HADESMEM_DETAIL_ARCH_X64)
  auto const call_conv = asmjit::kFuncConvHost;
#elif defined(HADESMEM_DETAIL_ARCH_X86)
  auto const call_conv = asmjit::kFuncConvHostCDecl;
#else
#error "[HadesMem] Unsupported architecture."
#endif
  c.addFunc(call_conv, HookMeFuncBuilderT());
  c.getFunc()->setHint(asmjit::kFuncHintNaked, true);

  asmjit::GpVar a1(c, asmjit::kVarTypeInt32);
  c.setArg(0, a1);
  asmjit::GpVar a2(c, asmjit::kVarTypeInt32);
  c.setArg(1, a2);
  asmjit::GpVar a3(c, asmjit::kVarTypeInt32);
  c.setArg(2, a3);
  asmjit::GpVar a4(c, asmjit::kVarTypeInt32);
  c.setArg(3, a4);
  asmjit::GpVar a5(c, asmjit::kVarTypeInt32);
  c.setArg(4, a5);
  asmjit::GpVar a6(c, asmjit::kVarTypeInt32);
  c.setArg(5, a6);
  asmjit::GpVar a7(c, asmjit::kVarTypeInt32);
  c.setArg(6, a7);
  asmjit::GpVar a8(c, asmjit::kVarTypeInt32);
  c.setArg(7, a8);

  asmjit::GpVar address(c.newGpVar());
  c.mov(address, asmjit::imm_u(reinterpret_cast<std::uintptr_t>(&HookMe)));

  asmjit::GpVar var(c.newGpVar());
  asmjit::X86CallNode* ctx = c.call(address, call_conv, HookMeFuncBuilderT());
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
}

void GenerateBasicJmp(asmjit::X86Assembler& a)
{
  a.jmp(asmjit::imm_ptr(reinterpret_cast<void*>(&HookMe)));
}

template <typename PatchType, typename WrapperFunc, typename PackagedFunc>
void TestPatchDetourCommon(WrapperFunc hook_me_wrapper,
                           PackagedFunc hook_me_packaged)
{
  hadesmem::Process const& process = GetThisProcess();

  auto& detour_1 = GetDetour1();
  detour_1 = std::make_unique<PatchType>(process, hook_me_wrapper, &HookMeHk);

  bool const can_chain = detour_1->CanHookChain();

  auto& detour_2 = GetDetour2();
  if (can_chain)
  {
    detour_2 =
      std::make_unique<PatchType>(process, hook_me_wrapper, &HookMeHk2);
  }

  BOOST_TEST_EQ(hook_me_packaged(), 0x1234UL);

  detour_1->Apply();

  BOOST_TEST_EQ(hook_me_packaged(), 0x1337UL);

  if (can_chain)
  {
    detour_2->Apply();

    BOOST_TEST_EQ(hook_me_packaged(), 0x5678UL);

    detour_2->Remove();

    BOOST_TEST_EQ(hook_me_packaged(), 0x1337UL);
  }

  detour_1->Remove();

  BOOST_TEST_EQ(hook_me_packaged(), 0x1234UL);

  detour_1->Apply();

  BOOST_TEST_EQ(hook_me_packaged(), 0x1337UL);

  if (can_chain)
  {
    detour_2->Apply();

    BOOST_TEST_EQ(hook_me_packaged(), 0x5678UL);

    detour_2->Remove();

    BOOST_TEST_EQ(hook_me_packaged(), 0x1337UL);
  }

  detour_1->Remove();

  BOOST_TEST_EQ(hook_me_packaged(), 0x1234UL);

  detour_2 = nullptr;
  detour_1 = nullptr;
}

class AsmJitMemoryReleaser
{
public:
  AsmJitMemoryReleaser(asmjit::JitRuntime& runtime, void* func)
    : runtime_{&runtime}, func_{func}
  {
  }

  AsmJitMemoryReleaser(AsmJitMemoryReleaser const& other) = delete;

  AsmJitMemoryReleaser& operator=(AsmJitMemoryReleaser const& other) = delete;

  AsmJitMemoryReleaser(AsmJitMemoryReleaser&& other)
    : runtime_{other.runtime_}, func_{other.func_}
  {
    other.runtime_ = nullptr;
    other.func_ = nullptr;
  }

  AsmJitMemoryReleaser& operator=(AsmJitMemoryReleaser&& other)
  {
    CleanupUnchecked();

    std::swap(runtime_, other.runtime_);
    std::swap(func_, other.func_);

    return *this;
  }

  ~AsmJitMemoryReleaser()
  {
    CleanupUnchecked();
  }

  void Cleanup()
  {
    if (func_)
    {
      auto const error = runtime_->release(func_);
      (void)error;
      HADESMEM_DETAIL_ASSERT(error == 0);
    }

    runtime_ = nullptr;
    func_ = nullptr;
  }

  void CleanupUnchecked()
  {
    try
    {
      Cleanup();
    }
    catch (...)
    {
      HADESMEM_DETAIL_TRACE_A(
        boost::current_exception_diagnostic_information().c_str());
      HADESMEM_DETAIL_ASSERT(false);
    }

    runtime_ = nullptr;
    func_ = nullptr;
  }

private:
  asmjit::JitRuntime* runtime_;
  void* func_;
};

using HookPackageData = std::tuple<decltype(&HookMe),
                                   std::function<std::uint32_t()>,
                                   AsmJitMemoryReleaser>;

HookPackageData GenerateAndCheckHookPackage(asmjit::JitRuntime& runtime,
                                            asmjit::CodeGen& c)
{
  void* const hook_me_wrapper_raw = c.make();
  AsmJitMemoryReleaser hook_me_wrapper_cleanup{runtime, hook_me_wrapper_raw};

  auto const volatile hook_me_wrapper =
    hadesmem::detail::AliasCast<decltype(&HookMe)>(hook_me_wrapper_raw);

  auto const hook_me_packaged = [=]()
  {
    return hook_me_wrapper(1, 2, 3, 4, 5, 6, 7, 8);
  };
  BOOST_TEST_EQ(hook_me_packaged(), 0x1234UL);

  return std::make_tuple(
    hook_me_wrapper, hook_me_packaged, std::move(hook_me_wrapper_cleanup));
}

template <typename PatchType> void TestPatchDetourCall()
{
  asmjit::JitRuntime runtime;
  asmjit::X86Compiler c{&runtime};
  GenerateBasicCall(c);
  auto const wrapper_and_package = GenerateAndCheckHookPackage(runtime, c);
  TestPatchDetourCommon<PatchType>(std::get<0>(wrapper_and_package),
                                   std::get<1>(wrapper_and_package));
}

template <typename PatchType> void TestPatchDetourJmp()
{
  asmjit::JitRuntime runtime;
  asmjit::X86Assembler a{&runtime};
  GenerateBasicJmp(a);
  auto const wrapper_and_package = GenerateAndCheckHookPackage(runtime, a);
  TestPatchDetourCommon<PatchType>(std::get<0>(wrapper_and_package),
                                   std::get<1>(wrapper_and_package));
}

void TestPatchDetour()
{
  TestPatchDetourCall<hadesmem::PatchDetour<decltype(&HookMe)>>();
  TestPatchDetourJmp<hadesmem::PatchDetour<decltype(&HookMe)>>();
}

void TestPatchInt3()
{
  TestPatchDetourCall<hadesmem::PatchInt3<decltype(&HookMe)>>();
  TestPatchDetourJmp<hadesmem::PatchInt3<decltype(&HookMe)>>();
}

void TestPatchDr()
{
  TestPatchDetourCall<hadesmem::PatchDr<decltype(&HookMe)>>();
  TestPatchDetourJmp<hadesmem::PatchDr<decltype(&HookMe)>>();
}

__declspec(noinline) void TestGetLastErrorOrig()
{
  ::SetLastError(0x1234);
  BOOST_TEST_EQ(::GetLastError(), 0x1234);
}

__declspec(noinline) void TestGetLastErrorHooked()
{
  ::SetLastError(0x1234);
  BOOST_TEST_EQ(::GetLastError(), 0x1337UL);
}

void TestPatchIat()
{
  hadesmem::Process const& process = GetThisProcess();
  auto const kernel32_mod = GetModuleHandleW(L"kernel32.dll");
  BOOST_TEST_NE(kernel32_mod, static_cast<HMODULE>(nullptr));
  __analysis_assume(kernel32_mod != nullptr);
  TestGetLastErrorOrig();
  auto volatile const get_last_error_orig =
    GetProcAddress(kernel32_mod, "GetLastError");
  auto const get_last_error_detour = [](
    hadesmem::PatchDetourBase* patch) -> DWORD
  {
    (void)patch;
    return 0x1337UL;
  };
  hadesmem::PatchIat<decltype(&::GetLastError)> get_last_error_patch{
    process, L"kernel32.dll", "GetLastError", get_last_error_detour};
  get_last_error_patch.Apply();
  auto volatile const get_last_error_hooked =
    GetProcAddress(kernel32_mod, "GetLastError");
  // TODO: Need a proper implementation that doesn't use multiple stubs in order
  // for this to work...
  // BOOST_TEST_EQ(get_last_error_hooked, get_last_error_hooked_2);
  BOOST_TEST_NE(get_last_error_orig, get_last_error_hooked);
  TestGetLastErrorHooked();
  get_last_error_patch.Remove();
  TestGetLastErrorOrig();
}

int main()
{
  TestPatchRaw();
  TestPatchDetour();
  TestPatchInt3();
  TestPatchDr();
  TestPatchDetour2();
  TestPatchIat();
  return boost::report_errors();
}
