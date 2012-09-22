// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/call.hpp"

#define BOOST_TEST_MODULE call
#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/test/unit_test.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/error.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/detail/union_cast.hpp"
#include "hadesmem/detail/static_assert.hpp"

// Boost.Test causes the following warning under GCC:
// error: base class 'struct boost::unit_test::ut_detail::nil_t' has a 
// non-virtual destructor [-Werror=effc++]
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic ignored "-Weffc++"
#endif // #if defined(HADESMEM_GCC)

// TODO: Test argument combinations more thoroughly.
// TODO: Improve multi-call testing.
// TODO: Test all possible Call overloads.

namespace
{

struct DummyType { };
DummyType dummy_glob;

typedef int (* IntRetFuncT)();
HADESMEM_STATIC_ASSERT(std::is_same<
  decltype(hadesmem::Call<IntRetFuncT>(std::declval<hadesmem::Process>(), 
  nullptr, hadesmem::CallConv::kDefault).first), 
  int>::value);

typedef double (* DoubleRetFuncT)();
HADESMEM_STATIC_ASSERT(std::is_same<
  decltype(hadesmem::Call<DoubleRetFuncT>(std::declval<hadesmem::Process>(), 
  nullptr, hadesmem::CallConv::kDefault).first), 
  double>::value);

typedef DummyType* (* PtrRetFuncT)();
HADESMEM_STATIC_ASSERT(std::is_same<
  decltype(hadesmem::Call<PtrRetFuncT>(std::declval<hadesmem::Process>(), 
  nullptr, hadesmem::CallConv::kDefault).first), 
  DummyType*>::value);

typedef void (* VoidRetFuncT)();
HADESMEM_STATIC_ASSERT(std::is_same<
  decltype(hadesmem::Call<VoidRetFuncT>(std::declval<hadesmem::Process>(), 
  nullptr, hadesmem::CallConv::kDefault).first), 
  int>::value);

}

DWORD_PTR TestInteger(int a, int b, int c, int d, int e, int f)
{
  BOOST_CHECK_EQUAL(a, static_cast<int>(0xAAAAAAAA));
  BOOST_CHECK_EQUAL(b, static_cast<int>(0xBBBBBBBB));
  BOOST_CHECK_EQUAL(c, static_cast<int>(0xCCCCCCCC));
  BOOST_CHECK_EQUAL(d, static_cast<int>(0xDDDDDDDD));
  BOOST_CHECK_EQUAL(e, static_cast<int>(0xEEEEEEEE));
  BOOST_CHECK_EQUAL(f, static_cast<int>(0xFFFFFFFF));
  
  SetLastError(0x87654321);
  
  return 0x12345678;
}

float TestFloat(float a, float b, float c, float d, float e, float f)
{
  BOOST_CHECK_EQUAL(a, 1.11111f);
  BOOST_CHECK_EQUAL(b, 2.22222f);
  BOOST_CHECK_EQUAL(c, 3.33333f);
  BOOST_CHECK_EQUAL(d, 4.44444f);
  BOOST_CHECK_EQUAL(e, 5.55555f);
  BOOST_CHECK_EQUAL(f, 6.66666f);
  
  return 1.23456f;
}

double TestDouble(double a, double b, double c, double d, double e, double f)
{
  BOOST_CHECK_EQUAL(a, 1.11111);
  BOOST_CHECK_EQUAL(b, 2.22222);
  BOOST_CHECK_EQUAL(c, 3.33333);
  BOOST_CHECK_EQUAL(d, 4.44444);
  BOOST_CHECK_EQUAL(e, 5.55555);
  BOOST_CHECK_EQUAL(f, 6.66666);
  
  return 1.23456;
}

DWORD_PTR TestMixed(double a, void const* b, char c, float d, int e, 
  unsigned int f, float g, double h, DummyType const* i, int j, int k, 
  DWORD64 l)
{
  BOOST_CHECK_EQUAL(a, 1337.6666);
  BOOST_CHECK_EQUAL(b, static_cast<void const*>(nullptr));
  BOOST_CHECK_EQUAL(c, 'c');
  BOOST_CHECK_EQUAL(d, 9081.736455f);
  BOOST_CHECK_EQUAL(e, -1234);
  BOOST_CHECK_EQUAL(f, static_cast<unsigned int>(0xDEAFBEEF));
  BOOST_CHECK_EQUAL(g, 1234.56f);
  BOOST_CHECK_EQUAL(h, 9876.54);
  BOOST_CHECK_EQUAL(i, &dummy_glob);
  BOOST_CHECK_EQUAL(j, 1234);
  BOOST_CHECK_EQUAL(k, 5678);
  BOOST_CHECK_EQUAL(l, 0xAAAAAAAABBBBBBBBULL);
  
  SetLastError(5678);
  return 1234;
}

int TestInteger64(DWORD64 a)
{
  BOOST_CHECK_EQUAL(a, 0xAAAAAAAABBBBBBBBULL);
  
  return 0;
}

DWORD64 TestCall64Ret()
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
{ }

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
  DWORD_PTR TestIntegerThis(int a, int b, int c, int d, int e, int f) const
  {
    BOOST_CHECK_EQUAL(a, static_cast<int>(0xAAAAAAAA));
    BOOST_CHECK_EQUAL(b, static_cast<int>(0xBBBBBBBB));
    BOOST_CHECK_EQUAL(c, static_cast<int>(0xCCCCCCCC));
    BOOST_CHECK_EQUAL(d, static_cast<int>(0xDDDDDDDD));
    BOOST_CHECK_EQUAL(e, static_cast<int>(0xEEEEEEEE));
    BOOST_CHECK_EQUAL(f, static_cast<int>(0xFFFFFFFF));
    
    SetLastError(0x87654321);
    
    return 0x12345678;
  }
};

#if defined(_M_AMD64)

#elif defined(_M_IX86)

DWORD_PTR __fastcall TestIntegerFast(int a, int b, int c, int d, int e, int f)
{
  BOOST_CHECK_EQUAL(a, static_cast<int>(0xAAAAAAAA));
  BOOST_CHECK_EQUAL(b, static_cast<int>(0xBBBBBBBB));
  BOOST_CHECK_EQUAL(c, static_cast<int>(0xCCCCCCCC));
  BOOST_CHECK_EQUAL(d, static_cast<int>(0xDDDDDDDD));
  BOOST_CHECK_EQUAL(e, static_cast<int>(0xEEEEEEEE));
  BOOST_CHECK_EQUAL(f, static_cast<int>(0xFFFFFFFF));
  
  SetLastError(0x87654321);
  
  return 0x12345678;
}

DWORD_PTR __stdcall TestIntegerStd(int a, int b, int c, int d, int e, int f)
{
  BOOST_CHECK_EQUAL(a, static_cast<int>(0xAAAAAAAA));
  BOOST_CHECK_EQUAL(b, static_cast<int>(0xBBBBBBBB));
  BOOST_CHECK_EQUAL(c, static_cast<int>(0xCCCCCCCC));
  BOOST_CHECK_EQUAL(d, static_cast<int>(0xDDDDDDDD));
  BOOST_CHECK_EQUAL(e, static_cast<int>(0xEEEEEEEE));
  BOOST_CHECK_EQUAL(f, static_cast<int>(0xFFFFFFFF));
  
  SetLastError(0x87654321);
  
  return 0x12345678;
}

int __fastcall TestInteger64Fast(DWORD64 a)
{
  BOOST_CHECK_EQUAL(a, 0xAAAAAAAABBBBBBBBULL);
  
  return 0;
}

#else
#error "[HadesMem] Unsupported architecture."
#endif

BOOST_AUTO_TEST_CASE(call)
{
  hadesmem::Process const process(::GetCurrentProcessId());
  
  typedef DWORD_PTR (*TestIntegerT)(int a, int b, int c, int d, int e, int f);
  auto const call_int_ret = hadesmem::Call<TestIntegerT>(
    process, reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    &TestInteger)), hadesmem::CallConv::kDefault, 0xAAAAAAAA, 0xBBBBBBBB, 
    0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE, 0xFFFFFFFF);
  BOOST_CHECK_EQUAL(call_int_ret.first, static_cast<DWORD_PTR>(0x12345678));
  
  typedef float (*TestFloatT)(float a, float b, float c, float d, float e, 
    float f);
  auto const call_float_ret = hadesmem::Call<TestFloatT>(
    process, reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    &TestFloat)), hadesmem::CallConv::kDefault, 1.11111f, 2.22222f, 
    3.33333f, 4.44444f, 5.55555f, 6.66666f);
  BOOST_CHECK_EQUAL(call_float_ret.first, 1.23456f);
  
  typedef double (*TestDoubleT)(double a, double b, double c, double d, 
    double e, double f);
  auto const call_double_ret = 
    hadesmem::Call<TestDoubleT>(process, reinterpret_cast<PVOID>(
    reinterpret_cast<DWORD_PTR>(&TestDouble)), hadesmem::CallConv::kDefault, 
    1.11111, 2.22222, 3.33333, 4.44444, 5.55555, 6.66666);
  BOOST_CHECK_EQUAL(call_double_ret.first, 1.23456);
  
  struct ImplicitConvTest
  {
    operator int() const
    {
      return -1234;
    }
  };
  
  typedef DWORD_PTR (*TestFuncT)(double a, void const* b, char c, 
    float d, int e, unsigned int f, float g, double h, DummyType const* i, 
    int j, int k, DWORD64 l);
  auto const call_ret = hadesmem::Call<TestFuncT>(
    process, reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(&TestMixed)), 
    hadesmem::CallConv::kDefault, 1337.6666, nullptr, 'c', 9081.736455f, 
    ImplicitConvTest(), 0xDEAFBEEF, 1234.56f, 9876.54, &dummy_glob, 1234, 
    5678, 0xAAAAAAAABBBBBBBBULL);
  BOOST_CHECK_EQUAL(call_ret.first, static_cast<DWORD_PTR>(1234));
  BOOST_CHECK_EQUAL(call_ret.second, static_cast<DWORD>(5678));
  
  hadesmem::Call<int (*)(DWORD64 a)>(process, reinterpret_cast<PVOID>(
    reinterpret_cast<DWORD_PTR>(&TestInteger64)), 
    hadesmem::CallConv::kDefault, 0xAAAAAAAABBBBBBBBULL);

  // Clang does not yet implement MSVC-style __thiscall
#if !defined(HADESMEM_CLANG)

#if defined(_M_AMD64)
  hadesmem::CallConv const thiscall_call_conv = hadesmem::CallConv::kDefault;
#elif defined(_M_IX86)
  hadesmem::CallConv const thiscall_call_conv = hadesmem::CallConv::kThisCall;
#else
#error "[HadesMem] Unsupported architecture."
#endif
  hadesmem::detail::UnionCast<decltype(&ThiscallDummy::TestIntegerThis), 
    PVOID> mem_fn_to_pvoid(&ThiscallDummy::TestIntegerThis);
  PVOID test_integer_this = mem_fn_to_pvoid.GetTo();
  ThiscallDummy thiscall_dummy;
  typedef DWORD_PTR (*TestIntegerThisT)(ThiscallDummy* instance, int a, int b, 
    int c, int d, int e, int f);
  auto const call_int_this_ret = hadesmem::Call<TestIntegerThisT>(
    process, test_integer_this, thiscall_call_conv, &thiscall_dummy, 
    0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE, 0xFFFFFFFF);
  BOOST_CHECK_EQUAL(call_int_this_ret.first, 
    static_cast<DWORD_PTR>(0x12345678));
  BOOST_CHECK_EQUAL(call_int_this_ret.second, static_cast<DWORD>(0x87654321));

#endif // #if !defined(HADESMEM_CLANG)

#if defined(_M_AMD64)
  
#elif defined(_M_IX86)

  auto const call_int_fast_ret = hadesmem::Call<TestIntegerT>(
    process, reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    &TestIntegerFast)), hadesmem::CallConv::kFastCall, 0xAAAAAAAA, 0xBBBBBBBB, 
    0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE, 0xFFFFFFFF);
  BOOST_CHECK_EQUAL(call_int_fast_ret.first, 
    static_cast<DWORD_PTR>(0x12345678));
  BOOST_CHECK_EQUAL(call_int_fast_ret.second, static_cast<DWORD>(0x87654321));

  auto const call_int_std_ret = hadesmem::Call<TestIntegerT>(
    process, reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    &TestIntegerStd)), hadesmem::CallConv::kStdCall, 0xAAAAAAAA, 0xBBBBBBBB, 
    0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE, 0xFFFFFFFF);
  BOOST_CHECK_EQUAL(call_int_std_ret.first, 
    static_cast<DWORD_PTR>(0x12345678));
  BOOST_CHECK_EQUAL(call_int_std_ret.second, static_cast<DWORD>(0x87654321));

  // Clang does not yet implement MSVC-style __fastcall (it seems to do so 
  // for the 'regular' case, but will do things differently when faced 
  // with a 64-bit integer as the first parameter).
#if !defined(HADESMEM_CLANG)

  hadesmem::Call<int (*)(DWORD64 a)>(process, reinterpret_cast<PVOID>(
    reinterpret_cast<DWORD_PTR>(&TestInteger64Fast)), 
    hadesmem::CallConv::kFastCall, 0xAAAAAAAABBBBBBBBULL);

#endif // #if !defined(HADESMEM_CLANG)

#else
#error "[HadesMem] Unsupported architecture."
#endif
  
  auto const call_ret_64 = hadesmem::Call<DWORD64 (*)()>(
    process, reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    &TestCall64Ret)), hadesmem::CallConv::kDefault);
  BOOST_CHECK_EQUAL(call_ret_64.first, 0x123456787654321ULL);
  
  auto const call_ret_float = hadesmem::Call<float (*)()>(
    process, reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    &TestCallFloatRet)), hadesmem::CallConv::kDefault);
  BOOST_CHECK_EQUAL(call_ret_float.first, 1.234f);
  
  auto const call_ret_double = 
    hadesmem::Call<double (*)()>(process, reinterpret_cast<PVOID>(
    reinterpret_cast<DWORD_PTR>(&TestCallDoubleRet)), 
    hadesmem::CallConv::kDefault);
  BOOST_CHECK_EQUAL(call_ret_double.first, 9.876);

  auto const call_ret_void = 
    hadesmem::Call<void (*)()>(process, reinterpret_cast<PVOID>(
    reinterpret_cast<DWORD_PTR>(&TestCallVoidRet)), 
    hadesmem::CallConv::kDefault);
  BOOST_CHECK_EQUAL(call_ret_void.second, 0U);
  
  HMODULE const kernel32_mod = ::GetModuleHandle(L"kernel32.dll");
  BOOST_REQUIRE(kernel32_mod != 0);
  
#if defined(HADESMEM_MSVC)
#pragma warning(push)
#pragma warning(disable: 6387)
#endif // #if defined(HADESMEM_MSVC)
  
  FARPROC const get_proc_address_tmp = ::GetProcAddress(kernel32_mod, 
    "GetProcAddress");
  PVOID const get_proc_address = reinterpret_cast<PVOID>(
    reinterpret_cast<DWORD_PTR>(get_proc_address_tmp));
  BOOST_REQUIRE(get_proc_address != 0);
  
#if defined(HADESMEM_MSVC)
#pragma warning(pop)
#endif // #if defined(HADESMEM_MSVC)
  
  auto const call_win = 
    hadesmem::Call<PVOID (*)(HMODULE, LPCSTR)>(process, get_proc_address, 
    hadesmem::CallConv::kWinApi, kernel32_mod, "GetProcAddress");
  BOOST_CHECK_EQUAL(call_win.first, get_proc_address);
  
  hadesmem::MultiCall multi_call(&process);
  multi_call.Add<void (*)(DWORD last_error)>(process, reinterpret_cast<PVOID>(
    reinterpret_cast<DWORD_PTR>(&MultiThreadSet)), 
    hadesmem::CallConv::kDefault, 0x1337);
  multi_call.Add<DWORD (*)()>(process, reinterpret_cast<PVOID>(
    reinterpret_cast<DWORD_PTR>(&MultiThreadGet)), 
    hadesmem::CallConv::kDefault);
  multi_call.Add<void (*)(DWORD last_error)>(process, reinterpret_cast<PVOID>(
    reinterpret_cast<DWORD_PTR>(&MultiThreadSet)), 
    hadesmem::CallConv::kDefault, 0x1234);
  multi_call.Add<DWORD (*)()>(process, reinterpret_cast<PVOID>(
    reinterpret_cast<DWORD_PTR>(&MultiThreadGet)), 
    hadesmem::CallConv::kDefault);
  std::vector<hadesmem::CallResult> multi_call_ret = multi_call.Call();
  BOOST_CHECK_EQUAL(multi_call_ret[0].GetLastError(), 
    static_cast<DWORD>(0x1337));
  BOOST_CHECK_EQUAL(multi_call_ret[1].GetReturnValueIntPtr(), 
    static_cast<DWORD_PTR>(0x1337));
  BOOST_CHECK_EQUAL(multi_call_ret[2].GetLastError(), 
    static_cast<DWORD>(0x1234));
  BOOST_CHECK_EQUAL(multi_call_ret[3].GetReturnValueIntPtr(), 
    static_cast<DWORD_PTR>(0x1234));
}
