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

namespace
{

struct DummyType { };
DummyType dummy_glob;

}

DWORD_PTR TestCall(double a, DummyType const* b, char c, 
  float d, int e, unsigned int f)
{
  BOOST_CHECK_EQUAL(a, 1337.6666);
  BOOST_CHECK_EQUAL(b, &dummy_glob);
  BOOST_CHECK_EQUAL(c, 'c');
  BOOST_CHECK_EQUAL(d, 9081.736455f);
  BOOST_CHECK_EQUAL(e, -1234);
  BOOST_CHECK_EQUAL(f, static_cast<unsigned int>(0xDEAFBEEF));
  
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
DWORD_PTR __fastcall TestFastCall(void const* a, DummyType const* b, char c, 
  wchar_t d, int e, unsigned int f)
{
  BOOST_CHECK_EQUAL(a, static_cast<PVOID>(nullptr));
  BOOST_CHECK_EQUAL(b, &dummy_glob);
  BOOST_CHECK_EQUAL(c, 'c');
  BOOST_CHECK_EQUAL(d, L'd');
  BOOST_CHECK_EQUAL(e, -1234);
  BOOST_CHECK_EQUAL(f, static_cast<unsigned int>(0xDEAFBEEF));
  
  SetLastError(5678);
  return 1234;
}

DWORD_PTR __stdcall TestStdCall(void const* a, DummyType const* b, char c, 
  wchar_t d, int e, unsigned int f)
{
  BOOST_CHECK_EQUAL(a, static_cast<PVOID>(nullptr));
  BOOST_CHECK_EQUAL(b, &dummy_glob);
  BOOST_CHECK_EQUAL(c, 'c');
  BOOST_CHECK_EQUAL(d, L'd');
  BOOST_CHECK_EQUAL(e, -1234);
  BOOST_CHECK_EQUAL(f, static_cast<unsigned int>(0xDEAFBEEF));
  
  SetLastError(5678);
  return 1234;
}
#else 
#error "[HadesMem] Unsupported architecture."
#endif

BOOST_AUTO_TEST_CASE(call)
{
  hadesmem::Process const process(::GetCurrentProcessId());
  
  struct ImplicitConvTest
  {
    operator int() const
    {
      return -1234;
    }
  };
  
  typedef DWORD_PTR (*TestFuncT)(double a, DummyType const* b, char c, 
  float d, int e, unsigned int f);
  hadesmem::RemoteFunctionRet const CallRet = hadesmem::Call<TestFuncT>(
    process, reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(&TestCall)), 
    hadesmem::CallConv::kDefault, 1337.6666, &dummy_glob, 'c', 9081.736455f, 
    ImplicitConvTest(), 0xDEAFBEEF);
  BOOST_CHECK_EQUAL(CallRet.GetReturnValue(), static_cast<DWORD_PTR>(1234));
  BOOST_CHECK_EQUAL(CallRet.GetLastError(), static_cast<DWORD>(5678));
  
#if defined(_M_AMD64) 
#elif defined(_M_IX86) 
  hadesmem::RemoteFunctionRet const CallRetFast = 
    hadesmem::Call<TestFuncT>(process, reinterpret_cast<PVOID>(
    reinterpret_cast<DWORD_PTR>(&TestFastCall)), 
    hadesmem::CallConv::kFastCall, nullptr, &dummy_glob, 'c', L'd', 
    ImplicitConvTest(), 0xDEAFBEEF);
  BOOST_CHECK_EQUAL(CallRetFast.GetReturnValue(), static_cast<DWORD_PTR>(
    1234));
  BOOST_CHECK_EQUAL(CallRetFast.GetLastError(), static_cast<DWORD>(5678));
  
  hadesmem::RemoteFunctionRet const CallRetStd = 
    hadesmem::Call<TestFuncT>(process, reinterpret_cast<PVOID>(
    reinterpret_cast<DWORD_PTR>(&TestStdCall)), hadesmem::CallConv::kStdCall, 
    nullptr, &dummy_glob, 'c', L'd', ImplicitConvTest(), 0xDEAFBEEF);
  BOOST_CHECK_EQUAL(CallRetStd.GetReturnValue(), static_cast<DWORD_PTR>(
    1234));
  BOOST_CHECK_EQUAL(CallRetStd.GetLastError(), static_cast<DWORD>(5678));
#else 
#error "[HadesMem] Unsupported architecture."
#endif
  
  hadesmem::RemoteFunctionRet const CallRet64 = hadesmem::Call<DWORD64 (*)()>(
    process, reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    &TestCall64Ret)), hadesmem::CallConv::kDefault);
  BOOST_CHECK_EQUAL(CallRet64.GetReturnValue64(), static_cast<DWORD64>(
    0x123456787654321LL));
  
  hadesmem::RemoteFunctionRet const CallRetFloat = hadesmem::Call<float (*)()>(
    process, reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    &TestCallFloatRet)), hadesmem::CallConv::kDefault);
  BOOST_CHECK_EQUAL(CallRetFloat.GetReturnValueFloat(), 1.234f);
  
  hadesmem::RemoteFunctionRet const CallRetDouble = 
    hadesmem::Call<double (*)()>(process, reinterpret_cast<PVOID>(
    reinterpret_cast<DWORD_PTR>(&TestCallDoubleRet)), 
    hadesmem::CallConv::kDefault);
  BOOST_CHECK_EQUAL(CallRetDouble.GetReturnValueDouble(), 9.876);
  BOOST_CHECK_EQUAL(CallRetDouble.GetReturnValueLongDouble(), 9.876);
}
