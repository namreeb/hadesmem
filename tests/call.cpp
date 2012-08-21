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

// Boost.Test causes the following warning under GCC:
// error: base class 'struct boost::unit_test::ut_detail::nil_t' has a 
// non-virtual destructor [-Werror=effc++]
#if defined(HADESMEM_GCC)
#pragma GCC diagnostic ignored "-Weffc++"
#endif // #if defined(HADESMEM_GCC)

// TODO: Test argument combinations more thorougly.
// TODO: Test multi-call.
// TODO: Test all possible Call overloads.
// TODO: Test that bad call-conv/address/etc fails as expected.
// TODO: Fix naming/style/etc.

namespace
{

struct DummyType { };
DummyType dummy_glob;

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

#if defined(_M_AMD64)

#elif defined(_M_IX86)

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
  auto const CallIntRet = hadesmem::Call<TestIntegerT>(
    process, reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    &TestInteger)), hadesmem::CallConv::kDefault, 0xAAAAAAAA, 0xBBBBBBBB, 
    0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE, 0xFFFFFFFF);
  BOOST_CHECK_EQUAL(CallIntRet.first, static_cast<DWORD_PTR>(0x12345678));
  
  typedef float (*TestFloatT)(float a, float b, float c, float d, float e, 
    float f);
  auto const CallFloatRet = hadesmem::Call<TestFloatT>(
    process, reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    &TestFloat)), hadesmem::CallConv::kDefault, 1.11111f, 2.22222f, 
    3.33333f, 4.44444f, 5.55555f, 6.66666f);
  BOOST_CHECK_EQUAL(CallFloatRet.first, 1.23456f);
  
  typedef double (*TestDoubleT)(double a, double b, double c, double d, 
    double e, double f);
  auto const CallDoubleRet = 
    hadesmem::Call<TestDoubleT>(process, reinterpret_cast<PVOID>(
    reinterpret_cast<DWORD_PTR>(&TestDouble)), hadesmem::CallConv::kDefault, 
    1.11111, 2.22222, 3.33333, 4.44444, 5.55555, 6.66666);
  BOOST_CHECK_EQUAL(CallDoubleRet.first, 1.23456);
  
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
  auto const CallRet = hadesmem::Call<TestFuncT>(
    process, reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(&TestMixed)), 
    hadesmem::CallConv::kDefault, 1337.6666, nullptr, 'c', 9081.736455f, 
    ImplicitConvTest(), 0xDEAFBEEF, 1234.56f, 9876.54, &dummy_glob, 1234, 
    5678, 0xAAAAAAAABBBBBBBBULL);
  BOOST_CHECK_EQUAL(CallRet.first, static_cast<DWORD_PTR>(1234));
  BOOST_CHECK_EQUAL(CallRet.second, static_cast<DWORD>(5678));
  
  hadesmem::Call<int (*)(DWORD64 a)>(process, reinterpret_cast<PVOID>(
    reinterpret_cast<DWORD_PTR>(&TestInteger64)), 
    hadesmem::CallConv::kDefault, 0xAAAAAAAABBBBBBBBULL);
  
#if defined(_M_AMD64)
  
#elif defined(_M_IX86)
  
  auto const CallIntFastRet = hadesmem::Call<TestIntegerT>(
    process, reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    &TestIntegerFast)), hadesmem::CallConv::kFastCall, 0xAAAAAAAA, 0xBBBBBBBB, 
    0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE, 0xFFFFFFFF);
  BOOST_CHECK_EQUAL(CallIntFastRet.first, static_cast<DWORD_PTR>(0x12345678));
  BOOST_CHECK_EQUAL(CallIntFastRet.second, static_cast<DWORD>(0x87654321));
  
  auto const CallIntStdRet = hadesmem::Call<TestIntegerT>(
    process, reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    &TestIntegerStd)), hadesmem::CallConv::kStdCall, 0xAAAAAAAA, 0xBBBBBBBB, 
    0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE, 0xFFFFFFFF);
  BOOST_CHECK_EQUAL(CallIntStdRet.first, static_cast<DWORD_PTR>(0x12345678));
  BOOST_CHECK_EQUAL(CallIntStdRet.second, static_cast<DWORD>(0x87654321));
  
  auto test_integer_this_temp = &ThiscallDummy::TestIntegerThis;
  typedef decltype(test_integer_this_temp) TestIntegerThisFnT;
  union Conv
  {
    TestIntegerThisFnT m;
    PVOID i;
  };
  Conv conv;
  conv.m = test_integer_this_temp;
  PVOID test_integer_this = conv.i;
  ThiscallDummy thiscall_dummy;
  typedef DWORD_PTR (*TestIntegerThisT)(ThiscallDummy* instance, int a, int b, 
    int c, int d, int e, int f);
  auto const CallIntThisRet = hadesmem::Call<TestIntegerThisT>(
    process, test_integer_this, hadesmem::CallConv::kThisCall, &thiscall_dummy, 
      0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE, 0xFFFFFFFF);
  BOOST_CHECK_EQUAL(CallIntThisRet.first, static_cast<DWORD_PTR>(0x12345678));
  BOOST_CHECK_EQUAL(CallIntThisRet.second, static_cast<DWORD>(0x87654321));
  
  hadesmem::Call<int (*)(DWORD64 a)>(process, reinterpret_cast<PVOID>(
    reinterpret_cast<DWORD_PTR>(&TestInteger64Fast)), 
    hadesmem::CallConv::kFastCall, 0xAAAAAAAABBBBBBBBULL);
  
#else
#error "[HadesMem] Unsupported architecture."
#endif
  
  auto const CallRet64 = hadesmem::Call<DWORD64 (*)()>(
    process, reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    &TestCall64Ret)), hadesmem::CallConv::kDefault);
  BOOST_CHECK_EQUAL(CallRet64.first, 0x123456787654321ULL);
  
  auto const CallRetFloat = hadesmem::Call<float (*)()>(
    process, reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    &TestCallFloatRet)), hadesmem::CallConv::kDefault);
  BOOST_CHECK_EQUAL(CallRetFloat.first, 1.234f);
  
  auto const CallRetDouble = 
    hadesmem::Call<double (*)()>(process, reinterpret_cast<PVOID>(
    reinterpret_cast<DWORD_PTR>(&TestCallDoubleRet)), 
    hadesmem::CallConv::kDefault);
  BOOST_CHECK_EQUAL(CallRetDouble.first, 9.876);

  auto const CallRetVoid = 
    hadesmem::Call<void (*)()>(process, reinterpret_cast<PVOID>(
    reinterpret_cast<DWORD_PTR>(&TestCallVoidRet)), 
    hadesmem::CallConv::kDefault);
  BOOST_CHECK_EQUAL(CallRetVoid.second, 0);

#if defined(_M_AMD64)
  auto const CallConvWinapi = hadesmem::CallConv::kDefault;
#elif defined(_M_IX86)
  auto const CallConvWinapi = hadesmem::CallConv::kStdCall;
#else
#error "[HadesMem] Unsupported architecture."
#endif
  
  HMODULE const kernel32_mod = GetModuleHandle(L"kernel32.dll");
  BOOST_REQUIRE(kernel32_mod != 0);
  
#if defined(HADESMEM_MSVC)
#pragma warning(push)
#pragma warning(disable: 6387)
#endif // #if defined(HADESMEM_MSVC)
  
  FARPROC const get_proc_address_tmp = GetProcAddress(kernel32_mod, 
    "GetProcAddress");
  PVOID const get_proc_address = reinterpret_cast<PVOID>(
    reinterpret_cast<DWORD_PTR>(get_proc_address_tmp));
  BOOST_REQUIRE(get_proc_address != 0);
  
#if defined(HADESMEM_MSVC)
#pragma warning(pop)
#endif // #if defined(HADESMEM_MSVC)
  
  auto const CallWin = 
    hadesmem::Call<PVOID (*)(HMODULE, LPCSTR)>(process, get_proc_address, 
    CallConvWinapi, kernel32_mod, "GetProcAddress");
  BOOST_CHECK_EQUAL(CallWin.first, get_proc_address);
  
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
  std::vector<hadesmem::RemoteFunctionRet> multi_call_ret = multi_call.Call();
  BOOST_CHECK_EQUAL(multi_call_ret[0].GetLastError(), 
  static_cast<DWORD>(0x1337));
  BOOST_CHECK_EQUAL(multi_call_ret[1].GetReturnValue(), 
  static_cast<DWORD_PTR>(0x1337));
  BOOST_CHECK_EQUAL(multi_call_ret[2].GetLastError(), 
  static_cast<DWORD>(0x1234));
  BOOST_CHECK_EQUAL(multi_call_ret[3].GetReturnValue(), 
  static_cast<DWORD_PTR>(0x1234));
}
