// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/call.hpp>

#include <cstring>
#include <cstddef>
#include <algorithm>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <asmjit/asmjit.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/read.hpp>
#include <hadesmem/alloc.hpp>
#include <hadesmem/write.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/smart_handle.hpp>

namespace hadesmem
{

namespace
{

#if defined(HADESMEM_ARCH_X86)

class ArgVisitor32 : public boost::static_visitor<>
{
public:
  ArgVisitor32(AsmJit::X86Assembler* assembler, std::size_t num_args, 
    CallConv call_conv) HADESMEM_NOEXCEPT;

  void operator()(DWORD32 arg) HADESMEM_NOEXCEPT;

  void operator()(DWORD64 arg) HADESMEM_NOEXCEPT;

  void operator()(float arg) HADESMEM_NOEXCEPT;

  void operator()(double arg) HADESMEM_NOEXCEPT;

private:
  AsmJit::X86Assembler* assembler_;
  std::size_t cur_arg_;
  CallConv call_conv_;
};

#endif // #if defined(HADESMEM_ARCH_X86)

#if defined(HADESMEM_ARCH_X64)

class ArgVisitor64 : public boost::static_visitor<>
{
public:
  ArgVisitor64(AsmJit::X86Assembler* assembler, std::size_t num_args) 
    HADESMEM_NOEXCEPT;

  void operator()(DWORD32 arg) HADESMEM_NOEXCEPT;

  void operator()(DWORD64 arg) HADESMEM_NOEXCEPT;

  void operator()(float arg) HADESMEM_NOEXCEPT;

  void operator()(double arg) HADESMEM_NOEXCEPT;

private:
  AsmJit::X86Assembler* assembler_;
  std::size_t num_args_;
  std::size_t cur_arg_;
};

#endif // #if defined(HADESMEM_ARCH_X64)

#if defined(HADESMEM_ARCH_X86)

void GenerateCallCode32(AsmJit::X86Assembler* assembler, 
  std::vector<FnPtr> const& addresses, 
  std::vector<CallConv> const& call_convs, 
  std::vector<std::vector<CallArg>> const& args_full, 
  DWORD_PTR get_last_error, 
  DWORD_PTR set_last_error, 
  DWORD_PTR is_debugger_present, 
  DWORD_PTR debug_break, 
  PVOID return_values_remote)
{
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

  for (std::size_t i = 0; i < addresses.size(); ++i)
  {
    FnPtr address = addresses[i];
    CallConv call_conv = call_convs[i];
    std::vector<CallArg> const& args = args_full[i];
    std::size_t const num_args = args.size();

    ArgVisitor32 arg_visitor(assembler, num_args, call_conv);
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

#endif // #if defined(HADESMEM_ARCH_X86)

#if defined(HADESMEM_ARCH_X64)

void GenerateCallCode64(AsmJit::X86Assembler* assembler, 
  std::vector<FnPtr> const& addresses, 
  std::vector<CallConv> const& call_convs, 
  std::vector<std::vector<CallArg>> const& args_full, 
  DWORD_PTR get_last_error, 
  DWORD_PTR set_last_error, 
  DWORD_PTR is_debugger_present, 
  DWORD_PTR debug_break, 
  PVOID return_values_remote)
{
  AsmJit::Label label_nodebug(assembler->newLabel());

  auto const max_args_list = std::max_element(std::begin(args_full), 
    std::end(args_full), 
    [] (std::vector<CallArg> const& args1, std::vector<CallArg> const& args2)
  {
    return args1.size() < args2.size();
  });
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

  for (std::size_t i = 0; i < addresses.size(); ++i)
  {
    FnPtr address = addresses[i];
    std::vector<CallArg> const& args = args_full[i];
    std::size_t const num_args = args.size();

    (void)call_convs;
    HADESMEM_ASSERT(call_convs[i] == CallConv::kDefault || 
      call_convs[i] == CallConv::kWinApi || 
      call_convs[i] == CallConv::kX64);

    ArgVisitor64 arg_visitor(assembler, num_args);
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

#endif // #if defined(HADESMEM_ARCH_X64)

Allocator GenerateCallCode(Process const& process, 
  std::vector<FnPtr> const& addresses, 
  std::vector<CallConv> const& call_convs, 
  std::vector<std::vector<CallArg>> const& args_full, 
  PVOID return_values_remote)
{
  Module const kernel32(process, L"kernel32.dll");
  auto const get_last_error = reinterpret_cast<DWORD_PTR>(FindProcedure(
    kernel32, "GetLastError"));
  auto const set_last_error = reinterpret_cast<DWORD_PTR>(FindProcedure(
    kernel32, "SetLastError"));
  auto const is_debugger_present = reinterpret_cast<DWORD_PTR>(FindProcedure(
    kernel32, "IsDebuggerPresent"));
  auto const debug_break = reinterpret_cast<DWORD_PTR>(FindProcedure(
    kernel32, "DebugBreak"));

  AsmJit::X86Assembler assembler;
  
#if defined(HADESMEM_ARCH_X64)
  GenerateCallCode64(&assembler, addresses, call_convs, args_full, 
    get_last_error, set_last_error, is_debugger_present, debug_break, 
    return_values_remote);
#elif defined(HADESMEM_ARCH_X86)
  GenerateCallCode32(&assembler, addresses, call_convs, args_full, 
    get_last_error, set_last_error, is_debugger_present, debug_break, 
    return_values_remote);
#else
#error "[HadesMem] Unsupported architecture."
#endif
  
  DWORD_PTR const stub_size = assembler.getCodeSize();
  
  Allocator stub_mem_remote(process, stub_size);
  
  std::vector<BYTE> code_real(stub_size);
  assembler.relocCode(code_real.data(), reinterpret_cast<DWORD_PTR>(
    stub_mem_remote.GetBase()));
  
  WriteVector(process, stub_mem_remote.GetBase(), code_real);

  return stub_mem_remote;
}

}

#if defined(HADESMEM_ARCH_X86)

ArgVisitor32::ArgVisitor32(AsmJit::X86Assembler* assembler, 
  std::size_t num_args, CallConv call_conv) HADESMEM_NOEXCEPT
  : assembler_(assembler), 
  cur_arg_(num_args), 
  call_conv_(call_conv)
{ }

void ArgVisitor32::operator()(DWORD32 arg) HADESMEM_NOEXCEPT
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

void ArgVisitor32::operator()(DWORD64 arg) HADESMEM_NOEXCEPT
{
  assembler_->mov(AsmJit::eax, AsmJit::uimm(static_cast<DWORD>(
    (arg >> 32) & 0xFFFFFFFFUL)));
  assembler_->push(AsmJit::eax);

  assembler_->mov(AsmJit::eax, AsmJit::uimm(static_cast<DWORD>(arg & 
    0xFFFFFFFFUL)));
  assembler_->push(AsmJit::eax);

  --cur_arg_;
}

void ArgVisitor32::operator()(float arg) HADESMEM_NOEXCEPT
{
  HADESMEM_STATIC_ASSERT(sizeof(float) == 4);
  HADESMEM_STATIC_ASSERT(sizeof(float) == sizeof(DWORD));

  DWORD arg_conv;
  std::memcpy(&arg_conv, &arg, sizeof(arg));

  assembler_->mov(AsmJit::eax, AsmJit::uimm(arg_conv));
  assembler_->push(AsmJit::eax);

  --cur_arg_;
}

void ArgVisitor32::operator()(double arg) HADESMEM_NOEXCEPT
{
  HADESMEM_STATIC_ASSERT(sizeof(double) == 8);
  HADESMEM_STATIC_ASSERT(sizeof(double) == sizeof(DWORD64));

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

#endif // #if defined(HADESMEM_ARCH_X86)

#if defined(HADESMEM_ARCH_X64)

ArgVisitor64::ArgVisitor64(AsmJit::X86Assembler* assembler, 
  std::size_t num_args) HADESMEM_NOEXCEPT
  : assembler_(assembler), 
  num_args_(num_args), 
  cur_arg_(num_args)
{ }

void ArgVisitor64::operator()(DWORD32 arg) HADESMEM_NOEXCEPT
{
  return (*this)(static_cast<DWORD64>(arg));
}

void ArgVisitor64::operator()(DWORD64 arg) HADESMEM_NOEXCEPT
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

void ArgVisitor64::operator()(float arg) HADESMEM_NOEXCEPT
{
  HADESMEM_STATIC_ASSERT(sizeof(float) == 4);
  HADESMEM_STATIC_ASSERT(sizeof(float) == sizeof(DWORD));

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

void ArgVisitor64::operator()(double arg) HADESMEM_NOEXCEPT
{
  HADESMEM_STATIC_ASSERT(sizeof(double) == 8);
  HADESMEM_STATIC_ASSERT(sizeof(double) == sizeof(DWORD64));

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

#endif // #if defined(HADESMEM_ARCH_X64)

CallResultRaw Call(Process const& process, 
  FnPtr address, 
  CallConv call_conv, 
  std::vector<CallArg> const& args)
{
  std::vector<FnPtr> addresses;
  addresses.push_back(address);
  std::vector<CallConv> call_convs;
  call_convs.push_back(call_conv);
  std::vector<std::vector<CallArg>> args_full;
  args_full.push_back(args);
  return CallMulti(process, addresses, call_convs, args_full)[0];
}

std::vector<CallResultRaw> CallMulti(Process const& process, 
  std::vector<FnPtr> const& addresses, 
  std::vector<CallConv> const& call_convs, 
  std::vector<std::vector<CallArg>> const& args_full) 
{
  HADESMEM_ASSERT(addresses.size() == call_convs.size() && 
    addresses.size() == args_full.size());

  Allocator const return_values_remote(process, 
    sizeof(detail::CallResultRemote) * addresses.size());

  Allocator const code_remote(GenerateCallCode(process, addresses, 
    call_convs, args_full, return_values_remote.GetBase()));
  LPTHREAD_START_ROUTINE code_remote_pfn = 
    reinterpret_cast<LPTHREAD_START_ROUTINE>(
    reinterpret_cast<DWORD_PTR>(code_remote.GetBase()));

  detail::SmartHandle const thread_remote(::CreateRemoteThread(
    process.GetHandle(), nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(
    code_remote_pfn), nullptr, 0, nullptr));
  if (!thread_remote.GetHandle())
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Could not create remote thread.") << 
      ErrorCodeWinLast(last_error));
  }
  
  // TODO: Allow customizable timeout.
  if (::WaitForSingleObject(thread_remote.GetHandle(), INFINITE) != 
    WAIT_OBJECT_0)
  {
    DWORD const LastError = ::GetLastError();
    HADESMEM_THROW_EXCEPTION(Error() << 
      ErrorString("Could not wait for remote thread.") << 
      ErrorCodeWinLast(LastError));
  }

  std::vector<detail::CallResultRemote> return_vals_remote = 
    ReadVector<detail::CallResultRemote>(process, 
    return_values_remote.GetBase(), addresses.size());
  
  std::vector<CallResultRaw> return_vals;
  return_vals.reserve(addresses.size());
  
  std::transform(std::begin(return_vals_remote), std::end(return_vals_remote), 
    std::back_inserter(return_vals), 
    [] (detail::CallResultRemote const& r)
    {
      return static_cast<CallResultRaw>(r);
    });

  return return_vals;
}

}
