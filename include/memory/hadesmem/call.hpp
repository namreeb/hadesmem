// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <vector>
#include <cstddef>
#include <cstring>
#include <utility>
#include <iterator>
#include <algorithm>
#include <type_traits>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/mpl/at.hpp>
#include <boost/variant.hpp>
#include <boost/preprocessor.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <windows.h>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <asmjit/asmjit.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/read.hpp>
#include <hadesmem/alloc.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/write.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/detail/func_args.hpp>
#include <hadesmem/detail/func_arity.hpp>
#include <hadesmem/detail/func_result.hpp>
#include <hadesmem/detail/smart_handle.hpp>
#include <hadesmem/detail/remote_thread.hpp>
#include <hadesmem/detail/static_assert.hpp>

// TODO: Cross-session injection (also cross-winsta and cross-desktop 
// injection). Easiest solution is to use a broker process via a service 
// and CreateProcessAsUser. Potentially 'better' solution would be to use 
// NtCreateThread/RtlCreateUserThread.

// TODO: Support injection into CSRSS. CreateRemoteThread can't be used on 
// CSRSS because when the thread is initialized it tries to notify CSRSS of 
// the thread creation and gets confused. Potential workaround is to use 
// NtCreateThread/RtlCreateUserThread.

// TODO: Support using NtCreateThread/RtlCreateUserThread. Does not create an 
// activation context, so it will need special work done to get cases like 
// .NET working.

// TODO: Support injected code using only NT APIs (for smss.exe etc).

// TODO: Support injection into uninitialized processes, native processes, 
// CSRSS, etc.

// TODO: Improve safety via EH. Both x86 and x64.

// TODO: Clean up ASM code and code generation.

// TODO: Add support for more 'complex' argument and return types, including 
// struct/class/union, long double, SIMD types, etc. A good reference for 
// calling conventions is available at http://bit.ly/3CvgMV.

// TODO: Add support for 'custom' calling conventions (e.g. in PGO-generated 
// code, 'private' functions, obfuscated code, etc).

// TODO: Only JIT code for Call once, then cache. Rewrite to pull data 
// externally instead of being regenerated for every call.

// TODO: Once the JIT-once rewrite is complete, transition to using code 
// generated at compile-time with FASM and stored in a binary 'blob' 
// (embedded in the source). This will remove the dependency on AsmJit.

// TODO: Clean up x64 assembly related to the stack. Important for EH.

// TODO: Wow64 bypass.

// TODO: Split this mess up into multiple headers where possible.

// TODO: Add a 'thumbprint' to all memory allocations so the blocks can be 
// easily identified in a debugger.

// TODO: Consolidate memory allocations where possible.

namespace hadesmem
{
  
typedef void (*FnPtr)();
HADESMEM_DETAIL_STATIC_ASSERT(sizeof(FnPtr) == sizeof(void*));

enum class CallConv
{
  kDefault, 
  kWinApi, 
  kCdecl, 
  kStdCall, 
  kThisCall, 
  kFastCall, 
  kX64
};

template <typename T>
class CallResult
{
public:
  HADESMEM_DETAIL_STATIC_ASSERT(std::is_integral<T>::value || 
    std::is_pointer<T>::value || 
    std::is_same<float, typename std::remove_cv<T>::type>::value || 
    std::is_same<double, typename std::remove_cv<T>::type>::value);

  explicit CallResult(T const& result, DWORD last_error) HADESMEM_DETAIL_NOEXCEPT
    : result_(result), 
    last_error_(last_error)
  { }

  T GetReturnValue() const HADESMEM_DETAIL_NOEXCEPT
  {
    return result_;
  }

  DWORD GetLastError() const HADESMEM_DETAIL_NOEXCEPT
  {
    return last_error_;
  }

private:
  T result_;
  DWORD last_error_;
};

template <>
class CallResult<void>
{
public:
  explicit CallResult(DWORD last_error) HADESMEM_DETAIL_NOEXCEPT
    : last_error_(last_error)
  { }

  DWORD GetLastError() const HADESMEM_DETAIL_NOEXCEPT
  {
    return last_error_;
  }

private:
  DWORD last_error_;
};

namespace detail
{

struct CallResultRemote
{
  DWORD64 return_i64;
  float return_float;
  double return_double;
  DWORD last_error;
};

// CallResultRemote must be POD because of 'offsetof' usage.
HADESMEM_DETAIL_STATIC_ASSERT(std::is_pod<detail::CallResultRemote>::value);

}

class CallResultRaw
{
public:
  explicit CallResultRaw(DWORD64 return_i64, float return_float, 
    double return_double, DWORD last_error) HADESMEM_DETAIL_NOEXCEPT
    : result_()
  {
    result_.return_i64 = return_i64;
    result_.return_float = return_float;
    result_.return_double = return_double;
    result_.last_error = last_error;
  }

  explicit CallResultRaw(detail::CallResultRemote const& result) 
    HADESMEM_DETAIL_NOEXCEPT
    : result_(result)
  { }

  DWORD GetLastError() const HADESMEM_DETAIL_NOEXCEPT
  {
    return result_.last_error;
  }

  template <typename T>
  typename std::decay<T>::type GetReturnValue() const HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_STATIC_ASSERT(std::is_integral<T>::value || 
      std::is_pointer<T>::value || 
      std::is_same<float, typename std::decay<T>::type>::value || 
      std::is_same<double, typename std::decay<T>::type>::value);

    typedef typename std::decay<T>::type U;
    return GetReturnValueImpl<U>(std::is_pointer<U>());
  }
  
private:
  template <typename T>
  T GetReturnValueIntImpl(std::true_type) const HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_STATIC_ASSERT(std::is_integral<T>::value);
    return static_cast<T>(GetReturnValueInt64());
  }

  template <typename T>
  T GetReturnValueIntImpl(std::false_type) const HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_STATIC_ASSERT(sizeof(T) <= sizeof(DWORD32));
    HADESMEM_DETAIL_STATIC_ASSERT(std::is_integral<T>::value);
    return static_cast<T>(GetReturnValueInt32());
  }

  template <typename T>
  T GetReturnValuePtrImpl(std::true_type) const HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_STATIC_ASSERT(std::is_pointer<T>::value);
    return reinterpret_cast<T>(GetReturnValueInt64());
  }

  template <typename T>
  T GetReturnValuePtrImpl(std::false_type) const HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_STATIC_ASSERT(std::is_pointer<T>::value);
    return reinterpret_cast<T>(GetReturnValueInt32());
  }

  template <typename T>
  T GetReturnValueIntOrFloatImpl(std::true_type, std::false_type, 
    std::false_type) const HADESMEM_DETAIL_NOEXCEPT
  {
    return GetReturnValueFloat();
  }

  template <typename T>
  T GetReturnValueIntOrFloatImpl(std::false_type, std::true_type, 
    std::false_type) const HADESMEM_DETAIL_NOEXCEPT
  {
    return GetReturnValueDouble();
  }

  template <typename T>
  T GetReturnValueIntOrFloatImpl(std::false_type, std::false_type, 
    std::true_type) const HADESMEM_DETAIL_NOEXCEPT
  {
    return GetReturnValueIntImpl<T>(std::integral_constant<bool, 
      (sizeof(T) == sizeof(DWORD64))>());
  }

  template <typename T>
  T GetReturnValueImpl(std::true_type) const HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_STATIC_ASSERT(std::is_pointer<T>::value);
    return GetReturnValuePtrImpl<T>(std::integral_constant<bool, 
      (sizeof(void*) == sizeof(DWORD64))>());
  }

  template <typename T>
  T GetReturnValueImpl(std::false_type) const HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_STATIC_ASSERT(std::is_integral<T>::value || 
      std::is_floating_point<T>::value);
    return GetReturnValueIntOrFloatImpl<T>(std::is_same<T, float>(), 
      std::is_same<T, double>(), std::is_integral<T>());
  }

  DWORD_PTR GetReturnValueIntPtr() const HADESMEM_DETAIL_NOEXCEPT
  {
    return static_cast<DWORD_PTR>(result_.return_i64 & 
      static_cast<DWORD_PTR>(-1));
  }

  DWORD32 GetReturnValueInt32() const HADESMEM_DETAIL_NOEXCEPT
  {
    return static_cast<DWORD32>(result_.return_i64 & 0xFFFFFFFFUL);
  }

  DWORD64 GetReturnValueInt64() const HADESMEM_DETAIL_NOEXCEPT
  {
    return result_.return_i64;
  }

  float GetReturnValueFloat() const HADESMEM_DETAIL_NOEXCEPT
  {
    return result_.return_float;
  }

  double GetReturnValueDouble() const HADESMEM_DETAIL_NOEXCEPT
  {
    return result_.return_double;
  }

  detail::CallResultRemote result_;
};

namespace detail
{

template <typename T>
CallResult<T> CallResultRawToCallResult(CallResultRaw const& result) 
  HADESMEM_DETAIL_NOEXCEPT
{
  return CallResult<T>(result.GetReturnValue<T>(), result.GetLastError());
}

template <>
inline CallResult<void> CallResultRawToCallResult(CallResultRaw const& result) 
  HADESMEM_DETAIL_NOEXCEPT
{
  return CallResult<void>(result.GetLastError());
}

}

class CallArg
{
public:
  template <typename T>
  explicit CallArg(T t) HADESMEM_DETAIL_NOEXCEPT
    : arg_()
  {
    HADESMEM_DETAIL_STATIC_ASSERT(std::is_integral<T>::value || 
      std::is_pointer<T>::value || 
      std::is_same<float, typename std::remove_cv<T>::type>::value || 
      std::is_same<double, typename std::remove_cv<T>::type>::value);
    
    Initialize(t);
  }
  
  boost::variant<DWORD32, DWORD64, float, double> GetVariant() const
  {
    return arg_;
  }
  
private:
  template <typename T>
  void Initialize(T t) HADESMEM_DETAIL_NOEXCEPT
  {
    typedef typename std::conditional<sizeof(T) == sizeof(DWORD64), DWORD64, 
      DWORD32>::type D;
    Initialize(static_cast<D>(t));
  }

  template <typename T>
  void Initialize(T const* t) HADESMEM_DETAIL_NOEXCEPT
  {
    typedef typename std::conditional<sizeof(T const*) == sizeof(DWORD64), 
      DWORD64, DWORD32>::type D;
    Initialize(reinterpret_cast<D>(t));
  }

  template <typename T>
  void Initialize(T* t) HADESMEM_DETAIL_NOEXCEPT
  {
    Initialize(static_cast<T const*>(t));
  }

  void Initialize(DWORD32 t) HADESMEM_DETAIL_NOEXCEPT
  {
    arg_ = t;
  }

  void Initialize(DWORD64 t) HADESMEM_DETAIL_NOEXCEPT
  {
    arg_ = t;
  }
  
  void Initialize(float t) HADESMEM_DETAIL_NOEXCEPT
  {
    arg_ = t;
  }
  
  void Initialize(double t) HADESMEM_DETAIL_NOEXCEPT
  {
    arg_ = t;
  }
  
  typedef boost::variant<DWORD32, DWORD64, float, double> Arg;
  Arg arg_;
};

namespace detail
{
  
#if defined(HADESMEM_DETAIL_ARCH_X86)

class ArgVisitor32 : public boost::static_visitor<>
{
public:
  ArgVisitor32(AsmJit::X86Assembler* assembler, std::size_t num_args, 
    CallConv call_conv) HADESMEM_DETAIL_NOEXCEPT;

  void operator()(DWORD32 arg) HADESMEM_DETAIL_NOEXCEPT;

  void operator()(DWORD64 arg) HADESMEM_DETAIL_NOEXCEPT;

  void operator()(float arg) HADESMEM_DETAIL_NOEXCEPT;

  void operator()(double arg) HADESMEM_DETAIL_NOEXCEPT;

private:
  AsmJit::X86Assembler* assembler_;
  std::size_t cur_arg_;
  CallConv call_conv_;
};

#endif // #if defined(HADESMEM_DETAIL_ARCH_X86)

#if defined(HADESMEM_DETAIL_ARCH_X64)

class ArgVisitor64 : public boost::static_visitor<>
{
public:
  ArgVisitor64(AsmJit::X86Assembler* assembler, std::size_t num_args) 
    HADESMEM_DETAIL_NOEXCEPT;

  void operator()(DWORD32 arg) HADESMEM_DETAIL_NOEXCEPT;

  void operator()(DWORD64 arg) HADESMEM_DETAIL_NOEXCEPT;

  void operator()(float arg) HADESMEM_DETAIL_NOEXCEPT;

  void operator()(double arg) HADESMEM_DETAIL_NOEXCEPT;

private:
  AsmJit::X86Assembler* assembler_;
  std::size_t num_args_;
  std::size_t cur_arg_;
};

#endif // #if defined(HADESMEM_DETAIL_ARCH_X64)

#if defined(HADESMEM_DETAIL_ARCH_X86)

template <typename AddressesForwardIterator, 
  typename ConvForwardIterator, 
  typename ArgsForwardIterator>
inline void GenerateCallCode32(AsmJit::X86Assembler* assembler, 
  AddressesForwardIterator addresses_beg, 
  AddressesForwardIterator addresses_end, 
  ConvForwardIterator call_convs_beg, 
  ArgsForwardIterator args_full_beg, 
  DWORD_PTR get_last_error, 
  DWORD_PTR set_last_error, 
  DWORD_PTR is_debugger_present, 
  DWORD_PTR debug_break, 
  PVOID return_values_remote)
{
  HADESMEM_DETAIL_TRACE_A("GenerateCallCode32 called.");

  AsmJit::Label label_nodebug(assembler->newLabel());

  assembler->push(AsmJit::ebp);
  assembler->mov(AsmJit::ebp, AsmJit::esp);

  assembler->mov(AsmJit::eax, AsmJit::uimm(is_debugger_present));
  assembler->call(AsmJit::eax);

  assembler->test(AsmJit::eax, AsmJit::eax);
  assembler->jz(label_nodebug);

  assembler->mov(AsmJit::eax, AsmJit::uimm(debug_break));
  assembler->call(AsmJit::eax);

  assembler->bind(label_nodebug);

  assembler->push(AsmJit::imm(0x0));
  assembler->mov(AsmJit::eax, AsmJit::uimm(set_last_error));
  assembler->call(AsmJit::eax);
  
  for (std::size_t i = 0; addresses_beg != addresses_end; ++addresses_beg, 
    ++call_convs_beg, ++args_full_beg, ++i)
  {
    FnPtr address = *addresses_beg;
    CallConv call_conv = *call_convs_beg;
    auto const& args = *args_full_beg;
    // TODO: Make this code more generic and remove the dependency on size().
    std::size_t const num_args = args.size();

    ArgVisitor32 arg_visitor(assembler, num_args, call_conv);
    // TODO: Make this code more generic and remove the dependency on 
    // rbegin() and rend().
    std::for_each(args.rbegin(), args.rend(), 
      [&] (CallArg const& arg)
    {
      auto variant = arg.GetVariant();
      boost::apply_visitor(arg_visitor, variant);
    });

    assembler->mov(AsmJit::eax, reinterpret_cast<sysint_t>(address));
    assembler->call(AsmJit::eax);

    assembler->mov(AsmJit::ecx, AsmJit::uimm(
      reinterpret_cast<DWORD_PTR>(return_values_remote) + 
      i * sizeof(detail::CallResultRemote) + 
      offsetof(detail::CallResultRemote, return_i64)));
    assembler->mov(AsmJit::dword_ptr(AsmJit::ecx), AsmJit::eax);
    assembler->mov(AsmJit::dword_ptr(AsmJit::ecx, 4), AsmJit::edx);

    assembler->mov(AsmJit::ecx, AsmJit::uimm(
      reinterpret_cast<DWORD_PTR>(return_values_remote) + 
      i * sizeof(detail::CallResultRemote) + 
      offsetof(detail::CallResultRemote, return_float)));
    assembler->fst(AsmJit::dword_ptr(AsmJit::ecx));

    assembler->mov(AsmJit::ecx, AsmJit::uimm(
      reinterpret_cast<DWORD_PTR>(return_values_remote) + 
      i * sizeof(detail::CallResultRemote) + 
      offsetof(detail::CallResultRemote, return_double)));
    assembler->fst(AsmJit::qword_ptr(AsmJit::ecx));

    assembler->mov(AsmJit::eax, AsmJit::uimm(get_last_error));
    assembler->call(AsmJit::eax);

    assembler->mov(AsmJit::ecx, AsmJit::uimm(
      reinterpret_cast<DWORD_PTR>(return_values_remote) + 
      i * sizeof(detail::CallResultRemote) + 
      offsetof(detail::CallResultRemote, last_error)));
    assembler->mov(AsmJit::dword_ptr(AsmJit::ecx), AsmJit::eax);

    if (call_conv == CallConv::kDefault || call_conv == CallConv::kCdecl)
    {
      assembler->add(AsmJit::esp, AsmJit::uimm(num_args * sizeof(PVOID)));
    }
  }

  assembler->mov(AsmJit::esp, AsmJit::ebp);
  assembler->pop(AsmJit::ebp);

  assembler->ret(0x4);
}

#endif // #if defined(HADESMEM_DETAIL_ARCH_X86)

#if defined(HADESMEM_DETAIL_ARCH_X64)

template <typename AddressesForwardIterator, 
  typename ConvForwardIterator, 
  typename ArgsForwardIterator>
inline void GenerateCallCode64(AsmJit::X86Assembler* assembler, 
  AddressesForwardIterator addresses_beg, 
  AddressesForwardIterator addresses_end, 
  ConvForwardIterator call_convs_beg, 
  ArgsForwardIterator args_full_beg, 
  DWORD_PTR get_last_error, 
  DWORD_PTR set_last_error, 
  DWORD_PTR is_debugger_present, 
  DWORD_PTR debug_break, 
  PVOID return_values_remote)
{
  HADESMEM_DETAIL_TRACE_A("GenerateCallCode64 called.");

  AsmJit::Label label_nodebug(assembler->newLabel());
  
  std::size_t const num_addresses = std::distance(addresses_beg, 
    addresses_end);
  
  // TODO: Make this code more generic and remove the dependency on 
  // std::vector<CallArg>.
  auto const max_args_list = std::max_element(args_full_beg, 
    args_full_beg + num_addresses, 
    [] (std::vector<CallArg> const& args1, std::vector<CallArg> const& args2)
  {
    return args1.size() < args2.size();
  });
  // TODO: Make this code more generic and remove the dependency on 
  // size().
  std::size_t const max_num_args = max_args_list->size();

  // TODO: Comment/document this properly.
  std::size_t stack_offs = (std::max)(static_cast<std::size_t>(0x20), 
    max_num_args * 0x8);
  stack_offs += (stack_offs % 16);
  // TODO: Fix argument handling code to not require the extra scratch space.
  stack_offs += 16;
  stack_offs += 8;

  assembler->sub(AsmJit::rsp, stack_offs);

  assembler->mov(AsmJit::rax, is_debugger_present);
  assembler->call(AsmJit::rax);

  assembler->test(AsmJit::rax, AsmJit::rax);
  assembler->jz(label_nodebug);

  assembler->mov(AsmJit::rax, debug_break);
  assembler->call(AsmJit::rax);

  assembler->bind(label_nodebug);

  assembler->mov(AsmJit::rcx, 0);
  assembler->mov(AsmJit::rax, set_last_error);
  assembler->call(AsmJit::rax);
  
  for (std::size_t i = 0; addresses_beg != addresses_end; ++addresses_beg, 
    ++call_convs_beg, ++args_full_beg, ++i)
  {
    FnPtr address = *addresses_beg;
    auto const& args = *args_full_beg;
    // TODO: Make this code more generic and remove the dependency on size().
    std::size_t const num_args = args.size();

    HADESMEM_DETAIL_ASSERT(*call_convs_beg == CallConv::kDefault || 
      *call_convs_beg == CallConv::kWinApi || 
      *call_convs_beg == CallConv::kX64);

    ArgVisitor64 arg_visitor(assembler, num_args);
    // TODO: Make this code more generic and remove the dependency on 
    // rbegin() and rend().
    std::for_each(args.rbegin(), args.rend(), 
      [&] (CallArg const& arg)
    {
      auto variant = arg.GetVariant();
      boost::apply_visitor(arg_visitor, variant);
    });

    assembler->mov(AsmJit::rax, reinterpret_cast<DWORD_PTR>(address));
    assembler->call(AsmJit::rax);

    assembler->mov(AsmJit::rcx, reinterpret_cast<DWORD_PTR>(
      return_values_remote) + i * sizeof(detail::CallResultRemote) + 
      offsetof(detail::CallResultRemote, return_i64));
    assembler->mov(AsmJit::qword_ptr(AsmJit::rcx), AsmJit::rax);

    assembler->mov(AsmJit::rcx, reinterpret_cast<DWORD_PTR>(
      return_values_remote) + i * sizeof(detail::CallResultRemote) + 
      offsetof(detail::CallResultRemote, return_float));
    assembler->movss(AsmJit::dword_ptr(AsmJit::rcx), AsmJit::xmm0);

    assembler->mov(AsmJit::rcx, reinterpret_cast<DWORD_PTR>(
      return_values_remote) + i * sizeof(detail::CallResultRemote) + 
      offsetof(detail::CallResultRemote, return_double));
    assembler->movsd(AsmJit::qword_ptr(AsmJit::rcx), AsmJit::xmm0);

    assembler->mov(AsmJit::rax, get_last_error);
    assembler->call(AsmJit::rax);

    assembler->mov(AsmJit::rcx, reinterpret_cast<DWORD_PTR>(
      return_values_remote) + i * sizeof(detail::CallResultRemote) + 
      offsetof(detail::CallResultRemote, last_error));
    assembler->mov(AsmJit::dword_ptr(AsmJit::rcx), AsmJit::eax);
  }

  assembler->add(AsmJit::rsp, stack_offs);

  assembler->ret();
}

#endif // #if defined(HADESMEM_DETAIL_ARCH_X64)
  
template <typename AddressesForwardIterator, 
  typename ConvForwardIterator, 
  typename ArgsForwardIterator>
inline Allocator GenerateCallCode(Process const& process, 
  AddressesForwardIterator addresses_beg, 
  AddressesForwardIterator addresses_end, 
  ConvForwardIterator call_convs_beg, 
  ArgsForwardIterator args_full_beg, 
  PVOID return_values_remote)
{
  HADESMEM_DETAIL_TRACE_A("GenerateCallCode called.");

  Module const kernel32(process, L"kernel32.dll");
  auto const get_last_error = reinterpret_cast<DWORD_PTR>(FindProcedure(
    process, kernel32, "GetLastError"));
  auto const set_last_error = reinterpret_cast<DWORD_PTR>(FindProcedure(
    process, kernel32, "SetLastError"));
  auto const is_debugger_present = reinterpret_cast<DWORD_PTR>(FindProcedure(
    process, kernel32, "IsDebuggerPresent"));
  auto const debug_break = reinterpret_cast<DWORD_PTR>(FindProcedure(
    process, kernel32, "DebugBreak"));

  AsmJit::X86Assembler assembler;
  
#if defined(HADESMEM_DETAIL_ARCH_X64)
  GenerateCallCode64(
#elif defined(HADESMEM_DETAIL_ARCH_X86)
  GenerateCallCode32(
#else
#error "[HadesMem] Unsupported architecture."
#endif
    &assembler, 
    addresses_beg, addresses_end, 
    call_convs_beg, 
    args_full_beg, 
    get_last_error, 
    set_last_error, 
    is_debugger_present, 
    debug_break, 
    return_values_remote);
  
  DWORD_PTR const stub_size = assembler.getCodeSize();
  
  HADESMEM_DETAIL_TRACE_A("Allocating memory for remote stub.");

  Allocator stub_mem_remote(process, stub_size);
  
  HADESMEM_DETAIL_TRACE_A("Performing code relocation.");

  std::vector<BYTE> code_real(stub_size);
  assembler.relocCode(code_real.data(), reinterpret_cast<DWORD_PTR>(
    stub_mem_remote.GetBase()));
  
  HADESMEM_DETAIL_TRACE_A("Writing remote code stub.");

  WriteVector(process, stub_mem_remote.GetBase(), code_real);

  return stub_mem_remote;
}

#if defined(HADESMEM_DETAIL_ARCH_X86)

ArgVisitor32::ArgVisitor32(AsmJit::X86Assembler* assembler, 
  std::size_t num_args, CallConv call_conv) HADESMEM_DETAIL_NOEXCEPT
  : assembler_(assembler), 
  cur_arg_(num_args), 
  call_conv_(call_conv)
{ }

void ArgVisitor32::operator()(DWORD32 arg) HADESMEM_DETAIL_NOEXCEPT
{
  switch (cur_arg_)
  {
  case 1:
    switch (call_conv_)
    {
    case CallConv::kThisCall:
    case CallConv::kFastCall:
      assembler_->mov(AsmJit::ecx, AsmJit::uimm(arg));
      break;
    case CallConv::kDefault:
    case CallConv::kWinApi:
    case CallConv::kCdecl:
    case CallConv::kStdCall:
    case CallConv::kX64:
      assembler_->mov(AsmJit::eax, AsmJit::uimm(arg));
      assembler_->push(AsmJit::eax);
      break;
    }
    break;
  case 2:
    switch (call_conv_)
    {
    case CallConv::kFastCall:
      assembler_->mov(AsmJit::edx, AsmJit::uimm(arg));
      break;
    case CallConv::kDefault:
    case CallConv::kWinApi:
    case CallConv::kCdecl:
    case CallConv::kStdCall:
    case CallConv::kThisCall:
    case CallConv::kX64:
      assembler_->mov(AsmJit::eax, AsmJit::uimm(arg));
      assembler_->push(AsmJit::eax);
      break;
    }
    break;
  default:
    assembler_->mov(AsmJit::eax, AsmJit::uimm(arg));
    assembler_->push(AsmJit::eax);
    break;
  }

  --cur_arg_;
}

void ArgVisitor32::operator()(DWORD64 arg) HADESMEM_DETAIL_NOEXCEPT
{
  assembler_->mov(AsmJit::eax, AsmJit::uimm(static_cast<DWORD>(
    (arg >> 32) & 0xFFFFFFFFUL)));
  assembler_->push(AsmJit::eax);

  assembler_->mov(AsmJit::eax, AsmJit::uimm(static_cast<DWORD>(arg & 
    0xFFFFFFFFUL)));
  assembler_->push(AsmJit::eax);

  --cur_arg_;
}

void ArgVisitor32::operator()(float arg) HADESMEM_DETAIL_NOEXCEPT
{
  HADESMEM_DETAIL_STATIC_ASSERT(sizeof(float) == 4);
  HADESMEM_DETAIL_STATIC_ASSERT(sizeof(float) == sizeof(DWORD));

  DWORD arg_conv;
  std::memcpy(&arg_conv, &arg, sizeof(arg));

  assembler_->mov(AsmJit::eax, AsmJit::uimm(arg_conv));
  assembler_->push(AsmJit::eax);

  --cur_arg_;
}

void ArgVisitor32::operator()(double arg) HADESMEM_DETAIL_NOEXCEPT
{
  HADESMEM_DETAIL_STATIC_ASSERT(sizeof(double) == 8);
  HADESMEM_DETAIL_STATIC_ASSERT(sizeof(double) == sizeof(DWORD64));

  DWORD64 arg_conv;
  std::memcpy(&arg_conv, &arg, sizeof(arg));

  assembler_->mov(AsmJit::eax, AsmJit::uimm(static_cast<DWORD>(
    (arg_conv >> 32) & 0xFFFFFFFFUL)));
  assembler_->push(AsmJit::eax);

  assembler_->mov(AsmJit::eax, AsmJit::uimm(static_cast<DWORD>(
    arg_conv & 0xFFFFFFFFUL)));
  assembler_->push(AsmJit::eax);

  --cur_arg_;
}

#endif // #if defined(HADESMEM_DETAIL_ARCH_X86)

#if defined(HADESMEM_DETAIL_ARCH_X64)

ArgVisitor64::ArgVisitor64(AsmJit::X86Assembler* assembler, 
  std::size_t num_args) HADESMEM_DETAIL_NOEXCEPT
  : assembler_(assembler), 
  num_args_(num_args), 
  cur_arg_(num_args)
{ }

void ArgVisitor64::operator()(DWORD32 arg) HADESMEM_DETAIL_NOEXCEPT
{
  return (*this)(static_cast<DWORD64>(arg));
}

void ArgVisitor64::operator()(DWORD64 arg) HADESMEM_DETAIL_NOEXCEPT
{
  switch (cur_arg_)
  {
  case 1:
    assembler_->mov(AsmJit::rcx, arg);
    break;
  case 2:
    assembler_->mov(AsmJit::rdx, arg);
    break;
  case 3:
    assembler_->mov(AsmJit::r8, arg);
    break;
  case 4:
    assembler_->mov(AsmJit::r9, arg);
    break;
  default:
    assembler_->mov(AsmJit::dword_ptr(AsmJit::rsp, (cur_arg_ - 1) * 8), 
      static_cast<DWORD>(arg & 0xFFFFFFFFUL));
    assembler_->mov(AsmJit::dword_ptr(AsmJit::rsp, (cur_arg_ - 1) * 8 + 4), 
      static_cast<DWORD>((arg >> 32) & 0xFFFFFFFFUL));
    break;
  }

  --cur_arg_;
}

void ArgVisitor64::operator()(float arg) HADESMEM_DETAIL_NOEXCEPT
{
  HADESMEM_DETAIL_STATIC_ASSERT(sizeof(float) == 4);
  HADESMEM_DETAIL_STATIC_ASSERT(sizeof(float) == sizeof(DWORD));

  DWORD arg_conv;
  std::memcpy(&arg_conv, &arg, sizeof(arg));

  std::size_t const scratch_offs = num_args_ * 8;

  switch (cur_arg_)
  {
  case 1:
  case 2:
  case 3:
  case 4:
    assembler_->mov(AsmJit::dword_ptr(AsmJit::rsp, scratch_offs), arg_conv);
    break;
  default:
    break;
  }

  switch (cur_arg_)
  {
  case 1:
    assembler_->movss(AsmJit::xmm0, AsmJit::dword_ptr(AsmJit::rsp, 
      scratch_offs));
    break;
  case 2:
    assembler_->movss(AsmJit::xmm1, AsmJit::dword_ptr(AsmJit::rsp, 
      scratch_offs));
    break;
  case 3:
    assembler_->movss(AsmJit::xmm2, AsmJit::dword_ptr(AsmJit::rsp, 
      scratch_offs));
    break;
  case 4:
    assembler_->movss(AsmJit::xmm3, AsmJit::dword_ptr(AsmJit::rsp, 
      scratch_offs));
    break;
  default:
    assembler_->mov(AsmJit::dword_ptr(AsmJit::rsp, (cur_arg_ - 1) * 8), 
      arg_conv);
    break;
  }

  --cur_arg_;
}

void ArgVisitor64::operator()(double arg) HADESMEM_DETAIL_NOEXCEPT
{
  HADESMEM_DETAIL_STATIC_ASSERT(sizeof(double) == 8);
  HADESMEM_DETAIL_STATIC_ASSERT(sizeof(double) == sizeof(DWORD64));

  DWORD64 arg_conv;
  std::memcpy(&arg_conv, &arg, sizeof(arg));

  DWORD const arg_low = static_cast<DWORD>(arg_conv & 0xFFFFFFFFUL);
  DWORD const arg_high = static_cast<DWORD>((arg_conv >> 32) & 0xFFFFFFFFUL);

  std::size_t const scratch_offs = num_args_ * 8;

  switch (cur_arg_)
  {
  case 1:
  case 2:
  case 3:
  case 4:
    assembler_->mov(AsmJit::dword_ptr(AsmJit::rsp, scratch_offs), arg_low);
    assembler_->mov(AsmJit::dword_ptr(AsmJit::rsp, scratch_offs + 4), 
      arg_high);
    break;
  default:
    break;
  }

  switch (cur_arg_)
  {
  case 1:
    assembler_->movsd(AsmJit::xmm0, AsmJit::qword_ptr(AsmJit::rsp, 
      scratch_offs));
    break;
  case 2:
    assembler_->movsd(AsmJit::xmm1, AsmJit::qword_ptr(AsmJit::rsp, 
      scratch_offs));
    break;
  case 3:
    assembler_->movsd(AsmJit::xmm2, AsmJit::qword_ptr(AsmJit::rsp, 
      scratch_offs));
    break;
  case 4:
    assembler_->movsd(AsmJit::xmm3, AsmJit::qword_ptr(AsmJit::rsp, 
      scratch_offs));
    break;
  default:
    assembler_->mov(AsmJit::dword_ptr(AsmJit::rsp, (cur_arg_ - 1) * 8), 
      arg_low);
    assembler_->mov(AsmJit::dword_ptr(AsmJit::rsp, (cur_arg_ - 1) * 8 + 4), 
      arg_high);
    break;
  }

  --cur_arg_;
}

#endif // #if defined(HADESMEM_DETAIL_ARCH_X64)

}

template <typename AddressesForwardIterator, 
  typename ConvForwardIterator, 
  typename ArgsForwardIterator, 
  typename ResultsOutputIterator>
inline void CallMulti(Process const& process, 
  AddressesForwardIterator addresses_beg, 
  AddressesForwardIterator addresses_end, 
  ConvForwardIterator call_convs_beg, 
  ArgsForwardIterator args_full_beg, 
  ResultsOutputIterator results)
{
  // TODO: Iterator checks for type and category.
  
  HADESMEM_DETAIL_TRACE_A("CallMulti called.");

  auto const num_addresses_signed = std::distance(addresses_beg, 
    addresses_end);
  HADESMEM_DETAIL_ASSERT(num_addresses_signed > 0);
  auto const num_addresses = static_cast<typename std::make_unsigned<decltype(
    num_addresses_signed)>::type>(num_addresses_signed);
  
  HADESMEM_DETAIL_TRACE_A("Allocating memory for return values.");

  Allocator const return_values_remote(process, 
    sizeof(detail::CallResultRemote) * num_addresses);
  
  HADESMEM_DETAIL_TRACE_A("Allocating memory for code stub.");

  Allocator const code_remote(detail::GenerateCallCode(process, 
    addresses_beg, addresses_end, 
    call_convs_beg, 
    args_full_beg, 
    return_values_remote.GetBase()));
  LPTHREAD_START_ROUTINE code_remote_pfn = 
    reinterpret_cast<LPTHREAD_START_ROUTINE>(
    reinterpret_cast<DWORD_PTR>(code_remote.GetBase()));
  
  HADESMEM_DETAIL_TRACE_A("Creating remote thread and waiting.");

  // TODO: Configurable timeout. This will complicate resource management 
  // however, as we will need to extend the lifetime of the remote memory 
  // in case it executes after we time out. Also, if it times out there 
  // is no way to try again in the future... Should we just leak the memory 
  // on timeout? Return a 'future' object? Some sort of combination? Requires 
  // more investigation...
  detail::CreateRemoteThreadAndWait(process, code_remote_pfn);
  
  HADESMEM_DETAIL_TRACE_A("Reading return values.");

  std::vector<detail::CallResultRemote> return_vals_remote = 
    ReadVector<detail::CallResultRemote>(process, 
    return_values_remote.GetBase(), num_addresses);
  
  std::vector<CallResultRaw> return_vals;
  return_vals.reserve(num_addresses);
  
  // TODO: Ensure that the documentation covers the exception guarantee for 
  // what happens if we throw while writing to the output iterator.
  std::transform(std::begin(return_vals_remote), std::end(return_vals_remote), 
    results, 
    [] (detail::CallResultRemote const& r)
    {
      return static_cast<CallResultRaw>(r);
    });
}

template <typename ArgsForwardIterator>
inline CallResultRaw Call(Process const& process, 
  FnPtr address, 
  CallConv call_conv, 
  ArgsForwardIterator args_beg, 
  ArgsForwardIterator args_end)
{
  std::vector<FnPtr> addresses;
  addresses.push_back(address);
  std::vector<CallConv> call_convs;
  call_convs.push_back(call_conv);
  std::vector<std::vector<CallArg>> args_full;
  args_full.emplace_back(args_beg, args_end);
  std::vector<CallResultRaw> results;
  CallMulti(process, 
    std::begin(addresses), std::end(addresses), 
    std::begin(call_convs), 
    std::begin(args_full), 
    std::back_inserter(results));
  BOOST_ASSERT(results.size() == 1);
  return results.front();
}

namespace detail
{

#if defined(HADESMEM_MSVC)
#pragma warning(push)
#pragma warning(disable: 4100)
#endif // #if defined(HADESMEM_MSVC)

template <typename FuncT, int N, typename T, typename OutputIterator>
void AddCallArg(OutputIterator call_args, T&& arg)
{
  typedef typename detail::FuncArgs<FuncT>::type FuncArgs;
  typedef typename boost::mpl::at_c<FuncArgs, N>::type RealT;
  HADESMEM_DETAIL_STATIC_ASSERT(std::is_convertible<T, RealT>::value);
  *call_args = static_cast<CallArg>(static_cast<RealT>(std::forward<T>(arg)));
}

#if defined(HADESMEM_MSVC)
#pragma warning(pop)
#endif // #if defined(HADESMEM_MSVC)

}

#if !defined(HADESMEM_DETAIL_NO_VARIADIC_TEMPLATES)

namespace detail
{

template <typename FuncT, int N, typename OutputIterator>
void BuildCallArgs(OutputIterator /*call_args*/) HADESMEM_DETAIL_NOEXCEPT
{
  return;
}

template <typename FuncT, int N, typename T, typename OutputIterator, 
  typename... Args>
void BuildCallArgs(OutputIterator call_args, T&& arg, Args&&... args)
{
  AddCallArg<FuncT, N>(call_args, std::forward<T>(arg));
  return BuildCallArgs<FuncT, N + 1>(
    ++call_args, 
    std::forward<Args>(args)...);
}

}

template <typename FuncT, typename... Args>
CallResult<typename detail::FuncResult<FuncT>::type> Call(
  Process const& process, FnPtr address, CallConv call_conv, 
  Args&&... args)
{
  HADESMEM_DETAIL_STATIC_ASSERT(detail::FuncArity<FuncT>::value == 
    sizeof...(args));

  std::vector<CallArg> call_args;
  call_args.reserve(sizeof...(args));
  detail::BuildCallArgs<FuncT, 0>(std::back_inserter(call_args), args...);

  CallResultRaw const ret = Call(process, address, call_conv, 
    std::begin(call_args), std::end(call_args));
  typedef typename detail::FuncResult<FuncT>::type ResultT;
  return detail::CallResultRawToCallResult<ResultT>(ret);
}

#else // #if !defined(HADESMEM_DETAIL_NO_VARIADIC_TEMPLATES)

HADESMEM_DETAIL_STATIC_ASSERT(HADESMEM_CALL_MAX_ARGS < 
  BOOST_PP_LIMIT_REPEAT);

HADESMEM_DETAIL_STATIC_ASSERT(HADESMEM_CALL_MAX_ARGS < 
  BOOST_PP_LIMIT_ITERATION);

#define HADESMEM_DETAIL_CHECK_FUNC_ARITY(n) \
  HADESMEM_DETAIL_STATIC_ASSERT(detail::FuncArity<FuncT>::value == n)

#define HADESMEM_DETAIL_CALL_ADD_ARG(n) \
  detail::AddCallArg<FuncT, n>(std::back_inserter(args), \
    std::forward<T##n>(t##n))

#define HADESMEM_DETAIL_CALL_ADD_ARG_WRAPPER(z, n, unused) \
  HADESMEM_DETAIL_CALL_ADD_ARG(n);

#define BOOST_PP_LOCAL_MACRO(n) \
template <typename FuncT BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)>\
CallResult<typename detail::FuncResult<FuncT>::type>\
  Call(Process const& process, FnPtr address, CallConv call_conv \
  BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(n, T, && t))\
{\
  HADESMEM_DETAIL_CHECK_FUNC_ARITY(n);\
  std::vector<CallArg> args;\
  BOOST_PP_REPEAT(n, HADESMEM_DETAIL_CALL_ADD_ARG_WRAPPER, ~)\
  CallResultRaw const ret = Call(process, address, call_conv, \
    std::begin(args), std::end(args));\
  typedef typename detail::FuncResult<FuncT>::type ResultT;\
  return detail::CallResultRawToCallResult<ResultT>(ret);\
}\

#define BOOST_PP_LOCAL_LIMITS (0, HADESMEM_CALL_MAX_ARGS)

#if defined(HADESMEM_MSVC)
#pragma warning(push)
#pragma warning(disable: 4100)
#endif // #if defined(HADESMEM_MSVC)

#include BOOST_PP_LOCAL_ITERATE()

#if defined(HADESMEM_MSVC)
#pragma warning(pop)
#endif // #if defined(HADESMEM_MSVC)

#endif // #if !defined(HADESMEM_DETAIL_NO_VARIADIC_TEMPLATES)

class MultiCall
{
public:
  explicit MultiCall(Process const& process)
    : process_(&process), 
    addresses_(), 
    call_convs_(), 
    args_()
  { }

#if !defined(HADESMEM_DETAIL_NO_DEFAULTED_FUNCTIONS)

  MultiCall(MultiCall const&) HADESMEM_DETAIL_DEFAULTED_FUNCTION;

  MultiCall& operator=(MultiCall const&) HADESMEM_DETAIL_DEFAULTED_FUNCTION;

  MultiCall(MultiCall&&) HADESMEM_DETAIL_DEFAULTED_FUNCTION;

  MultiCall& operator=(MultiCall&&) HADESMEM_DETAIL_DEFAULTED_FUNCTION;

#else // #if !defined(HADESMEM_DETAIL_NO_DEFAULTED_FUNCTIONS)

  MultiCall(MultiCall const& other)
    : process_(other.process_), 
    addresses_(other.addresses_), 
    call_convs_(other.call_convs_), 
    args_(other.args_)
  { }

  MultiCall& operator=(MultiCall const& other)
  {
    MultiCall tmp(other);
    *this = std::move(tmp);

    return *this;
  }
  
  MultiCall(MultiCall&& other) HADESMEM_DETAIL_NOEXCEPT
    : process_(other.process_), 
    addresses_(std::move(other.addresses_)), 
    call_convs_(std::move(other.call_convs_)), 
    args_(std::move(other.args_))
  { }
  
  MultiCall& operator=(MultiCall&& other) HADESMEM_DETAIL_NOEXCEPT
  {
    process_ = other.process_;
    addresses_ = std::move(other.addresses_);
    call_convs_ = std::move(other.call_convs_);
    args_ = std::move(other.args_);

    return *this;
  }

#endif // #if !defined(HADESMEM_DETAIL_NO_DEFAULTED_FUNCTIONS)

#if !defined(HADESMEM_DETAIL_NO_VARIADIC_TEMPLATES)

  template <typename FuncT, typename... Args>
  void Add(FnPtr address, CallConv call_conv, Args&&... args)
  {
    HADESMEM_DETAIL_STATIC_ASSERT(detail::FuncArity<FuncT>::value == 
      sizeof...(args));

    std::vector<CallArg> call_args;
    call_args.reserve(sizeof...(args));
    detail::BuildCallArgs<FuncT, 0>(std::back_inserter(call_args), args...);

    addresses_.push_back(address);
    call_convs_.push_back(call_conv);
    args_.push_back(call_args);
  }

#else // #if !defined(HADESMEM_DETAIL_NO_VARIADIC_TEMPLATES)

#define BOOST_PP_LOCAL_MACRO(n) \
  template <typename FuncT BOOST_PP_ENUM_TRAILING_PARAMS(n, typename T)>\
  void Add(FnPtr address, CallConv call_conv \
    BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(n, T, && t))\
  {\
    HADESMEM_DETAIL_CHECK_FUNC_ARITY(n);\
    std::vector<CallArg> args;\
    BOOST_PP_REPEAT(n, HADESMEM_DETAIL_CALL_ADD_ARG_WRAPPER, ~)\
    addresses_.push_back(address);\
    call_convs_.push_back(call_conv);\
    args_.push_back(args);\
  }\

#define BOOST_PP_LOCAL_LIMITS (0, HADESMEM_CALL_MAX_ARGS)

#if defined(HADESMEM_MSVC)
#pragma warning(push)
#pragma warning(disable: 4100)
#endif // #if defined(HADESMEM_MSVC)

#include BOOST_PP_LOCAL_ITERATE()

#if defined(HADESMEM_MSVC)
#pragma warning(pop)
#endif // #if defined(HADESMEM_MSVC)

#undef HADESMEM_DETAIL_CHECK_FUNC_ARITY

#undef HADESMEM_DETAIL_CALL_ADD_ARG

#undef HADESMEM_DETAIL_CALL_ADD_ARG_WRAPPER

#endif // #if !defined(HADESMEM_DETAIL_NO_VARIADIC_TEMPLATES)
  
  template <typename OutputIterator>
  void Call(OutputIterator results) const
  {
    // TODO: Iterator checks for type and category.

    CallMulti(*process_, std::begin(addresses_), std::end(addresses_), 
      std::begin(call_convs_), std::begin(args_), results);
  }
  
private:
  Process const* process_;
  std::vector<FnPtr> addresses_; 
  std::vector<CallConv> call_convs_; 
  std::vector<std::vector<CallArg>> args_;
};

}
