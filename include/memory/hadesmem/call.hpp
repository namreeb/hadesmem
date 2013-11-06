// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

#include <windows.h>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <asmjit/asmjit.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/alloc.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/func_args.hpp>
#include <hadesmem/detail/func_arity.hpp>
#include <hadesmem/detail/func_result.hpp>
#include <hadesmem/detail/remote_thread.hpp>
#include <hadesmem/detail/smart_handle.hpp>
#include <hadesmem/detail/static_assert.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/detail/union_cast.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

// TODO: Cross-session injection (also cross-winsta and cross-desktop 
// injection). Easiest solution is to use a broker process via a service 
// and CreateProcessAsUser. Potentially 'better' solution would be to use 
// NtCreateThread/RtlCreateUserThread.

// TODO: Support using NtCreateThread/RtlCreateUserThread. Does not create an 
// activation context, so it will need special work done to get cases like 
// .NET working.

// TODO: Support injected code using only NT APIs (for native processes such 
// as smss.exe etc).

// TODO: Support injecting into uninitialized processes without forcing a 
// LdrInitializeThunk call, as this may cause execution of code which we 
// want to hook (among other things)!

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

  HADESMEM_DETAIL_CONSTEXPR explicit CallResult(
    T result, 
    DWORD last_error) HADESMEM_DETAIL_NOEXCEPT
    : result_(result), 
    last_error_(last_error)
  { }

  HADESMEM_DETAIL_CONSTEXPR T GetReturnValue() const 
    HADESMEM_DETAIL_NOEXCEPT
  {
    return result_;
  }

  HADESMEM_DETAIL_CONSTEXPR DWORD GetLastError() const 
    HADESMEM_DETAIL_NOEXCEPT
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
  HADESMEM_DETAIL_CONSTEXPR explicit CallResult(DWORD last_error) 
    HADESMEM_DETAIL_NOEXCEPT
    : last_error_(last_error)
  { }

  HADESMEM_DETAIL_CONSTEXPR DWORD GetLastError() const 
    HADESMEM_DETAIL_NOEXCEPT
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

// CallResultRemote must be POD because 'offsetof' requires a standard layout 
// type and 'malloc'/'memcpy'/etc requires a trivial type.
HADESMEM_DETAIL_STATIC_ASSERT(std::is_pod<detail::CallResultRemote>::value);

}

class CallResultRaw
{
public:
  explicit CallResultRaw(
    DWORD64 return_i64, 
    float return_float, 
    double return_double, 
    DWORD last_error) HADESMEM_DETAIL_NOEXCEPT
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
  // TODO: Make this constexpr.
  template <typename T>
  explicit CallArg(T t) HADESMEM_DETAIL_NOEXCEPT
    : arg_(), 
    type_(VariantType::kNone)
  {
    HADESMEM_DETAIL_STATIC_ASSERT(std::is_integral<T>::value || 
      std::is_pointer<T>::value || 
      std::is_same<float, typename std::remove_cv<T>::type>::value || 
      std::is_same<double, typename std::remove_cv<T>::type>::value);

    Initialize(t);
  }

  template <typename F>
  void Apply(F f) const
  {
    switch (type_)
    {
    case VariantType::kNone:
      HADESMEM_DETAIL_ASSERT(false);
      break;
    case VariantType::kInt32:
      f(arg_.i32);
      break;
    case VariantType::kInt64:
      f(arg_.i64);
      break;
    case VariantType::kFloat32:
      f(arg_.f32);
      break;
    case VariantType::kFloat64:
      f(arg_.f64);
      break;
    }
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
    arg_.i32 = t;
    type_ = VariantType::kInt32;
  }

  void Initialize(DWORD64 t) HADESMEM_DETAIL_NOEXCEPT
  {
    arg_.i64 = t;
    type_ = VariantType::kInt64;
  }

  void Initialize(float t) HADESMEM_DETAIL_NOEXCEPT
  {
    arg_.f32 = t;
    type_ = VariantType::kFloat32;
  }

  void Initialize(double t) HADESMEM_DETAIL_NOEXCEPT
  {
    arg_.f64 = t;
    type_ = VariantType::kFloat64;
  }

  enum class VariantType
  {
    kNone, 
    kInt32, 
    kInt64, 
    kFloat32, 
    kFloat64
  };

  union Variant
  {
    DWORD32 i32;
    DWORD64 i64;
    float f32;
    double f64;
  };
  Variant arg_;
  VariantType type_;
};

namespace detail
{
  
#if defined(HADESMEM_DETAIL_ARCH_X86)

class ArgVisitor32
{
public:
  ArgVisitor32(AsmJit::X86Assembler* assembler, 
    std::size_t num_args, 
    CallConv call_conv) HADESMEM_DETAIL_NOEXCEPT
    : assembler_(assembler), 
    cur_arg_(num_args), 
    call_conv_(call_conv)
  { }

  void operator()(DWORD32 arg) HADESMEM_DETAIL_NOEXCEPT
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

  void operator()(DWORD64 arg) HADESMEM_DETAIL_NOEXCEPT
  {
    assembler_->mov(AsmJit::eax, AsmJit::uimm(static_cast<DWORD>(
      (arg >> 32) & 0xFFFFFFFFUL)));
    assembler_->push(AsmJit::eax);

    assembler_->mov(AsmJit::eax, AsmJit::uimm(static_cast<DWORD>(
      arg & 0xFFFFFFFFUL)));
    assembler_->push(AsmJit::eax);

    --cur_arg_;
  }

  void operator()(float arg) HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_STATIC_ASSERT(sizeof(float) == 4);
    HADESMEM_DETAIL_STATIC_ASSERT(sizeof(float) == sizeof(DWORD));

    auto const arg_conv = UnionCast<DWORD>(arg);

    assembler_->mov(AsmJit::eax, AsmJit::uimm(arg_conv));
    assembler_->push(AsmJit::eax);

    --cur_arg_;
  }

  void operator()(double arg) HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_STATIC_ASSERT(sizeof(double) == 8);
    HADESMEM_DETAIL_STATIC_ASSERT(sizeof(double) == sizeof(DWORD64));

    auto const arg_conv = UnionCast<DWORD64>(arg);

    assembler_->mov(AsmJit::eax, AsmJit::uimm(static_cast<DWORD>(
      (arg_conv >> 32) & 0xFFFFFFFFUL)));
    assembler_->push(AsmJit::eax);

    assembler_->mov(AsmJit::eax, AsmJit::uimm(static_cast<DWORD>(
      arg_conv & 0xFFFFFFFFUL)));
    assembler_->push(AsmJit::eax);

    --cur_arg_;
  }
  
private:
  AsmJit::X86Assembler* assembler_;
  std::size_t cur_arg_;
  CallConv call_conv_;
};

#endif // #if defined(HADESMEM_DETAIL_ARCH_X86)

#if defined(HADESMEM_DETAIL_ARCH_X64)

class ArgVisitor64
{
public:
  ArgVisitor64(AsmJit::X86Assembler* assembler, 
    std::size_t num_args) HADESMEM_DETAIL_NOEXCEPT
    : assembler_(assembler), 
    num_args_(num_args), 
    cur_arg_(num_args)
  { }

  void operator()(DWORD32 arg) HADESMEM_DETAIL_NOEXCEPT
  {
    return (*this)(static_cast<DWORD64>(arg));
  }

  void operator()(DWORD64 arg) HADESMEM_DETAIL_NOEXCEPT
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

  void operator()(float arg) HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_STATIC_ASSERT(sizeof(float) == 4);
    HADESMEM_DETAIL_STATIC_ASSERT(sizeof(float) == sizeof(DWORD));

    auto const arg_conv = UnionCast<DWORD>(arg);

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

  void operator()(double arg) HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_STATIC_ASSERT(sizeof(double) == 8);
    HADESMEM_DETAIL_STATIC_ASSERT(sizeof(double) == sizeof(DWORD64));

    auto const arg_conv = UnionCast<DWORD64>(arg);

    DWORD const arg_low = static_cast<DWORD>(arg_conv & 0xFFFFFFFFUL);
    DWORD const arg_high = static_cast<DWORD>(
      (arg_conv >> 32) & 0xFFFFFFFFUL);

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
    FnPtr const address = *addresses_beg;
    CallConv const call_conv = *call_convs_beg;
    auto const& args = *args_full_beg;
    std::size_t const num_args = args.size();

    ArgVisitor32 arg_visitor(assembler, num_args, call_conv);
    std::for_each(args.rbegin(), args.rend(), 
      [&] (CallArg const& arg)
    {
      arg.Apply(std::ref(arg_visitor));
    });

    assembler->mov(AsmJit::eax, reinterpret_cast<sysint_t>(address));
    assembler->call(AsmJit::eax);

    DWORD_PTR const current_return_value_remote = 
      reinterpret_cast<DWORD_PTR>(return_values_remote) + 
      i * sizeof(detail::CallResultRemote);

    assembler->mov(AsmJit::ecx, AsmJit::uimm(current_return_value_remote + 
      offsetof(detail::CallResultRemote, return_i64)));
    assembler->mov(AsmJit::dword_ptr(AsmJit::ecx), AsmJit::eax);
    assembler->mov(AsmJit::dword_ptr(AsmJit::ecx, 4), AsmJit::edx);

    assembler->mov(AsmJit::ecx, AsmJit::uimm(current_return_value_remote + 
      offsetof(detail::CallResultRemote, return_float)));
    assembler->fst(AsmJit::dword_ptr(AsmJit::ecx));

    assembler->mov(AsmJit::ecx, AsmJit::uimm(current_return_value_remote + 
      offsetof(detail::CallResultRemote, return_double)));
    assembler->fst(AsmJit::qword_ptr(AsmJit::ecx));

    assembler->mov(AsmJit::eax, AsmJit::uimm(get_last_error));
    assembler->call(AsmJit::eax);
    
    assembler->mov(AsmJit::ecx, AsmJit::uimm(current_return_value_remote + 
      offsetof(detail::CallResultRemote, last_error)));
    assembler->mov(AsmJit::dword_ptr(AsmJit::ecx), AsmJit::eax);

    if (call_conv == CallConv::kDefault || call_conv == CallConv::kCdecl)
    {
      assembler->add(AsmJit::esp, AsmJit::uimm(num_args * sizeof(void*)));
    }
  }

  assembler->mov(AsmJit::esp, AsmJit::ebp);
  assembler->pop(AsmJit::ebp);

  assembler->ret(0x4);
}

#endif // #if defined(HADESMEM_DETAIL_ARCH_X86)

#if defined(HADESMEM_DETAIL_ARCH_X64)

struct ContainerSizeComparer
{
  template <typename C1, typename C2>
  bool operator()(C1 const& lhs, C2 const& rhs)
  {
    return lhs.size() < rhs.size();
  }
};

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
  auto const max_args_list = std::max_element(args_full_beg, 
    args_full_beg + num_addresses, ContainerSizeComparer());
  std::size_t const max_num_args = max_args_list->size();

  std::size_t const stack_offset = 
    [&]()
    {
      // Minimum 0x20 bytes of ghost space for spilling args.
      std::size_t const ghost_size = 0x20UL;
      std::size_t stack_offs_tmp = (std::max)(ghost_size, max_num_args * 0x8);
      // Add scratch space for use when converting args etc.
      stack_offs_tmp += 16;
      // Align the stack for the return address.
      stack_offs_tmp += (stack_offs_tmp % 16) ? 0 : 8;
      return stack_offs_tmp;
    }();

  assembler->sub(AsmJit::rsp, stack_offset);

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
    FnPtr const address = *addresses_beg;
    auto const& args = *args_full_beg;
    std::size_t const num_args = args.size();

    HADESMEM_DETAIL_ASSERT(*call_convs_beg == CallConv::kDefault || 
      *call_convs_beg == CallConv::kWinApi || 
      *call_convs_beg == CallConv::kX64);

    ArgVisitor64 arg_visitor(assembler, num_args);
    std::for_each(args.rbegin(), args.rend(), 
      [&] (CallArg const& arg)
    {
      arg.Apply(std::ref(arg_visitor));
    });

    assembler->mov(AsmJit::rax, reinterpret_cast<DWORD_PTR>(address));
    assembler->call(AsmJit::rax);

    DWORD_PTR const current_return_value_remote = 
      reinterpret_cast<DWORD_PTR>(return_values_remote) + 
      i * sizeof(detail::CallResultRemote);
    
    assembler->mov(AsmJit::rcx, current_return_value_remote + 
      offsetof(detail::CallResultRemote, return_i64));
    assembler->mov(AsmJit::qword_ptr(AsmJit::rcx), AsmJit::rax);

    assembler->mov(AsmJit::rcx, current_return_value_remote + 
      offsetof(detail::CallResultRemote, return_float));
    assembler->movss(AsmJit::dword_ptr(AsmJit::rcx), AsmJit::xmm0);

    assembler->mov(AsmJit::rcx, current_return_value_remote + 
      offsetof(detail::CallResultRemote, return_double));
    assembler->movsd(AsmJit::qword_ptr(AsmJit::rcx), AsmJit::xmm0);

    assembler->mov(AsmJit::rax, get_last_error);
    assembler->call(AsmJit::rax);

    assembler->mov(AsmJit::rcx, current_return_value_remote + 
      offsetof(detail::CallResultRemote, last_error));
    assembler->mov(AsmJit::dword_ptr(AsmJit::rcx), AsmJit::eax);
  }

  assembler->add(AsmJit::rsp, stack_offset);

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

}

// TODO: Remove dependency on ArgsForwardIterator being an iterator with 
// value_type of std::vector<CallArg> (or rather, any container supporting 
// size() and rbegin()/rend(), but there's no good reason to use anything but 
// vector even if it's technically supported).
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
  HADESMEM_DETAIL_STATIC_ASSERT(std::is_base_of<std::forward_iterator_tag, 
    typename std::iterator_traits<AddressesForwardIterator>::
    iterator_category>::value);
  HADESMEM_DETAIL_STATIC_ASSERT(std::is_base_of<std::forward_iterator_tag, 
    typename std::iterator_traits<ConvForwardIterator>::
    iterator_category>::value);
  HADESMEM_DETAIL_STATIC_ASSERT(std::is_base_of<std::forward_iterator_tag, 
    typename std::iterator_traits<ArgsForwardIterator>::
    iterator_category>::value);
  HADESMEM_DETAIL_STATIC_ASSERT(std::is_base_of<std::output_iterator_tag, 
    typename std::iterator_traits<ResultsOutputIterator>::
    iterator_category>::value);
  
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

  std::vector<detail::CallResultRemote> const return_vals_remote = 
    ReadVector<detail::CallResultRemote>(process, 
    return_values_remote.GetBase(), num_addresses);
  
  std::vector<CallResultRaw> return_vals;
  return_vals.reserve(num_addresses);
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

template <typename FuncT, std::int32_t N, typename T, typename OutputIterator>
void AddCallArg(OutputIterator call_args, T&& arg)
{
  typedef typename detail::FuncArgs<FuncT>::type FuncArgs;
  typedef typename std::tuple_element<N, FuncArgs>::type RealT;
  HADESMEM_DETAIL_STATIC_ASSERT(std::is_convertible<T, RealT>::value);
  *call_args = static_cast<CallArg>(static_cast<RealT>(std::forward<T>(arg)));
}

}

namespace detail
{

template <typename FuncT, std::int32_t N, typename OutputIterator>
void BuildCallArgs(OutputIterator /*call_args*/) HADESMEM_DETAIL_NOEXCEPT
{
  return;
}

template <typename FuncT, std::int32_t N, typename T, typename OutputIterator, 
  typename... Args>
void BuildCallArgs(OutputIterator call_args, T&& arg, Args&&... args)
{
  AddCallArg<FuncT, N>(call_args, std::forward<T>(arg));
  return BuildCallArgs<FuncT, N + 1>(
    ++call_args, 
    std::forward<Args>(args)...);
}

}

// TODO: Support decltype(&SomeFunc) and decltype(SomeFunc) as args to FuncT.
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

class MultiCall
{
public:
  explicit MultiCall(Process const& process)
    : process_(&process), 
    addresses_(), 
    call_convs_(), 
    args_()
  { }

  MultiCall(MultiCall const&) = default;

  MultiCall& operator=(MultiCall const&) = default;

#if !defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

  MultiCall(MultiCall&&) = default;

  MultiCall& operator=(MultiCall&&) = default;

#else // #if !defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

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

#endif // #if !defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

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

  template <typename OutputIterator>
  void Call(OutputIterator results) const
  {
    HADESMEM_DETAIL_STATIC_ASSERT(std::is_base_of<std::output_iterator_tag, 
      typename std::iterator_traits<OutputIterator>::iterator_category>::
      value);

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
