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
  unsigned int f, float g, double h, DummyType const* i, int j, int k)
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
  
  SetLastError(5678);
  return 1234;
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

#if defined(_M_AMD64) 
#elif defined(_M_IX86) 
DWORD_PTR __fastcall TestFastCall(double a, DummyType const* b, char c, 
  float d, int e, unsigned int f, float g, double h, void const* i)
{
  BOOST_CHECK_EQUAL(a, 1337.6666);
  BOOST_CHECK_EQUAL(b, &dummy_glob);
  BOOST_CHECK_EQUAL(c, 'c');
  BOOST_CHECK_EQUAL(d, 9081.736455f);
  BOOST_CHECK_EQUAL(e, -1234);
  BOOST_CHECK_EQUAL(f, static_cast<unsigned int>(0xDEAFBEEF));
  BOOST_CHECK_EQUAL(g, 1234.56f);
  BOOST_CHECK_EQUAL(h, 9876.54);
  BOOST_CHECK_EQUAL(i, static_cast<void const*>(nullptr));
  
  SetLastError(5678);
  return 1234;
}

DWORD_PTR __stdcall TestStdCall(double a, DummyType const* b, char c, 
  float d, int e, unsigned int f, float g, double h, void const* i)
{
  BOOST_CHECK_EQUAL(a, 1337.6666);
  BOOST_CHECK_EQUAL(b, &dummy_glob);
  BOOST_CHECK_EQUAL(c, 'c');
  BOOST_CHECK_EQUAL(d, 9081.736455f);
  BOOST_CHECK_EQUAL(e, -1234);
  BOOST_CHECK_EQUAL(f, static_cast<unsigned int>(0xDEAFBEEF));
  BOOST_CHECK_EQUAL(g, 1234.56f);
  BOOST_CHECK_EQUAL(h, 9876.54);
  BOOST_CHECK_EQUAL(i, static_cast<void const*>(nullptr));
  
  SetLastError(5678);
  return 1234;
}
#else 
#error "[HadesMem] Unsupported architecture."
#endif

BOOST_AUTO_TEST_CASE(call)
{
  hadesmem::Process const process(::GetCurrentProcessId());
  
  typedef DWORD_PTR (*TestIntegerT)(int a, int b, int c, int d, int e, int f);
  hadesmem::RemoteFunctionRet const CallIntRet = hadesmem::Call<TestIntegerT>(
    process, reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    &TestInteger)), hadesmem::CallConv::kDefault, 0xAAAAAAAA, 0xBBBBBBBB, 
    0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE, 0xFFFFFFFF);
  BOOST_CHECK_EQUAL(CallIntRet.GetReturnValue<DWORD_PTR>(), 
    static_cast<DWORD_PTR>(0x12345678));
  BOOST_CHECK_EQUAL(CallIntRet.GetReturnValue(), static_cast<DWORD_PTR>(
    0x12345678));
  
  typedef float (*TestFloatT)(float a, float b, float c, float d, float e, 
    float f);
  hadesmem::RemoteFunctionRet const CallFloatRet = hadesmem::Call<TestFloatT>(
    process, reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    &TestFloat)), hadesmem::CallConv::kDefault, 1.11111f, 2.22222f, 
    3.33333f, 4.44444f, 5.55555f, 6.66666f);
  BOOST_CHECK_EQUAL(CallFloatRet.GetReturnValue<float>(), 1.23456f);
  BOOST_CHECK_EQUAL(CallFloatRet.GetReturnValueFloat(), 1.23456f);
  
  typedef double (*TestDoubleT)(double a, double b, double c, double d, 
    double e, double f);
  hadesmem::RemoteFunctionRet const CallDoubleRet = 
    hadesmem::Call<TestDoubleT>(process, reinterpret_cast<PVOID>(
    reinterpret_cast<DWORD_PTR>(&TestDouble)), hadesmem::CallConv::kDefault, 
    1.11111, 2.22222, 3.33333, 4.44444, 5.55555, 6.66666);
  BOOST_CHECK_EQUAL(CallDoubleRet.GetReturnValue<double>(), 1.23456);
  BOOST_CHECK_EQUAL(CallDoubleRet.GetReturnValueDouble(), 1.23456);
  
  struct ImplicitConvTest
  {
    operator int() const
    {
      return -1234;
    }
  };
  
  typedef DWORD_PTR (*TestFuncT)(double a, void const* b, char c, 
    float d, int e, unsigned int f, float g, double h, DummyType const* i, 
    int j, int k);
  hadesmem::RemoteFunctionRet const CallRet = hadesmem::Call<TestFuncT>(
    process, reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(&TestMixed)), 
    hadesmem::CallConv::kDefault, 1337.6666, nullptr, 'c', 9081.736455f, 
    ImplicitConvTest(), 0xDEAFBEEF, 1234.56f, 9876.54, &dummy_glob, 1234, 5678);
  BOOST_CHECK_EQUAL(CallRet.GetReturnValue<DWORD_PTR>(), 
    static_cast<DWORD_PTR>(1234));
  BOOST_CHECK_EQUAL(CallRet.GetReturnValue(), static_cast<DWORD_PTR>(1234));
  BOOST_CHECK_EQUAL(CallRet.GetLastError(), static_cast<DWORD>(5678));
  
#if defined(_M_AMD64) 
#elif defined(_M_IX86) 
  hadesmem::RemoteFunctionRet const CallRetFast = 
    hadesmem::Call<TestFuncT>(process, reinterpret_cast<PVOID>(
    reinterpret_cast<DWORD_PTR>(&TestFastCall)), 
    hadesmem::CallConv::kFastCall, nullptr, &dummy_glob, 'c', L'd', 
    ImplicitConvTest(), 0xDEAFBEEF, 1234.56f, 9876.54);
  BOOST_CHECK_EQUAL(CallRetFast.GetReturnValue(), static_cast<DWORD_PTR>(
    1234));
  BOOST_CHECK_EQUAL(CallRetFast.GetLastError(), static_cast<DWORD>(5678));
  
  hadesmem::RemoteFunctionRet const CallRetStd = 
    hadesmem::Call<TestFuncT>(process, reinterpret_cast<PVOID>(
    reinterpret_cast<DWORD_PTR>(&TestStdCall)), hadesmem::CallConv::kStdCall, 
    nullptr, &dummy_glob, 'c', L'd', ImplicitConvTest(), 0xDEAFBEEF, 
    1234.56f, 9876.54);
  BOOST_CHECK_EQUAL(CallRetStd.GetReturnValue(), static_cast<DWORD_PTR>(
    1234));
  BOOST_CHECK_EQUAL(CallRetStd.GetLastError(), static_cast<DWORD>(5678));
#else 
#error "[HadesMem] Unsupported architecture."
#endif
  
  hadesmem::RemoteFunctionRet const CallRet64 = hadesmem::Call<DWORD64 (*)()>(
    process, reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    &TestCall64Ret)), hadesmem::CallConv::kDefault);
  BOOST_CHECK_EQUAL(CallRet64.GetReturnValue<DWORD64>(), 0x123456787654321ULL);
  BOOST_CHECK_EQUAL(CallRet64.GetReturnValue64(), 0x123456787654321ULL);
  
  hadesmem::RemoteFunctionRet const CallRetFloat = hadesmem::Call<float (*)()>(
    process, reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    &TestCallFloatRet)), hadesmem::CallConv::kDefault);
  BOOST_CHECK_EQUAL(CallRetFloat.GetReturnValueFloat(), 1.234f);
  
  hadesmem::RemoteFunctionRet const CallRetDouble = 
    hadesmem::Call<double (*)()>(process, reinterpret_cast<PVOID>(
    reinterpret_cast<DWORD_PTR>(&TestCallDoubleRet)), 
    hadesmem::CallConv::kDefault);
  BOOST_CHECK_EQUAL(CallRetDouble.GetReturnValue<double>(), 9.876);
  BOOST_CHECK_EQUAL(CallRetDouble.GetReturnValueDouble(), 9.876);
}
