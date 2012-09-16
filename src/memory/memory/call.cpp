// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#include "hadesmem/call.hpp"

#include <cstddef>

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <boost/scope_exit.hpp>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/detail/warning_disable_prefix.hpp"
#include <AsmJit/AsmJit.h>
#include "hadesmem/detail/warning_disable_suffix.hpp"

#include "hadesmem/read.hpp"
#include "hadesmem/alloc.hpp"
#include "hadesmem/error.hpp"
#include "hadesmem/write.hpp"
#include "hadesmem/module.hpp"
#include "hadesmem/process.hpp"
#include "hadesmem/detail/type_traits.hpp"
#include "hadesmem/detail/call_remote_data.hpp"
#include "hadesmem/detail/call_codegen_x86.hpp"
#include "hadesmem/detail/call_codegen_x64.hpp"
#include "hadesmem/detail/call_arg_visitor_x86.hpp"
#include "hadesmem/detail/call_arg_visitor_x64.hpp"

// TODO: Improve and clean up this mess, move details to different files, 
// split code gen into detail funcs, etc.

// TODO: Improve safety via EH.

// TODO: Add support for more 'complex' argument and return types, including 
// struct/class/union, long double, SIMD types, etc. A good reference for 
// calling conventions is available at http://goo.gl/5rUxn.

// TODO: Only JIT code for Call once, then cache. Rewrite to pull data 
// externally instead of being regenerated for every call.

namespace hadesmem
{

CallResult::CallResult(DWORD_PTR return_int_ptr, 
  DWORD32 return_int_32, 
  DWORD64 return_int_64, 
  float return_float, 
  double return_double, 
  DWORD last_error) BOOST_NOEXCEPT
  : int_ptr_(return_int_ptr), 
  int_32_(return_int_32), 
  int_64_(return_int_64), 
  float_(return_float), 
  double_(return_double), 
  last_error_(last_error)
{ }

CallResult::CallResult(CallResult const& other) BOOST_NOEXCEPT
  : int_ptr_(other.int_ptr_), 
  int_32_(other.int_32_), 
  int_64_(other.int_64_), 
  float_(other.float_), 
  double_(other.double_), 
  last_error_(other.last_error_)
{ }

CallResult& CallResult::operator=(CallResult const& other) BOOST_NOEXCEPT
{
  int_ptr_ = other.int_ptr_;
  int_32_ = other.int_32_;
  int_64_ = other.int_64_;
  float_ = other.float_;
  double_ = other.double_;
  last_error_ = other.last_error_;

  return *this;
}

CallResult::CallResult(CallResult&& other) BOOST_NOEXCEPT
  : int_ptr_(other.int_ptr_), 
  int_32_(other.int_32_), 
  int_64_(other.int_64_), 
  float_(other.float_), 
  double_(other.double_), 
  last_error_(other.last_error_)
{
  other.int_ptr_ = 0;
  other.int_32_ = 0;
  other.int_64_ = 0;
  other.float_ = 0;
  other.double_ = 0;
  other.last_error_ = 0;
}

CallResult& CallResult::operator=(CallResult&& other) BOOST_NOEXCEPT
{
  int_ptr_ = other.int_ptr_;
  other.int_ptr_ = 0;

  int_64_ = other.int_64_;
  other.int_64_ = 0;

  int_32_ = other.int_32_;
  other.int_32_ = 0;

  float_ = other.float_;
  other.float_ = 0;

  double_ = other.double_;
  other.double_ = 0;

  last_error_ = other.last_error_;
  other.last_error_ = 0;

  return *this;
}

CallResult::~CallResult()
{ }

DWORD_PTR CallResult::GetReturnValueIntPtr() const BOOST_NOEXCEPT
{
  return int_ptr_;
}

DWORD32 CallResult::GetReturnValueInt32() const BOOST_NOEXCEPT
{
  return int_32_;
}

DWORD64 CallResult::GetReturnValueInt64() const BOOST_NOEXCEPT
{
  return int_64_;
}

float CallResult::GetReturnValueFloat() const BOOST_NOEXCEPT
{
  return float_;
}

double CallResult::GetReturnValueDouble() const BOOST_NOEXCEPT
{
  return double_;
}

DWORD CallResult::GetLastError() const BOOST_NOEXCEPT
{
  return last_error_;
}

CallResult Call(Process const& process, 
  LPCVOID address, 
  CallConv call_conv, 
  std::vector<CallArg> const& args)
{
  std::vector<LPCVOID> addresses;
  addresses.push_back(address);
  std::vector<CallConv> call_convs;
  call_convs.push_back(call_conv);
  std::vector<std::vector<CallArg>> args_full;
  args_full.push_back(args);
  return CallMulti(process, addresses, call_convs, args_full)[0];
}

std::vector<CallResult> CallMulti(Process const& process, 
  std::vector<LPCVOID> const& addresses, 
  std::vector<CallConv> const& call_convs, 
  std::vector<std::vector<CallArg>> const& args_full) 
{
  BOOST_ASSERT(addresses.size() == call_convs.size() && 
    addresses.size() == args_full.size());
  
  Module kernel32(&process, L"kernel32.dll");
  DWORD_PTR const get_last_error = reinterpret_cast<DWORD_PTR>(
    FindProcedure(kernel32, "GetLastError"));
  DWORD_PTR const set_last_error = reinterpret_cast<DWORD_PTR>(
    FindProcedure(kernel32, "SetLastError"));
  DWORD_PTR const is_debugger_present = reinterpret_cast<DWORD_PTR>(
    FindProcedure(kernel32, "IsDebuggerPresent"));
  DWORD_PTR const debug_break = reinterpret_cast<DWORD_PTR>(
    FindProcedure(kernel32, "DebugBreak"));

  Allocator const return_values_remote(&process, 
    sizeof(detail::CallResultRemote) * addresses.size());

  AsmJit::X86Assembler assembler;
  
#if defined(_M_AMD64)
  detail::GenerateCallCode64(&assembler, addresses, call_convs, args_full, 
    get_last_error, set_last_error, is_debugger_present, debug_break, 
    return_values_remote.GetBase());
#elif defined(_M_IX86)
  detail::GenerateCallCode32(&assembler, addresses, call_convs, args_full, 
    get_last_error, set_last_error, is_debugger_present, debug_break, 
    return_values_remote.GetBase());
#else
#error "[HadesMem] Unsupported architecture."
#endif
  
  DWORD_PTR const stub_size = assembler.getCodeSize();
  
  Allocator const stub_mem_remote(&process, stub_size);
  PBYTE const stub_remote = static_cast<PBYTE>(stub_mem_remote.GetBase());
  DWORD_PTR const stub_remote_temp = reinterpret_cast<DWORD_PTR>(stub_remote);
  
  std::vector<BYTE> code_real(stub_size);
  assembler.relocCode(code_real.data(), reinterpret_cast<DWORD_PTR>(
    stub_remote));
  
  WriteVector(process, stub_remote, code_real);
  
  HANDLE const thread_remote = ::CreateRemoteThread(process.GetHandle(), 
    nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(stub_remote_temp), 
    nullptr, 0, nullptr);
  if (!thread_remote)
  {
    DWORD const last_error = ::GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Could not create remote thread.") << 
      ErrorCodeWinLast(last_error));
  }
  
  BOOST_SCOPE_EXIT_ALL(&)
  {
    // WARNING: Handle is leaked if FreeLibrary fails.
    BOOST_VERIFY(::CloseHandle(thread_remote));
  };

  // TODO: Allow customizable timeout.
  if (::WaitForSingleObject(thread_remote, INFINITE) != WAIT_OBJECT_0)
  {
    DWORD const LastError = ::GetLastError();
    BOOST_THROW_EXCEPTION(HadesMemError() << 
      ErrorString("Could not wait for remote thread.") << 
      ErrorCodeWinLast(LastError));
  }

  std::vector<detail::CallResultRemote> return_vals_remote = 
    ReadVector<detail::CallResultRemote>(process, 
    return_values_remote.GetBase(), addresses.size());
  
  std::vector<CallResult> return_vals;
  return_vals.reserve(addresses.size());
  
  std::transform(std::begin(return_vals_remote), std::end(return_vals_remote), 
    std::back_inserter(return_vals), 
    [] (detail::CallResultRemote const& r)
    {
      return CallResult(r.return_value, 
        r.return_value_32, 
        r.return_value_64, 
        r.return_value_float, 
        r.return_value_double, 
        r.last_error);
    });

  return return_vals;
}

MultiCall::MultiCall(Process const* process)
  : process_(process), 
  addresses_(), 
  call_convs_(), 
  args_()
{ }

MultiCall::MultiCall(MultiCall const& other)
  : process_(other.process_), 
  addresses_(other.addresses_), 
  call_convs_(other.call_convs_), 
  args_(other.args_)
{ }

MultiCall& MultiCall::operator=(MultiCall const& other)
{
  process_ = other.process_;
  addresses_ = other.addresses_;
  call_convs_ = other.call_convs_;
  args_ = other.args_;

  return *this;
}

MultiCall::MultiCall(MultiCall&& other) BOOST_NOEXCEPT
  : process_(other.process_), 
  addresses_(std::move(other.addresses_)), 
  call_convs_(std::move(other.call_convs_)), 
  args_(std::move(other.args_))
{
  other.process_ = nullptr;
}

MultiCall& MultiCall::operator=(MultiCall&& other) BOOST_NOEXCEPT
{
  process_ = other.process_;
  other.process_ = nullptr;

  addresses_ = std::move(other.addresses_);

  call_convs_ = std::move(other.call_convs_);

  args_ = std::move(other.args_);

  return *this;
}

MultiCall::~MultiCall()
{ }

std::vector<CallResult> MultiCall::Call() const
{
  return hadesmem::CallMulti(*process_, addresses_, call_convs_, args_);
}

}
