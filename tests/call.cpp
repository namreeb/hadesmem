// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/call.hpp>

#define BOOST_TEST_MODULE call
#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/test/unit_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/error.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/detail/initialize.hpp>
#include <hadesmem/detail/static_assert.hpp>

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

// TODO: Test argument combinations more thoroughly.
// TODO: Improve multi-call testing.
// TODO: Test all possible Call overloads.
// TODO: Compile-fail tests.

namespace
{

struct DummyType { };
DummyType dummy_glob;

typedef int (IntRetFuncT)();
HADESMEM_STATIC_ASSERT(std::is_same<
  decltype(hadesmem::Call<IntRetFuncT>(std::declval<hadesmem::Process>(), 
  nullptr, hadesmem::CallConv::kDefault).GetReturnValue()), 
  int>::value);

typedef double (DoubleRetFuncT)();
HADESMEM_STATIC_ASSERT(std::is_same<
  decltype(hadesmem::Call<DoubleRetFuncT>(std::declval<hadesmem::Process>(), 
  nullptr, hadesmem::CallConv::kDefault).GetReturnValue()), 
  double>::value);

typedef DummyType* (PtrRetFuncT)();
HADESMEM_STATIC_ASSERT(std::is_same<
  decltype(hadesmem::Call<PtrRetFuncT>(std::declval<hadesmem::Process>(), 
  nullptr, hadesmem::CallConv::kDefault).GetReturnValue()), 
  DummyType*>::value);

DWORD_PTR TestInteger(unsigned int a, unsigned int b, unsigned int c, 
  unsigned int d, unsigned int e, unsigned int f)
{
  BOOST_CHECK_EQUAL(a, 0xAAAAAAAAU);
  BOOST_CHECK_EQUAL(b, 0xBBBBBBBBU);
  BOOST_CHECK_EQUAL(c, 0xCCCCCCCCU);
  BOOST_CHECK_EQUAL(d, 0xDDDDDDDDU);
  BOOST_CHECK_EQUAL(e, 0xEEEEEEEEU);
  BOOST_CHECK_EQUAL(f, 0xFFFFFFFFU);
  
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
  unsigned int f, float g, double h, DummyType const* i, DWORD64 j)
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
  BOOST_CHECK_EQUAL(j, 0xAAAAAAAABBBBBBBBULL);
  
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

// Clang does not yet implement MSVC-style __thiscall
#if !defined(HADESMEM_CLANG)

class ThiscallDummy
{
public:
  DWORD_PTR TestIntegerThis(unsigned int a, unsigned int b, unsigned int c, 
    unsigned int d, unsigned int e) const
  {
    BOOST_CHECK_EQUAL(a, 0xAAAAAAAAU);
    BOOST_CHECK_EQUAL(b, 0xBBBBBBBBU);
    BOOST_CHECK_EQUAL(c, 0xCCCCCCCCU);
    BOOST_CHECK_EQUAL(d, 0xDDDDDDDDU);
    BOOST_CHECK_EQUAL(e, 0xEEEEEEEEU);

    SetLastError(0x87654321);

    return 0x12345678;
  }
};

#endif // #if !defined(HADESMEM_CLANG)

#if defined(HADESMEM_ARCH_X64)

#elif defined(HADESMEM_ARCH_X86)

DWORD_PTR __fastcall TestIntegerFast(unsigned int a, unsigned int b, 
  unsigned int c, unsigned int d, unsigned int e, unsigned int f)
{
  BOOST_CHECK_EQUAL(a, 0xAAAAAAAAU);
  BOOST_CHECK_EQUAL(b, 0xBBBBBBBBU);
  BOOST_CHECK_EQUAL(c, 0xCCCCCCCCU);
  BOOST_CHECK_EQUAL(d, 0xDDDDDDDDU);
  BOOST_CHECK_EQUAL(e, 0xEEEEEEEEU);
  BOOST_CHECK_EQUAL(f, 0xFFFFFFFFU);
  
  SetLastError(0x87654321);
  
  return 0x12345678;
}

DWORD_PTR __stdcall TestIntegerStd(unsigned int a, unsigned int b, 
  unsigned int c, unsigned int d, unsigned int e, unsigned int f)
{
  BOOST_CHECK_EQUAL(a, 0xAAAAAAAAU);
  BOOST_CHECK_EQUAL(b, 0xBBBBBBBBU);
  BOOST_CHECK_EQUAL(c, 0xCCCCCCCCU);
  BOOST_CHECK_EQUAL(d, 0xDDDDDDDDU);
  BOOST_CHECK_EQUAL(e, 0xEEEEEEEEU);
  BOOST_CHECK_EQUAL(f, 0xFFFFFFFFU);
  
  SetLastError(0x87654321);
  
  return 0x12345678;
}

// Clang does not yet implement MSVC-style __fastcall (it seems to do so 
// for the 'regular' case, but will do things differently when faced 
// with a 64-bit integer as the first parameter).
#if !defined(HADESMEM_CLANG)

int __fastcall TestInteger64Fast(DWORD64 a)
{
  BOOST_CHECK_EQUAL(a, 0xAAAAAAAABBBBBBBBULL);

  return 0;
}

#endif // #if !defined(HADESMEM_CLANG)

#else
#error "[HadesMem] Unsupported architecture."
#endif

}

BOOST_AUTO_TEST_CASE(initialize)
{
  hadesmem::detail::InitializeAll();
}

BOOST_AUTO_TEST_CASE(call)
{
  hadesmem::Process const process(::GetCurrentProcessId());
  
  typedef DWORD_PTR (TestIntegerT)(unsigned int a, unsigned int b, 
    unsigned int c, unsigned int d, unsigned int e, unsigned int f);
  auto const call_int_ret = hadesmem::Call<TestIntegerT>(
    process, reinterpret_cast<hadesmem::FnPtr>(&TestInteger), 
    hadesmem::CallConv::kDefault, 0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 
    0xDDDDDDDD, 0xEEEEEEEE, 0xFFFFFFFF);
  BOOST_CHECK_EQUAL(call_int_ret.GetReturnValue(), 
    static_cast<DWORD_PTR>(0x12345678));
  
  typedef float (TestFloatT)(float a, float b, float c, float d, float e, 
    float f);
  auto const call_float_ret = hadesmem::Call<TestFloatT>(
    process, reinterpret_cast<hadesmem::FnPtr>(&TestFloat), 
    hadesmem::CallConv::kDefault, 1.11111f, 2.22222f, 3.33333f, 4.44444f, 
    5.55555f, 6.66666f);
  BOOST_CHECK_EQUAL(call_float_ret.GetReturnValue(), 1.23456f);
  
  typedef double (TestDoubleT)(double a, double b, double c, double d, 
    double e, double f);
  auto const call_double_ret = 
    hadesmem::Call<TestDoubleT>(process, reinterpret_cast<hadesmem::FnPtr>(
    &TestDouble), hadesmem::CallConv::kDefault, 1.11111, 2.22222, 3.33333, 
    4.44444, 5.55555, 6.66666);
  BOOST_CHECK_EQUAL(call_double_ret.GetReturnValue(), 1.23456);
  
  struct ImplicitConvTest
  {
    operator int() const
    {
      return -1234;
    }
  };
  unsigned int const lvalue_int = 0xDEAFBEEF;
  typedef DWORD_PTR (TestFuncT)(double a, void const* b, char c, 
    float d, int e, unsigned int f, float g, double h, DummyType const* i, 
    DWORD64 j);
  auto const call_ret = hadesmem::Call<TestFuncT>(process, 
    reinterpret_cast<hadesmem::FnPtr>(&TestMixed), hadesmem::CallConv::kDefault, 
    1337.6666, nullptr, 'c', 9081.736455f, ImplicitConvTest(), lvalue_int, 
    1234.56f, 9876.54, &dummy_glob, 0xAAAAAAAABBBBBBBBULL);
  BOOST_CHECK_EQUAL(call_ret.GetReturnValue(), static_cast<DWORD_PTR>(1234));
  BOOST_CHECK_EQUAL(call_ret.GetLastError(), static_cast<DWORD>(5678));
  
  hadesmem::Call<int (DWORD64 a)>(process, reinterpret_cast<hadesmem::FnPtr>(
    &TestInteger64), hadesmem::CallConv::kDefault, 0xAAAAAAAABBBBBBBBULL);

  auto const call_ptr_ret = hadesmem::Call<char const* ()>(process, 
    reinterpret_cast<hadesmem::FnPtr>(&TestPtrRet), 
    hadesmem::CallConv::kDefault);
  BOOST_CHECK_EQUAL(call_ptr_ret.GetReturnValue(), 
    static_cast<char const* const>(nullptr));

  // Clang does not yet implement MSVC-style __thiscall
#if !defined(HADESMEM_CLANG)

#if defined(HADESMEM_ARCH_X64)
  hadesmem::CallConv const thiscall_call_conv = hadesmem::CallConv::kDefault;
#elif defined(HADESMEM_ARCH_X86)
  hadesmem::CallConv const thiscall_call_conv = hadesmem::CallConv::kThisCall;
#else
#error "[HadesMem] Unsupported architecture."
#endif
  // The following is an extremely disgusting hack that should never be 
  // attempted anywhere by anyone. It relies on the fact that all tested 
  // compilers will lay out the structures representing pointer to member 
  // functions with the function address first (and other data after).
  // The C++ standard strictly forbids casting between the types below.
  // The C++ standard also forbids type punning using the technique below (aka 
  // doing a store to a union using one type then doing a load from the union 
  // using another type).
  // TODO: Find a way to do this without relying on undefined behavior.
  union FuncConv
  {
    decltype(&ThiscallDummy::TestIntegerThis) pmfn;
    hadesmem::FnPtr pfn;
  };
  FuncConv func_conv;
  func_conv.pmfn = &ThiscallDummy::TestIntegerThis;
  auto const test_integer_this = func_conv.pfn;
  ThiscallDummy thiscall_dummy;
  typedef DWORD_PTR (TestIntegerThisT)(ThiscallDummy* instance, 
    unsigned int a, unsigned int b, unsigned int c, unsigned int d, 
    unsigned int e);
  auto const call_int_this_ret = hadesmem::Call<TestIntegerThisT>(
    process, test_integer_this, thiscall_call_conv, &thiscall_dummy, 
    0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE);
  BOOST_CHECK_EQUAL(call_int_this_ret.GetReturnValue(), 
    static_cast<DWORD_PTR>(0x12345678));
  BOOST_CHECK_EQUAL(call_int_this_ret.GetLastError(), 
    static_cast<DWORD>(0x87654321));

#endif // #if !defined(HADESMEM_CLANG)

#if defined(HADESMEM_ARCH_X64)
  
#elif defined(HADESMEM_ARCH_X86)

  auto const call_int_fast_ret = hadesmem::Call<TestIntegerT>(
    process, reinterpret_cast<hadesmem::FnPtr>(&TestIntegerFast), 
    hadesmem::CallConv::kFastCall, 0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 
    0xDDDDDDDD, 0xEEEEEEEE, 0xFFFFFFFF);
  BOOST_CHECK_EQUAL(call_int_fast_ret.GetReturnValue(), 
    static_cast<DWORD_PTR>(0x12345678));
  BOOST_CHECK_EQUAL(call_int_fast_ret.GetLastError(), 
    static_cast<DWORD>(0x87654321));

  auto const call_int_std_ret = hadesmem::Call<TestIntegerT>(
    process, reinterpret_cast<hadesmem::FnPtr>(&TestIntegerStd), 
    hadesmem::CallConv::kStdCall, 0xAAAAAAAA, 0xBBBBBBBB, 0xCCCCCCCC, 
    0xDDDDDDDD, 0xEEEEEEEE, 0xFFFFFFFF);
  BOOST_CHECK_EQUAL(call_int_std_ret.GetReturnValue(), 
    static_cast<DWORD_PTR>(0x12345678));
  BOOST_CHECK_EQUAL(call_int_std_ret.GetLastError(), 
    static_cast<DWORD>(0x87654321));

  // Clang does not yet implement MSVC-style __fastcall (it seems to do so 
  // for the 'regular' case, but will do things differently when faced 
  // with a 64-bit integer as the first parameter).
#if !defined(HADESMEM_CLANG)

  hadesmem::Call<int (DWORD64 a)>(process, reinterpret_cast<hadesmem::FnPtr>(
    &TestInteger64Fast), hadesmem::CallConv::kFastCall, 0xAAAAAAAABBBBBBBBULL);

#endif // #if !defined(HADESMEM_CLANG)

#else
#error "[HadesMem] Unsupported architecture."
#endif
  
  auto const call_ret_64 = hadesmem::Call<DWORD64 ()>(
    process, reinterpret_cast<hadesmem::FnPtr>(&TestCall64Ret), 
    hadesmem::CallConv::kDefault);
  BOOST_CHECK_EQUAL(call_ret_64.GetReturnValue(), 0x123456787654321ULL);
  
  auto const call_ret_float = hadesmem::Call<float ()>(
    process, reinterpret_cast<hadesmem::FnPtr>(&TestCallFloatRet), 
    hadesmem::CallConv::kDefault);
  BOOST_CHECK_EQUAL(call_ret_float.GetReturnValue(), 1.234f);
  
  auto const call_ret_double = 
    hadesmem::Call<double ()>(process, reinterpret_cast<hadesmem::FnPtr>(
    &TestCallDoubleRet), hadesmem::CallConv::kDefault);
  BOOST_CHECK_EQUAL(call_ret_double.GetReturnValue(), 9.876);

  auto const call_ret_void = 
    hadesmem::Call<void ()>(process, reinterpret_cast<hadesmem::FnPtr>(
    &TestCallVoidRet), hadesmem::CallConv::kDefault);
  BOOST_CHECK_EQUAL(call_ret_void.GetLastError(), 0U);
  
  HMODULE const kernel32_mod = ::GetModuleHandle(L"kernel32.dll");
  BOOST_REQUIRE(kernel32_mod != 0);
  
#if defined(HADESMEM_MSVC)
#pragma warning(push)
#pragma warning(disable: 6387)
#endif // #if defined(HADESMEM_MSVC)
  
  FARPROC const get_proc_address_tmp = ::GetProcAddress(kernel32_mod, 
    "GetProcAddress");
  auto const get_proc_address = reinterpret_cast<hadesmem::FnPtr>(
    get_proc_address_tmp);
  BOOST_REQUIRE(get_proc_address != 0);
  
#if defined(HADESMEM_MSVC)
#pragma warning(pop)
#endif // #if defined(HADESMEM_MSVC)
  
  auto const call_win = 
    hadesmem::Call<FARPROC (HMODULE, LPCSTR)>(process, get_proc_address, 
    hadesmem::CallConv::kWinApi, kernel32_mod, "GetProcAddress");
  BOOST_CHECK_EQUAL(call_win.GetReturnValue(), get_proc_address_tmp);
  
  hadesmem::MultiCall multi_call(process);
  multi_call.Add<void (DWORD last_error)>(reinterpret_cast<hadesmem::FnPtr>(
    &MultiThreadSet), hadesmem::CallConv::kDefault, 0x1337UL);
  multi_call.Add<DWORD ()>(reinterpret_cast<hadesmem::FnPtr>(
    &MultiThreadGet), hadesmem::CallConv::kDefault);
  multi_call.Add<void (DWORD last_error)>(reinterpret_cast<hadesmem::FnPtr>(
    &MultiThreadSet), hadesmem::CallConv::kDefault, 0x1234UL);
  multi_call.Add<DWORD ()>(reinterpret_cast<hadesmem::FnPtr>(
    &MultiThreadGet), hadesmem::CallConv::kDefault);
  std::vector<hadesmem::CallResultRaw> multi_call_ret = multi_call.Call();
  BOOST_CHECK_EQUAL(multi_call_ret[0].GetLastError(), 
    0x1337UL);
  BOOST_CHECK_EQUAL(multi_call_ret[1].GetReturnValue<DWORD_PTR>(), 
    0x1337U);
  BOOST_CHECK_EQUAL(multi_call_ret[2].GetLastError(), 
    0x1234UL);
  BOOST_CHECK_EQUAL(multi_call_ret[3].GetReturnValue<DWORD_PTR>(), 
    0x1234U);
}
