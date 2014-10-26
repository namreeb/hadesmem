// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <tuple>
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
#include <hadesmem/detail/remote_thread.hpp>
#include <hadesmem/detail/smart_handle.hpp>
#include <hadesmem/detail/static_assert.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/detail/type_traits.hpp>
#include <hadesmem/detail/union_cast.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/flush.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

namespace hadesmem
{

HADESMEM_DETAIL_STATIC_ASSERT(sizeof(void (*)()) == sizeof(void*));

enum class CallConv
{
  kDefault,
  kCdecl,
  kStdCall,
  kThisCall,
  kFastCall,
  kX64
};

template <typename T> class CallResult
{
public:
  HADESMEM_DETAIL_STATIC_ASSERT(
    std::is_integral<T>::value || std::is_pointer<T>::value ||
    std::is_same<float, std::remove_cv_t<T>>::value ||
    std::is_same<double, std::remove_cv_t<T>>::value);

  HADESMEM_DETAIL_CONSTEXPR explicit CallResult(T result, DWORD last_error)
    HADESMEM_DETAIL_NOEXCEPT : result_{result},
                               last_error_{last_error}
  {
  }

  HADESMEM_DETAIL_CONSTEXPR T GetReturnValue() const HADESMEM_DETAIL_NOEXCEPT
  {
    return result_;
  }

  HADESMEM_DETAIL_CONSTEXPR DWORD GetLastError() const HADESMEM_DETAIL_NOEXCEPT
  {
    return last_error_;
  }

private:
  T result_;
  DWORD last_error_;
};

template <> class CallResult<void>
{
public:
  HADESMEM_DETAIL_CONSTEXPR explicit CallResult(DWORD last_error)
    HADESMEM_DETAIL_NOEXCEPT : last_error_{last_error}
  {
  }

  HADESMEM_DETAIL_CONSTEXPR DWORD GetLastError() const HADESMEM_DETAIL_NOEXCEPT
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

// CallResultRemote must be POD because 'offsetof' requires a
// standard layout type and 'malloc'/'memcpy'/etc requires a trivial
// type.
HADESMEM_DETAIL_STATIC_ASSERT(std::is_pod<detail::CallResultRemote>::value);
}

class CallResultRaw
{
public:
  explicit CallResultRaw(DWORD64 return_i64,
                         float return_float,
                         double return_double,
                         DWORD last_error) HADESMEM_DETAIL_NOEXCEPT
    : result_(detail::CallResultRemote{
        return_i64, return_float, return_double, last_error})
  {
  }

  explicit CallResultRaw(detail::CallResultRemote const& result)
    HADESMEM_DETAIL_NOEXCEPT : result_(result)
  {
  }

  DWORD GetLastError() const HADESMEM_DETAIL_NOEXCEPT
  {
    return result_.last_error;
  }

  template <typename T>
  std::decay_t<T> GetReturnValue() const HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_STATIC_ASSERT(std::is_integral<T>::value ||
                                  std::is_pointer<T>::value ||
                                  std::is_same<float, std::decay_t<T>>::value ||
                                  std::is_same<double, std::decay_t<T>>::value);

    using U = std::decay_t<T>;
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
  T GetReturnValueIntOrFloatImpl(std::true_type,
                                 std::false_type,
                                 std::false_type) const HADESMEM_DETAIL_NOEXCEPT
  {
    return GetReturnValueFloat();
  }

  template <typename T>
  T GetReturnValueIntOrFloatImpl(std::false_type,
                                 std::true_type,
                                 std::false_type) const HADESMEM_DETAIL_NOEXCEPT
  {
    return GetReturnValueDouble();
  }

  template <typename T>
  T GetReturnValueIntOrFloatImpl(std::false_type,
                                 std::false_type,
                                 std::true_type) const HADESMEM_DETAIL_NOEXCEPT
  {
    return GetReturnValueIntImpl<T>(
      std::integral_constant<bool, (sizeof(T) == sizeof(DWORD64))>());
  }

  template <typename T>
  T GetReturnValueImpl(std::true_type) const HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_STATIC_ASSERT(std::is_pointer<T>::value);
    return GetReturnValuePtrImpl<T>(
      std::integral_constant<bool, (sizeof(void*) == sizeof(DWORD64))>());
  }

  template <typename T>
  T GetReturnValueImpl(std::false_type) const HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_STATIC_ASSERT(std::is_integral<T>::value ||
                                  std::is_floating_point<T>::value);
    return GetReturnValueIntOrFloatImpl<T>(std::is_same<T, float>(),
                                           std::is_same<T, double>(),
                                           std::is_integral<T>());
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
  template <typename T> explicit CallArg(T t) HADESMEM_DETAIL_NOEXCEPT
  {
    using U = std::remove_cv_t<T>;
    HADESMEM_DETAIL_STATIC_ASSERT(
      std::is_integral<T>::value || std::is_pointer<T>::value ||
      std::is_same<float, U>::value || std::is_same<double, U>::value);

    Initialize(t);
  }

  template <typename F> void Apply(F f) const
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
  template <typename T> void Initialize(T t) HADESMEM_DETAIL_NOEXCEPT
  {
    using D = typename std::conditional<sizeof(T) == sizeof(DWORD64),
                                        DWORD64,
                                        DWORD32>::type;
    Initialize(static_cast<D>(t));
  }

  template <typename T> void Initialize(T const* t) HADESMEM_DETAIL_NOEXCEPT
  {
    using D = typename std::conditional<sizeof(T const*) == sizeof(DWORD64),
                                        DWORD64,
                                        DWORD32>::type;
    Initialize(reinterpret_cast<D>(t));
  }

  template <typename T> void Initialize(T* t) HADESMEM_DETAIL_NOEXCEPT
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

inline std::uint32_t GetLow32(std::uint64_t i)
{
  return static_cast<std::uint32_t>(i & 0xFFFFFFFFUL);
}

inline std::uint32_t GetHigh32(std::uint64_t i)
{
  return static_cast<std::uint32_t>((i >> 32) & 0xFFFFFFFFUL);
}

#if defined(HADESMEM_GCC)
#pragma GCC visibility push(hidden)
#endif // #if defined(HADESMEM_GCC)

class ArgVisitor32
{
public:
  ArgVisitor32(asmjit::x86::Assembler* assembler,
               std::size_t num_args,
               CallConv call_conv) HADESMEM_DETAIL_NOEXCEPT
    : assembler_{assembler},
      cur_arg_{num_args},
      call_conv_{call_conv}
  {
  }

  void operator()(std::uint32_t arg) HADESMEM_DETAIL_NOEXCEPT
  {
    asmjit::x86::GpReg const regs[] = {asmjit::x86::ecx, asmjit::x86::edx};
    auto const num_reg_args =
      (call_conv_ == CallConv::kThisCall || call_conv_ == CallConv::kFastCall)
        ? ((call_conv_ == CallConv::kThisCall) ? 1UL : 2UL)
        : 0UL;
    if (cur_arg_ > 0 && cur_arg_ <= num_reg_args)
    {
      assembler_->mov(regs[cur_arg_ - 1], asmjit::imm_u(arg));
    }
    else
    {
      assembler_->mov(asmjit::x86::eax, asmjit::imm_u(arg));
      assembler_->push(asmjit::x86::eax);
    }

    --cur_arg_;
  }

  void operator()(std::uint64_t arg) HADESMEM_DETAIL_NOEXCEPT
  {
    assembler_->mov(asmjit::x86::eax, asmjit::imm_u(GetHigh32(arg)));
    assembler_->push(asmjit::x86::eax);

    assembler_->mov(asmjit::x86::eax, asmjit::imm_u(GetLow32(arg)));
    assembler_->push(asmjit::x86::eax);

    --cur_arg_;
  }

  void operator()(float arg) HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_STATIC_ASSERT(sizeof(float) == 4);
    HADESMEM_DETAIL_STATIC_ASSERT(sizeof(float) == sizeof(std::uint32_t));

    auto const arg_conv = UnionCast<std::uint32_t>(arg);

    assembler_->mov(asmjit::x86::eax, asmjit::imm_u(arg_conv));
    assembler_->push(asmjit::x86::eax);

    --cur_arg_;
  }

  void operator()(double arg) HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_STATIC_ASSERT(sizeof(double) == 8);
    HADESMEM_DETAIL_STATIC_ASSERT(sizeof(double) == sizeof(std::uint64_t));

    auto const arg_conv = UnionCast<std::uint64_t>(arg);

    assembler_->mov(asmjit::x86::eax, asmjit::imm_u(GetHigh32(arg_conv)));
    assembler_->push(asmjit::x86::eax);

    assembler_->mov(asmjit::x86::eax, asmjit::imm_u(GetLow32(arg_conv)));
    assembler_->push(asmjit::x86::eax);

    --cur_arg_;
  }

private:
  asmjit::x86::Assembler* assembler_;
  std::size_t cur_arg_;
  CallConv call_conv_;
};

class ArgVisitor64
{
public:
  ArgVisitor64(asmjit::x64::Assembler* assembler,
               std::size_t num_args) HADESMEM_DETAIL_NOEXCEPT
    : assembler_{assembler},
      num_args_{num_args},
      cur_arg_{num_args}
  {
  }

  void operator()(std::uint32_t arg) HADESMEM_DETAIL_NOEXCEPT
  {
    return (*this)(static_cast<std::uint64_t>(arg));
  }

  void operator()(std::uint64_t arg) HADESMEM_DETAIL_NOEXCEPT
  {
    std::int32_t const stack_offs =
      static_cast<std::int32_t>((cur_arg_ - 1) * 8);

    if (cur_arg_ > 0 && cur_arg_ <= 4)
    {
      asmjit::x64::GpReg const regs[] = {
        asmjit::x64::rcx, asmjit::x64::rdx, asmjit::x64::r8, asmjit::x64::r9};
      assembler_->mov(regs[cur_arg_ - 1], asmjit::imm_u(arg));
    }
    else
    {
      assembler_->mov(asmjit::x64::dword_ptr(asmjit::x64::rsp, stack_offs),
                      asmjit::imm_u(GetLow32(arg)));
      assembler_->mov(asmjit::x64::dword_ptr(asmjit::x64::rsp, stack_offs + 4),
                      asmjit::imm_u(GetHigh32(arg)));
    }

    --cur_arg_;
  }

  void operator()(float arg) HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_STATIC_ASSERT(sizeof(float) == 4);
    HADESMEM_DETAIL_STATIC_ASSERT(sizeof(float) == sizeof(std::uint32_t));

    auto const arg_conv = UnionCast<std::uint32_t>(arg);

    std::int32_t const scratch_offs = static_cast<std::int32_t>(num_args_ * 8);
    std::int32_t const stack_offs =
      static_cast<std::int32_t>((cur_arg_ - 1) * 8);

    if (cur_arg_ > 0 && cur_arg_ <= 4)
    {
      asmjit::x64::XmmReg const regs[] = {asmjit::x64::xmm0,
                                          asmjit::x64::xmm1,
                                          asmjit::x64::xmm2,
                                          asmjit::x64::xmm3};
      assembler_->mov(asmjit::x64::dword_ptr(asmjit::x64::rsp, scratch_offs),
                      asmjit::imm_u(arg_conv));
      assembler_->movss(regs[cur_arg_ - 1],
                        asmjit::x64::dword_ptr(asmjit::x64::rsp, scratch_offs));
    }
    else
    {
      assembler_->mov(asmjit::x64::dword_ptr(asmjit::x64::rsp, stack_offs),
                      asmjit::imm_u(arg_conv));
    }

    --cur_arg_;
  }

  void operator()(double arg) HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_STATIC_ASSERT(sizeof(double) == 8);
    HADESMEM_DETAIL_STATIC_ASSERT(sizeof(double) == sizeof(std::uint64_t));

    auto const arg_conv = UnionCast<std::uint64_t>(arg);

    std::int32_t const scratch_offs = static_cast<std::int32_t>(num_args_ * 8);
    std::int32_t const stack_offs =
      static_cast<std::int32_t>((cur_arg_ - 1) * 8);

    if (cur_arg_ > 0 && cur_arg_ <= 4)
    {
      assembler_->mov(asmjit::x64::dword_ptr(asmjit::x64::rsp, scratch_offs),
                      asmjit::imm_u(GetLow32(arg_conv)));
      assembler_->mov(
        asmjit::x64::dword_ptr(asmjit::x64::rsp, scratch_offs + 4),
        asmjit::imm_u(GetHigh32(arg_conv)));
      asmjit::x64::XmmReg const regs[] = {asmjit::x64::xmm0,
                                          asmjit::x64::xmm1,
                                          asmjit::x64::xmm2,
                                          asmjit::x64::xmm3};
      assembler_->movsd(regs[cur_arg_ - 1],
                        asmjit::x64::qword_ptr(asmjit::x64::rsp, scratch_offs));
    }
    else
    {
      assembler_->mov(asmjit::x64::dword_ptr(asmjit::x64::rsp, stack_offs),
                      asmjit::imm_u(GetLow32(arg_conv)));
      assembler_->mov(asmjit::x64::dword_ptr(asmjit::x64::rsp, stack_offs + 4),
                      asmjit::imm_u(GetHigh32(arg_conv)));
    }

    --cur_arg_;
  }

private:
  asmjit::x64::Assembler* assembler_;
  std::size_t num_args_;
  std::size_t cur_arg_;
};

#if defined(HADESMEM_GCC)
#pragma GCC visibility pop
#endif // #if defined(HADESMEM_GCC)

template <typename AddressesForwardIterator,
          typename ConvForwardIterator,
          typename ArgsForwardIterator>
inline void GenerateCallCode32(asmjit::x86::Assembler* assembler,
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

  asmjit::Label label_nodebug(assembler->newLabel());

  assembler->push(asmjit::x86::ebp);
  assembler->mov(asmjit::x86::ebp, asmjit::x86::esp);

  assembler->mov(asmjit::x86::eax, asmjit::imm_u(is_debugger_present));
  assembler->call(asmjit::x86::eax);

  assembler->test(asmjit::x86::eax, asmjit::x86::eax);
  assembler->jz(label_nodebug);

  assembler->mov(asmjit::x86::eax, asmjit::imm_u(debug_break));
  assembler->call(asmjit::x86::eax);

  assembler->bind(label_nodebug);

  assembler->push(0x0);
  assembler->mov(asmjit::x86::eax, asmjit::imm_u(set_last_error));
  assembler->call(asmjit::x86::eax);

  for (std::size_t i = 0; addresses_beg != addresses_end;
       ++addresses_beg, ++call_convs_beg, ++args_full_beg, ++i)
  {
    void* const address = *addresses_beg;
    CallConv const call_conv = *call_convs_beg;
    auto const& args = *args_full_beg;
    std::size_t const num_args = args.size();

    ArgVisitor32 arg_visitor{assembler, num_args, call_conv};
    std::for_each(args.rbegin(),
                  args.rend(),
                  [&](CallArg const& arg)
                  {
      arg.Apply(std::ref(arg_visitor));
    });

    assembler->mov(asmjit::x86::eax,
                   asmjit::imm_u(reinterpret_cast<std::uintptr_t>(address)));
    assembler->call(asmjit::x86::eax);

    DWORD_PTR const current_return_value_remote =
      reinterpret_cast<DWORD_PTR>(return_values_remote) +
      i * sizeof(detail::CallResultRemote);

    assembler->mov(
      asmjit::x86::ecx,
      asmjit::imm_u(current_return_value_remote +
                    offsetof(detail::CallResultRemote, return_i64)));
    assembler->mov(asmjit::x86::dword_ptr(asmjit::x86::ecx), asmjit::x86::eax);
    assembler->mov(asmjit::x86::dword_ptr(asmjit::x86::ecx, 4),
                   asmjit::x86::edx);

    assembler->mov(
      asmjit::x86::ecx,
      asmjit::imm_u(current_return_value_remote +
                    offsetof(detail::CallResultRemote, return_float)));
    assembler->fst(asmjit::x86::dword_ptr(asmjit::x86::ecx));

    assembler->mov(
      asmjit::x86::ecx,
      asmjit::imm_u(current_return_value_remote +
                    offsetof(detail::CallResultRemote, return_double)));
    assembler->fst(asmjit::x86::qword_ptr(asmjit::x86::ecx));

    assembler->mov(asmjit::x86::eax, asmjit::imm_u(get_last_error));
    assembler->call(asmjit::x86::eax);

    assembler->mov(
      asmjit::x86::ecx,
      asmjit::imm_u(current_return_value_remote +
                    offsetof(detail::CallResultRemote, last_error)));
    assembler->mov(asmjit::x86::dword_ptr(asmjit::x86::ecx), asmjit::x86::eax);

    if (call_conv == CallConv::kDefault || call_conv == CallConv::kCdecl)
    {
      assembler->add(asmjit::x86::esp, asmjit::imm_u(num_args * sizeof(void*)));
    }
  }

  assembler->mov(asmjit::x86::esp, asmjit::x86::ebp);
  assembler->pop(asmjit::x86::ebp);

  assembler->ret(0x4);
}

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
inline void GenerateCallCode64(asmjit::x64::Assembler* assembler,
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

  asmjit::Label label_nodebug(assembler->newLabel());

  std::size_t const num_addresses = std::distance(addresses_beg, addresses_end);
  auto const max_args_list = std::max_element(
    args_full_beg, args_full_beg + num_addresses, ContainerSizeComparer());
  std::size_t const max_num_args = max_args_list->size();

  std::size_t const stack_offset = [&]()
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

  assembler->sub(asmjit::x64::rsp, asmjit::imm_u(stack_offset));

  assembler->mov(asmjit::x64::rax, asmjit::imm_u(is_debugger_present));
  assembler->call(asmjit::x64::rax);

  assembler->test(asmjit::x64::rax, asmjit::x64::rax);
  assembler->jz(label_nodebug);

  assembler->mov(asmjit::x64::rax, asmjit::imm_u(debug_break));
  assembler->call(asmjit::x64::rax);

  assembler->bind(label_nodebug);

  assembler->mov(asmjit::x64::rcx, 0);
  assembler->mov(asmjit::x64::rax, asmjit::imm_u(set_last_error));
  assembler->call(asmjit::x64::rax);

  for (std::size_t i = 0; addresses_beg != addresses_end;
       ++addresses_beg, ++call_convs_beg, ++args_full_beg, ++i)
  {
    void* const address = *addresses_beg;
    auto const& args = *args_full_beg;
    std::size_t const num_args = args.size();

    ArgVisitor64 arg_visitor{assembler, num_args};
    std::for_each(args.rbegin(),
                  args.rend(),
                  [&](CallArg const& arg)
                  {
      arg.Apply(std::ref(arg_visitor));
    });

    assembler->mov(asmjit::x64::rax,
                   asmjit::imm_u(reinterpret_cast<DWORD_PTR>(address)));
    assembler->call(asmjit::x64::rax);

    DWORD_PTR const current_return_value_remote =
      reinterpret_cast<DWORD_PTR>(return_values_remote) +
      i * sizeof(detail::CallResultRemote);

    assembler->mov(
      asmjit::x64::rcx,
      asmjit::imm_u(current_return_value_remote +
                    offsetof(detail::CallResultRemote, return_i64)));
    assembler->mov(asmjit::x64::qword_ptr(asmjit::x64::rcx), asmjit::x64::rax);

    assembler->mov(
      asmjit::x64::rcx,
      asmjit::imm_u(current_return_value_remote +
                    offsetof(detail::CallResultRemote, return_float)));
    assembler->movss(asmjit::x64::dword_ptr(asmjit::x64::rcx),
                     asmjit::x64::xmm0);

    assembler->mov(
      asmjit::x64::rcx,
      asmjit::imm_u(current_return_value_remote +
                    offsetof(detail::CallResultRemote, return_double)));
    assembler->movsd(asmjit::x64::qword_ptr(asmjit::x64::rcx),
                     asmjit::x64::xmm0);

    assembler->mov(asmjit::x64::rax, asmjit::imm_u(get_last_error));
    assembler->call(asmjit::x64::rax);

    assembler->mov(
      asmjit::x64::rcx,
      asmjit::imm_u(current_return_value_remote +
                    offsetof(detail::CallResultRemote, last_error)));
    assembler->mov(asmjit::x64::dword_ptr(asmjit::x64::rcx), asmjit::x64::eax);
  }

  assembler->add(asmjit::x64::rsp, asmjit::imm_u(stack_offset));

  assembler->ret();
}

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

  Module const kernel32{process, L"kernel32.dll"};
  auto const get_last_error = reinterpret_cast<DWORD_PTR>(
    FindProcedure(process, kernel32, "GetLastError"));
  auto const set_last_error = reinterpret_cast<DWORD_PTR>(
    FindProcedure(process, kernel32, "SetLastError"));
  auto const is_debugger_present = reinterpret_cast<DWORD_PTR>(
    FindProcedure(process, kernel32, "IsDebuggerPresent"));
  auto const debug_break =
    reinterpret_cast<DWORD_PTR>(FindProcedure(process, kernel32, "DebugBreak"));

  asmjit::JitRuntime runtime;
#if defined(HADESMEM_DETAIL_ARCH_X64)
  asmjit::x64::Assembler assembler{&runtime};
  GenerateCallCode64(
#elif defined(HADESMEM_DETAIL_ARCH_X86)
  asmjit::x86::Assembler assembler{&runtime};
  GenerateCallCode32(
#else
#error "[HadesMem] Unsupported architecture."
#endif
    &assembler,
    addresses_beg,
    addresses_end,
    call_convs_beg,
    args_full_beg,
    get_last_error,
    set_last_error,
    is_debugger_present,
    debug_break,
    return_values_remote);

  DWORD_PTR const stub_size = assembler.getCodeSize();

  HADESMEM_DETAIL_TRACE_A("Allocating memory for remote stub.");

  Allocator stub_mem_remote{process, stub_size};

  HADESMEM_DETAIL_TRACE_A("Performing code relocation.");

  std::vector<BYTE> code_real(stub_size);
  assembler.relocCode(code_real.data(),
                      reinterpret_cast<DWORD_PTR>(stub_mem_remote.GetBase()));

  HADESMEM_DETAIL_TRACE_A("Writing remote code stub.");

  WriteVector(process, stub_mem_remote.GetBase(), code_real);

  FlushInstructionCache(process, stub_mem_remote.GetBase(), stub_size);

  return stub_mem_remote;
}
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
  using AddressesForwardIteratorCategory =
    typename std::iterator_traits<AddressesForwardIterator>::iterator_category;
  HADESMEM_DETAIL_STATIC_ASSERT(
    std::is_base_of<std::forward_iterator_tag,
                    AddressesForwardIteratorCategory>::value);
  using ConvForwardIteratorCategory =
    typename std::iterator_traits<ConvForwardIterator>::iterator_category;
  HADESMEM_DETAIL_STATIC_ASSERT(
    std::is_base_of<std::forward_iterator_tag,
                    ConvForwardIteratorCategory>::value);
  using ArgsForwardIteratorCategory =
    typename std::iterator_traits<ArgsForwardIterator>::iterator_category;
  HADESMEM_DETAIL_STATIC_ASSERT(
    std::is_base_of<std::forward_iterator_tag,
                    ArgsForwardIteratorCategory>::value);
  using ResultsOutputIteratorCategory =
    typename std::iterator_traits<ResultsOutputIterator>::iterator_category;
  HADESMEM_DETAIL_STATIC_ASSERT(
    std::is_base_of<std::output_iterator_tag,
                    ResultsOutputIteratorCategory>::value);

  HADESMEM_DETAIL_TRACE_A("CallMulti called.");

  auto const num_addresses_signed = std::distance(addresses_beg, addresses_end);
  HADESMEM_DETAIL_ASSERT(num_addresses_signed > 0);
  using NumAddressesUnsigned =
    std::make_unsigned_t<decltype(num_addresses_signed)>;
  auto const num_addresses =
    static_cast<NumAddressesUnsigned>(num_addresses_signed);

  HADESMEM_DETAIL_TRACE_A("Allocating memory for return values.");

  Allocator const return_values_remote{
    process, sizeof(detail::CallResultRemote) * num_addresses};

  HADESMEM_DETAIL_TRACE_A("Allocating memory for code stub.");

  Allocator const code_remote{
    detail::GenerateCallCode(process,
                             addresses_beg,
                             addresses_end,
                             call_convs_beg,
                             args_full_beg,
                             return_values_remote.GetBase())};
  LPTHREAD_START_ROUTINE code_remote_pfn =
    reinterpret_cast<LPTHREAD_START_ROUTINE>(
      reinterpret_cast<DWORD_PTR>(code_remote.GetBase()));

  HADESMEM_DETAIL_TRACE_A("Creating remote thread and waiting.");

  detail::CreateRemoteThreadAndWait(process, code_remote_pfn);

  HADESMEM_DETAIL_TRACE_A("Reading return values.");

  std::vector<detail::CallResultRemote> const return_vals_remote =
    ReadVector<detail::CallResultRemote>(
      process, return_values_remote.GetBase(), num_addresses);

  std::vector<CallResultRaw> return_vals;
  return_vals.reserve(num_addresses);
  std::transform(std::begin(return_vals_remote),
                 std::end(return_vals_remote),
                 results,
                 [](detail::CallResultRemote const& r)
                 {
    return static_cast<CallResultRaw>(r);
  });
}

template <typename ArgsForwardIterator>
inline CallResultRaw CallRaw(Process const& process,
                             void* address,
                             CallConv call_conv,
                             ArgsForwardIterator args_beg,
                             ArgsForwardIterator args_end)
{
  std::vector<void*> addresses{address};
  std::vector<CallConv> call_convs{call_conv};
  std::vector<std::vector<CallArg>> args_full;
  // Using an initializer list for this causes an ICE under Intel C++
  // (14.0.1.139 Build 20131008).
  args_full.emplace_back(args_beg, args_end);
  std::vector<CallResultRaw> results;
  CallMulti(process,
            std::begin(addresses),
            std::end(addresses),
            std::begin(call_convs),
            std::begin(args_full),
            std::back_inserter(results));
  HADESMEM_DETAIL_ASSERT(results.size() == 1);
  return results.front();
}

namespace detail
{

template <typename FuncT,
          std::int32_t N,
          typename T,
          typename OutputIterator,
          int DummyCallConv = detail::FuncCallConv<FuncT>::value>
inline void AddCallArg(OutputIterator call_args, T&& arg)
{
  using RealT = typename std::tuple_element<N, FuncArgsT<FuncT>>::type;
  // Reference types are currently unsupported, just use a pointer instead.
  HADESMEM_DETAIL_STATIC_ASSERT(!std::is_reference<RealT>::value);
  HADESMEM_DETAIL_STATIC_ASSERT(std::is_convertible<T, RealT>::value);
  *call_args = static_cast<CallArg>(static_cast<RealT>(std::forward<T>(arg)));
}

template <typename FuncT,
          std::int32_t N,
          typename OutputIterator,
          int DummyCallConv = detail::FuncCallConv<FuncT>::value>
inline void BuildCallArgs(OutputIterator /*call_args*/) HADESMEM_DETAIL_NOEXCEPT
{
  return;
}

template <typename FuncT,
          std::int32_t N,
          typename T,
          typename OutputIterator,
          typename... Args,
          int DummyCallConv = detail::FuncCallConv<FuncT>::value>
inline void BuildCallArgs(OutputIterator call_args, T&& arg, Args&&... args)
{
  AddCallArg<FuncT, N>(call_args, std::forward<T>(arg));
  return BuildCallArgs<FuncT, N + 1>(++call_args, std::forward<Args>(args)...);
}
}

template <typename FuncT,
          typename... Args,
          int DummyCallConv = detail::FuncCallConv<FuncT>::value>
inline CallResult<detail::FuncResultT<FuncT>> Call(Process const& process,
                                                   void* address,
                                                   CallConv call_conv,
                                                   Args&&... args)
{
  HADESMEM_DETAIL_STATIC_ASSERT(detail::FuncArity<FuncT>::value ==
                                sizeof...(args));

  std::vector<CallArg> call_args;
  call_args.reserve(sizeof...(args));
  detail::BuildCallArgs<FuncT, 0>(std::back_inserter(call_args),
                                  std::forward<Args>(args)...);

  CallResultRaw const ret = CallRaw(
    process, address, call_conv, std::begin(call_args), std::end(call_args));
  using ResultT = detail::FuncResultT<FuncT>;
  return detail::CallResultRawToCallResult<ResultT>(ret);
}

namespace detail
{

template <typename FuncT,
          int DummyCallConv = detail::FuncCallConv<FuncT>::value>
inline void* FuncToPointerImpl(FuncT func, std::true_type)
{
  using FuncPtrT = std::add_pointer_t<FuncT>;
  auto const func_ptr = static_cast<FuncPtrT>(func);
  return detail::UnionCastUnchecked<void*>(func_ptr);
}

template <typename FuncT,
          int DummyCallConv = detail::FuncCallConv<FuncT>::value>
inline void* FuncToPointerImpl(FuncT func, std::false_type)
{
  auto const func_ptr = static_cast<FuncT>(func);
  return detail::UnionCastUnchecked<void*>(func_ptr);
}

template <typename FuncT,
          int DummyCallConv = detail::FuncCallConv<FuncT>::value>
inline void* FuncToPointer(FuncT func)
{
  return FuncToPointerImpl(func, std::is_function<FuncT>());
}
}

template <typename FuncT,
          typename... Args,
          int DummyCallConv = detail::FuncCallConv<FuncT>::value>
inline CallResult<detail::FuncResultT<FuncT>> Call(Process const& process,
                                                   FuncT address,
                                                   CallConv call_conv,
                                                   Args&&... args)
{
  HADESMEM_DETAIL_STATIC_ASSERT(detail::IsFunction<FuncT>::value);

  return Call<FuncT>(process,
                     detail::FuncToPointer(address),
                     call_conv,
                     std::forward<Args>(args)...);
}

class MultiCall
{
public:
  explicit MultiCall(Process const& process)
    : process_(&process), addresses_(), call_convs_(), args_()
  {
  }

  explicit MultiCall(Process&& process) = delete;

#if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

  MultiCall(MultiCall const&) = default;

  MultiCall& operator=(MultiCall const&) = default;

  MultiCall(MultiCall&& other)
    : process_(other.process_),
      addresses_(std::move(other.addresses_)),
      call_convs_(std::move(other.call_convs_)),
      args_(std::move(other.args_))
  {
  }

  MultiCall& operator=(MultiCall&& other)
  {
    process_ = other.process_;
    addresses_ = std::move(other.addresses_);
    call_convs_ = std::move(other.call_convs_);
    args_ = std::move(other.args_);

    return *this;
  }

#endif // #if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

  template <typename FuncT, typename... Args>
  inline void Add(void* address, CallConv call_conv, Args&&... args)
  {
    HADESMEM_DETAIL_STATIC_ASSERT(detail::FuncArity<FuncT>::value ==
                                  sizeof...(args));

    std::vector<CallArg> call_args;
    call_args.reserve(sizeof...(args));
    detail::BuildCallArgs<FuncT, 0>(std::back_inserter(call_args),
                                    std::forward<Args>(args)...);

    addresses_.push_back(address);
    call_convs_.push_back(call_conv);
    args_.push_back(call_args);
  }

  template <typename FuncT, typename... Args>
  inline void Add(FuncT address, CallConv call_conv, Args&&... args)
  {
    HADESMEM_DETAIL_STATIC_ASSERT(detail::IsFunction<FuncT>::value);

    return Add<FuncT>(
      detail::FuncToPointer(address), call_conv, std::forward<Args>(args)...);
  }

  template <typename OutputIterator> void Call(OutputIterator results) const
  {
    using OutputIteratorCategory =
      typename std::iterator_traits<OutputIterator>::iterator_category;
    HADESMEM_DETAIL_STATIC_ASSERT(
      std::is_base_of<std::output_iterator_tag, OutputIteratorCategory>::value);

    CallMulti(*process_,
              std::begin(addresses_),
              std::end(addresses_),
              std::begin(call_convs_),
              std::begin(args_),
              results);
  }

private:
  Process const* process_;
  std::vector<void*> addresses_;
  std::vector<CallConv> call_convs_;
  std::vector<std::vector<CallArg>> args_;
};
}
