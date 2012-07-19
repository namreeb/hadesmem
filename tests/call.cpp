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

DWORD_PTR TestCall(void const* a, void const* b, unsigned int c, unsigned int d, 
  unsigned int e, unsigned int f)
{
  BOOST_CHECK_EQUAL(a, static_cast<PVOID>(nullptr));
  BOOST_CHECK_EQUAL(b, reinterpret_cast<PVOID>(-1));
  BOOST_CHECK_EQUAL(c, static_cast<unsigned int>('c'));
  BOOST_CHECK_EQUAL(d, static_cast<unsigned int>(42));
  BOOST_CHECK_EQUAL(e, static_cast<unsigned int>(0xDEAFBEEF));
  BOOST_CHECK_EQUAL(f, static_cast<unsigned int>(0xBAADF00D));
  
  SetLastError(5678);
  return 1234;
}

DWORD64 TestCall64Ret()
{
  return 0x123456787654321LL;
}

#if defined(_M_AMD64) 
#elif defined(_M_IX86) 
DWORD_PTR __fastcall TestFastCall(PVOID const a, PVOID const b, PVOID const c, 
  PVOID const d, PVOID const e, PVOID const f)
{
  BOOST_CHECK_EQUAL(a, static_cast<PVOID>(nullptr));
  BOOST_CHECK_EQUAL(b, reinterpret_cast<PVOID>(-1));
  BOOST_CHECK_EQUAL(c, static_cast<unsigned int>('c'));
  BOOST_CHECK_EQUAL(d, static_cast<unsigned int>(42));
  BOOST_CHECK_EQUAL(e, static_cast<unsigned int>(0xDEAFBEEF));
  BOOST_CHECK_EQUAL(f, static_cast<unsigned int>(0xBAADF00D));
  
  SetLastError(5678);
  return 1234;
}

DWORD_PTR __stdcall TestStdCall(PVOID const a, PVOID const b, PVOID const c, 
  PVOID const d, PVOID const e, PVOID const f)
{
  BOOST_CHECK_EQUAL(a, static_cast<PVOID>(nullptr));
  BOOST_CHECK_EQUAL(b, reinterpret_cast<PVOID>(-1));
  BOOST_CHECK_EQUAL(c, static_cast<unsigned int>('c'));
  BOOST_CHECK_EQUAL(d, static_cast<unsigned int>(42));
  BOOST_CHECK_EQUAL(e, static_cast<unsigned int>(0xDEAFBEEF));
  BOOST_CHECK_EQUAL(f, static_cast<unsigned int>(0xBAADF00D));
  
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
      return 42;
    }
  };
  
  typedef DWORD_PTR (*TestFuncT)(void const*, void const*, unsigned int, 
    unsigned int, unsigned int, unsigned int);
  hadesmem::RemoteFunctionRet const CallRet = hadesmem::Call<TestFuncT>(
    process, reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(&TestCall)), 
    hadesmem::CallConv::kDefault, nullptr, reinterpret_cast<PVOID>(-1), 
    'c', ImplicitConvTest(), 0xDEAFBEEF, 0xBAADF00D);
  BOOST_CHECK_EQUAL(CallRet.GetReturnValue(), static_cast<DWORD_PTR>(1234));
  BOOST_CHECK_EQUAL(CallRet.GetLastError(), static_cast<DWORD>(5678));
  
#if defined(_M_AMD64) 
#elif defined(_M_IX86) 
  hadesmem::RemoteFunctionRet const CallRetFast = 
    hadesmem::Call<TestFuncT>(process, reinterpret_cast<PVOID>(
    reinterpret_cast<DWORD_PTR>(&TestFastCall)), 
    hadesmem::CallConv::kFastCall, nullptr, reinterpret_cast<PVOID>(-1), 
    'c', ImplicitConvTest(), 0xDEAFBEEF, 0xBAADF00D);
  BOOST_CHECK_EQUAL(CallRetFast.GetReturnValue(), static_cast<DWORD_PTR>(
    1234));
  BOOST_CHECK_EQUAL(CallRetFast.GetLastError(), static_cast<DWORD>(5678));
  
  hadesmem::RemoteFunctionRet const CallRetStd = 
    hadesmem::Call<TestFuncT>(process, reinterpret_cast<PVOID>(
    reinterpret_cast<DWORD_PTR>(&TestStdCall)), hadesmem::CallConv::kStdCall, 
    nullptr, reinterpret_cast<PVOID>(-1), 'c', ImplicitConvTest(), 0xDEAFBEEF, 
    0xBAADF00D);
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
}
