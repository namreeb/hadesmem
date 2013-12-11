// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/call.hpp>

#include <cstdint>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/static_assert.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>

// TODO: Test argument combinations more thoroughly.
// TODO: Improve multi-call testing.
// TODO: Test all possible Call overloads.
// TODO: Compile-fail tests.

struct DummyType
{
};
DummyType dummy_glob;

using IntRetFuncT = std::int32_t (*)();
HADESMEM_DETAIL_STATIC_ASSERT(std::is_same<
  decltype(hadesmem::Call<IntRetFuncT>(
    std::declval<hadesmem::Process>(), nullptr, hadesmem::CallConv::kDefault)
             .GetReturnValue()),
  std::int32_t>::value);

using DoubleRetFuncT = double (*)();
HADESMEM_DETAIL_STATIC_ASSERT(std::is_same<
  decltype(hadesmem::Call<DoubleRetFuncT>(
    std::declval<hadesmem::Process>(), nullptr, hadesmem::CallConv::kDefault)
             .GetReturnValue()),
  double>::value);

using PtrRetFuncT = DummyType* (*)();
HADESMEM_DETAIL_STATIC_ASSERT(std::is_same<
  decltype(hadesmem::Call<PtrRetFuncT>(
    std::declval<hadesmem::Process>(), nullptr, hadesmem::CallConv::kDefault)
             .GetReturnValue()),
  DummyType*>::value);

DWORD_PTR TestInteger(std::uint32_t a,
                      std::uint32_t b,
                      std::uint32_t c,
                      std::uint32_t d,
                      std::uint32_t e,
                      std::uint32_t f)
{
  BOOST_TEST_EQ(a, 0xAAAAAAAAU);
  BOOST_TEST_EQ(b, 0xBBBBBBBBU);
  BOOST_TEST_EQ(c, 0xCCCCCCCCU);
  BOOST_TEST_EQ(d, 0xDDDDDDDDU);
  BOOST_TEST_EQ(e, 0xEEEEEEEEU);
  BOOST_TEST_EQ(f, 0xFFFFFFFFU);

  SetLastError(0x87654321);

  return 0x12345678;
}

float TestFloat(float a, float b, float c, float d, float e, float f)
{
  BOOST_TEST_EQ(a, 1.11111f);
  BOOST_TEST_EQ(b, 2.22222f);
  BOOST_TEST_EQ(c, 3.33333f);
  BOOST_TEST_EQ(d, 4.44444f);
  BOOST_TEST_EQ(e, 5.55555f);
  BOOST_TEST_EQ(f, 6.66666f);

  return 1.23456f;
}

double TestDouble(double a, double b, double c, double d, double e, double f)
{
  BOOST_TEST_EQ(a, 1.11111);
  BOOST_TEST_EQ(b, 2.22222);
  BOOST_TEST_EQ(c, 3.33333);
  BOOST_TEST_EQ(d, 4.44444);
  BOOST_TEST_EQ(e, 5.55555);
  BOOST_TEST_EQ(f, 6.66666);

  return 1.23456;
}

DWORD_PTR TestMixed(double a,
                    void const* b,
                    char c,
                    float d,
                    std::int32_t e,
                    std::uint32_t f,
                    float g,
                    double h,
                    DummyType const* i,
                    std::uint64_t j)
{
  BOOST_TEST_EQ(a, 1337.6666);
  BOOST_TEST_EQ(b, static_cast<void const*>(nullptr));
  BOOST_TEST_EQ(c, 'c');
  BOOST_TEST_EQ(d, 9081.736455f);
  BOOST_TEST_EQ(e, -1234);
  BOOST_TEST_EQ(f, 0xDEAFBEEFU);
  BOOST_TEST_EQ(g, 1234.56f);
  BOOST_TEST_EQ(h, 9876.54);
  BOOST_TEST_EQ(i, &dummy_glob);
  BOOST_TEST_EQ(j, 0xAAAAAAAABBBBBBBBULL);

  SetLastError(5678);
  return 1234;
}

std::uint32_t TestRvalueOnly(std::uint32_t&& a)
{
  BOOST_TEST_EQ(a, 42U);
  return a;
}

std::uint32_t TestInteger64(DWORD64 a)
{
  BOOST_TEST_EQ(a, 0xAAAAAAAABBBBBBBBULL);

  return 0;
}

std::uint64_t TestCall64Ret()
{
  return 0x123456787654321LL;
}

float TestCallFloatRet()
{
  return 1.234f;
}

double TestCallDoubleRet()
{
  return 9.876;
}

void TestCallVoidRet()
{
}

char const* TestPtrRet()
{
  return nullptr;
}

void MultiThreadSet(DWORD last_error)
{
  SetLastError(last_error);
}

DWORD MultiThreadGet()
{
  return GetLastError();
}

class ThiscallDummy
{
public:
  DWORD_PTR TestIntegerThis(std::uint32_t a,
                            std::uint32_t b,
                            std::uint32_t c,
                            std::uint32_t d,
                            std::uint32_t e) const
  {
    BOOST_TEST_EQ(a, 0xAAAAAAAAU);
    BOOST_TEST_EQ(b, 0xBBBBBBBBU);
    BOOST_TEST_EQ(c, 0xCCCCCCCCU);
    BOOST_TEST_EQ(d, 0xDDDDDDDDU);
    BOOST_TEST_EQ(e, 0xEEEEEEEEU);

    SetLastError(0x87654321);

    return 0x12345678;
  }
};

#if defined(HADESMEM_DETAIL_ARCH_X64)

#elif defined(HADESMEM_DETAIL_ARCH_X86)

DWORD_PTR __fastcall TestIntegerFast(std::uint32_t a,
                                     std::uint32_t b,
                                     std::uint32_t c,
                                     std::uint32_t d,
                                     std::uint32_t e,
                                     std::uint32_t f)
{
  BOOST_TEST_EQ(a, 0xAAAAAAAAU);
  BOOST_TEST_EQ(b, 0xBBBBBBBBU);
  BOOST_TEST_EQ(c, 0xCCCCCCCCU);
  BOOST_TEST_EQ(d, 0xDDDDDDDDU);
  BOOST_TEST_EQ(e, 0xEEEEEEEEU);
  BOOST_TEST_EQ(f, 0xFFFFFFFFU);

  SetLastError(0x87654321);

  return 0x12345678;
}

DWORD_PTR __stdcall TestIntegerStd(std::uint32_t a,
                                   std::uint32_t b,
                                   std::uint32_t c,
                                   std::uint32_t d,
                                   std::uint32_t e,
                                   std::uint32_t f)
{
  BOOST_TEST_EQ(a, 0xAAAAAAAAU);
  BOOST_TEST_EQ(b, 0xBBBBBBBBU);
  BOOST_TEST_EQ(c, 0xCCCCCCCCU);
  BOOST_TEST_EQ(d, 0xDDDDDDDDU);
  BOOST_TEST_EQ(e, 0xEEEEEEEEU);
  BOOST_TEST_EQ(f, 0xFFFFFFFFU);

  SetLastError(0x87654321);

  return 0x12345678;
}

std::int32_t __fastcall TestInteger64Fast(std::uint64_t a)
{
  BOOST_TEST_EQ(a, 0xAAAAAAAABBBBBBBBULL);

  return 0;
}

#else
#error "[HadesMem] Unsupported architecture."
#endif

void TestCall()
{
  hadesmem::Process const process(::GetCurrentProcessId());

  auto const call_int_ret = hadesmem::Call(process,
                                           &TestInteger,
                                           hadesmem::CallConv::kDefault,
                                           0xAAAAAAAAU,
                                           0xBBBBBBBBU,
                                           0xCCCCCCCCU,
                                           0xDDDDDDDDU,
                                           0xEEEEEEEEU,
                                           0xFFFFFFFFU);
  BOOST_TEST_EQ(call_int_ret.GetReturnValue(), 0x12345678UL);

  auto const call_int_ret_2 = hadesmem::Call(process,
                                             TestInteger,
                                             hadesmem::CallConv::kDefault,
                                             0xAAAAAAAAU,
                                             0xBBBBBBBBU,
                                             0xCCCCCCCCU,
                                             0xDDDDDDDDU,
                                             0xEEEEEEEEU,
                                             0xFFFFFFFFU);
  BOOST_TEST_EQ(call_int_ret_2.GetReturnValue(), 0x12345678UL);

  auto const call_float_ret = hadesmem::Call(process,
                                             &TestFloat,
                                             hadesmem::CallConv::kDefault,
                                             1.11111f,
                                             2.22222f,
                                             3.33333f,
                                             4.44444f,
                                             5.55555f,
                                             6.66666f);
  BOOST_TEST_EQ(call_float_ret.GetReturnValue(), 1.23456f);

  auto const call_double_ret = hadesmem::Call(process,
                                              &TestDouble,
                                              hadesmem::CallConv::kDefault,
                                              1.11111,
                                              2.22222,
                                              3.33333,
                                              4.44444,
                                              5.55555,
                                              6.66666);
  BOOST_TEST_EQ(call_double_ret.GetReturnValue(), 1.23456);

  struct ImplicitConvTest
  {
    operator int() const
    {
      return -1234;
    }
  };
  std::uint32_t const lvalue_int = 0xDEAFBEEF;
  float const lvalue_float = 1234.56f;
  auto const call_ret = hadesmem::Call(process,
                                       &TestMixed,
                                       hadesmem::CallConv::kDefault,
                                       1337.6666,
                                       nullptr,
                                       'c',
                                       9081.736455f,
                                       ImplicitConvTest(),
                                       lvalue_int,
                                       lvalue_float,
                                       9876.54,
                                       &dummy_glob,
                                       0xAAAAAAAABBBBBBBBULL);
  BOOST_TEST_EQ(call_ret.GetReturnValue(), 1234UL);
  BOOST_TEST_EQ(call_ret.GetLastError(), 5678UL);

// TODO: Add a new compile-fail test to ensure that this (and other
// similar scenarios -- one test for each) doesn't compile.
#if 0
    std::uint32_t const lvalue_int_2 = 42U;
    hadesmem::Call(
        process,
        &TestRvalueOnly,
        hadesmem::CallConv::kDefault,
        lvalue_int_2);
#endif

// TODO: Reenable this once we fix reference support (and also add an
// lvalue reference test).
#if 0
    hadesmem::Call(
        process,
        &TestRvalueOnly,
        hadesmem::CallConv::kDefault,
        42U);
#endif

  hadesmem::Call(process,
                 &TestInteger64,
                 hadesmem::CallConv::kDefault,
                 0xAAAAAAAABBBBBBBBULL);

  auto const call_ptr_ret =
    hadesmem::Call(process, &TestPtrRet, hadesmem::CallConv::kDefault);
  BOOST_TEST_EQ(call_ptr_ret.GetReturnValue(),
                static_cast<char const* const>(nullptr));

#if defined(HADESMEM_DETAIL_ARCH_X64)
  hadesmem::CallConv const thiscall_call_conv = hadesmem::CallConv::kDefault;
#elif defined(HADESMEM_DETAIL_ARCH_X86)
  hadesmem::CallConv const thiscall_call_conv = hadesmem::CallConv::kThisCall;
#else
#error "[HadesMem] Unsupported architecture."
#endif
// The following is an extremely disgusting hack that should never be
// attempted anywhere by anyone. It relies on the fact that all tested
// compilers will lay out the structures representing pointer to member
// functions with the function address first (and other data after).
// The C++ standard strictly forbids casting between the types below.
// The C++ standard also forbids type punning using the technique below
// (aka doing a store to a union using one type then doing a load from
// the union using another type).
// TODO: Find a way to do this without relying on undefined behavior.
#if 0
    union FuncConv
    {
        decltype(&ThiscallDummy::TestIntegerThis) pmfn;
        void* pfn;
    };
    FuncConv func_conv;
    func_conv.pmfn = &ThiscallDummy::TestIntegerThis;
    auto const test_integer_this = func_conv.pfn;
#endif
  ThiscallDummy const thiscall_dummy;
  auto const call_int_this_ret = hadesmem::Call(process,
                                                &ThiscallDummy::TestIntegerThis,
                                                thiscall_call_conv,
                                                &thiscall_dummy,
                                                0xAAAAAAAA,
                                                0xBBBBBBBB,
                                                0xCCCCCCCC,
                                                0xDDDDDDDD,
                                                0xEEEEEEEE);
  BOOST_TEST_EQ(call_int_this_ret.GetReturnValue(), 0x12345678UL);
  BOOST_TEST_EQ(call_int_this_ret.GetLastError(), 0x87654321UL);

#if defined(HADESMEM_DETAIL_ARCH_X64)

#elif defined(HADESMEM_DETAIL_ARCH_X86)

  auto const call_int_fast_ret = hadesmem::Call(process,
                                                &TestIntegerFast,
                                                hadesmem::CallConv::kFastCall,
                                                0xAAAAAAAA,
                                                0xBBBBBBBB,
                                                0xCCCCCCCC,
                                                0xDDDDDDDD,
                                                0xEEEEEEEE,
                                                0xFFFFFFFF);
  BOOST_TEST_EQ(call_int_fast_ret.GetReturnValue(), 0x12345678UL);
  BOOST_TEST_EQ(call_int_fast_ret.GetLastError(), 0x87654321UL);

  auto const call_int_std_ret = hadesmem::Call(process,
                                               &TestIntegerStd,
                                               hadesmem::CallConv::kStdCall,
                                               0xAAAAAAAA,
                                               0xBBBBBBBB,
                                               0xCCCCCCCC,
                                               0xDDDDDDDD,
                                               0xEEEEEEEE,
                                               0xFFFFFFFF);
  BOOST_TEST_EQ(call_int_std_ret.GetReturnValue(), 0x12345678UL);
  BOOST_TEST_EQ(call_int_std_ret.GetLastError(), 0x87654321UL);

  hadesmem::Call(process,
                 &TestInteger64Fast,
                 hadesmem::CallConv::kFastCall,
                 0xAAAAAAAABBBBBBBBULL);

#else
#error "[HadesMem] Unsupported architecture."
#endif

  auto const call_ret_64 =
    hadesmem::Call(process, &TestCall64Ret, hadesmem::CallConv::kDefault);
  BOOST_TEST_EQ(call_ret_64.GetReturnValue(), 0x123456787654321ULL);

  auto const call_ret_float =
    hadesmem::Call(process, &TestCallFloatRet, hadesmem::CallConv::kDefault);
  BOOST_TEST_EQ(call_ret_float.GetReturnValue(), 1.234f);

  auto const call_ret_double =
    hadesmem::Call(process, &TestCallDoubleRet, hadesmem::CallConv::kDefault);
  BOOST_TEST_EQ(call_ret_double.GetReturnValue(), 9.876);

  auto const call_ret_void =
    hadesmem::Call(process, &TestCallVoidRet, hadesmem::CallConv::kDefault);
  BOOST_TEST_EQ(call_ret_void.GetLastError(), 0U);

  HMODULE const kernel32_mod = ::GetModuleHandle(L"kernel32.dll");
  BOOST_TEST(kernel32_mod != nullptr);

#if defined(HADESMEM_MSVC)
#pragma warning(push)
#pragma warning(disable : 6387)
#endif // #if defined(HADESMEM_MSVC)

  FARPROC const get_proc_address =
    ::GetProcAddress(kernel32_mod, "GetProcAddress");
  BOOST_TEST(get_proc_address != nullptr);

#if defined(HADESMEM_MSVC)
#pragma warning(pop)
#endif // #if defined(HADESMEM_MSVC)

  auto const call_win = hadesmem::Call(
    process,
    reinterpret_cast<decltype(&GetProcAddress)>(get_proc_address),
    hadesmem::CallConv::kWinApi,
    kernel32_mod,
    "GetProcAddress");
  BOOST_TEST_EQ(call_win.GetReturnValue(), get_proc_address);

  hadesmem::MultiCall multi_call(process);
  multi_call.Add<void (*)(DWORD)>(
    &MultiThreadSet, hadesmem::CallConv::kDefault, 0x1337UL);
  multi_call.Add<DWORD (*)()>(&MultiThreadGet, hadesmem::CallConv::kDefault);
  multi_call.Add<void (*)(DWORD)>(
    &MultiThreadSet, hadesmem::CallConv::kDefault, 0x1234UL);
  multi_call.Add<DWORD (*)()>(&MultiThreadGet, hadesmem::CallConv::kDefault);
  std::vector<hadesmem::CallResultRaw> multi_call_ret;
  multi_call.Call(std::back_inserter(multi_call_ret));
  BOOST_TEST_EQ(multi_call_ret[0].GetLastError(), 0x1337UL);
  BOOST_TEST_EQ(multi_call_ret[1].GetReturnValue<DWORD_PTR>(), 0x1337U);
  BOOST_TEST_EQ(multi_call_ret[2].GetLastError(), 0x1234UL);
  BOOST_TEST_EQ(multi_call_ret[3].GetReturnValue<DWORD_PTR>(), 0x1234U);
}

int main()
{
  TestCall();
  return boost::report_errors();
}
