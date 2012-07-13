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

DWORD_PTR TestCall(PVOID const a, PVOID const b, PVOID const c, PVOID const d, 
  PVOID const e, PVOID const f)
{
  BOOST_CHECK_EQUAL(a, static_cast<PVOID>(nullptr));
  BOOST_CHECK_EQUAL(b, reinterpret_cast<PVOID>(-1));
  BOOST_CHECK_EQUAL(c, reinterpret_cast<PVOID>(0x11223344));
  BOOST_CHECK_EQUAL(d, reinterpret_cast<PVOID>(0xAABBCCDD));
  BOOST_CHECK_EQUAL(e, reinterpret_cast<PVOID>(0x55667788));
  BOOST_CHECK_EQUAL(f, reinterpret_cast<PVOID>(0x99999999));
  
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
  BOOST_CHECK_EQUAL(c, reinterpret_cast<PVOID>(0x11223344));
  BOOST_CHECK_EQUAL(d, reinterpret_cast<PVOID>(0xAABBCCDD));
  BOOST_CHECK_EQUAL(e, reinterpret_cast<PVOID>(0x55667788));
  BOOST_CHECK_EQUAL(f, reinterpret_cast<PVOID>(0x99999999));
  
  SetLastError(5678);
  return 1234;
}

DWORD_PTR __stdcall TestStdCall(PVOID const a, PVOID const b, PVOID const c, 
  PVOID const d, PVOID const e, PVOID const f)
{
  BOOST_CHECK_EQUAL(a, static_cast<PVOID>(nullptr));
  BOOST_CHECK_EQUAL(b, reinterpret_cast<PVOID>(-1));
  BOOST_CHECK_EQUAL(c, reinterpret_cast<PVOID>(0x11223344));
  BOOST_CHECK_EQUAL(d, reinterpret_cast<PVOID>(0xAABBCCDD));
  BOOST_CHECK_EQUAL(e, reinterpret_cast<PVOID>(0x55667788));
  BOOST_CHECK_EQUAL(f, reinterpret_cast<PVOID>(0x99999999));
  
  SetLastError(5678);
  return 1234;
}
#else 
#error "[HadesMem] Unsupported architecture."
#endif

BOOST_AUTO_TEST_CASE(call)
{
  hadesmem::Process const process(::GetCurrentProcessId());

  std::vector<PVOID> TestCallArgs;
  TestCallArgs.push_back(nullptr);
  TestCallArgs.push_back(reinterpret_cast<PVOID>(-1));
  TestCallArgs.push_back(reinterpret_cast<PVOID>(0x11223344));
  TestCallArgs.push_back(reinterpret_cast<PVOID>(0xAABBCCDD));
  TestCallArgs.push_back(reinterpret_cast<PVOID>(0x55667788));
  TestCallArgs.push_back(reinterpret_cast<PVOID>(0x99999999));
  hadesmem::RemoteFunctionRet const CallRet = Call(process, 
    reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(&TestCall)), 
    hadesmem::CallConv::kDefault, TestCallArgs);
  BOOST_CHECK_EQUAL(CallRet.GetReturnValue(), static_cast<DWORD_PTR>(1234));
  BOOST_CHECK_EQUAL(CallRet.GetLastError(), static_cast<DWORD>(5678));
  
#if defined(_M_AMD64) 
#elif defined(_M_IX86) 
  hadesmem::RemoteFunctionRet const CallRetFast = 
    Call(process, reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    &TestFastCall)), hadesmem::CallConv::kFastCall, 
    TestCallArgs);
  BOOST_CHECK_EQUAL(CallRetFast.GetReturnValue(), static_cast<DWORD_PTR>(
    1234));
  BOOST_CHECK_EQUAL(CallRetFast.GetLastError(), static_cast<DWORD>(5678));
  
  hadesmem::RemoteFunctionRet const CallRetStd = 
    Call(process, reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(
    &TestStdCall)), hadesmem::CallConv::kStdCall, TestCallArgs);
  BOOST_CHECK_EQUAL(CallRetStd.GetReturnValue(), static_cast<DWORD_PTR>(
    1234));
  BOOST_CHECK_EQUAL(CallRetStd.GetLastError(), static_cast<DWORD>(5678));
#else 
#error "[HadesMem] Unsupported architecture."
#endif
  
  std::vector<PVOID> TestCall64Args;
  hadesmem::RemoteFunctionRet const CallRet64 = Call(process, 
    reinterpret_cast<PVOID>(reinterpret_cast<DWORD_PTR>(&TestCall64Ret)), 
    hadesmem::CallConv::kDefault, TestCall64Args);
  BOOST_CHECK_EQUAL(CallRet64.GetReturnValue64(), static_cast<DWORD64>(
    0x123456787654321LL));
}
