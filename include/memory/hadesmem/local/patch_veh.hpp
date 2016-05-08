// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <atomic>
#include <climits>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <type_traits>
#include <utility>
#include <vector>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <udis86.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <windows.h>

#include <hadesmem/alloc.hpp>
#include <hadesmem/detail/alias_cast.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/patch_detour_stub.hpp>
#include <hadesmem/detail/patcher_aux.hpp>
#include <hadesmem/detail/scope_warden.hpp>
#include <hadesmem/detail/srw_lock.hpp>
#include <hadesmem/detail/thread_aux.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/detail/type_traits.hpp>
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/flush.hpp>
#include <hadesmem/local/patch_detour.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/thread.hpp>
#include <hadesmem/thread_list.hpp>
#include <hadesmem/thread_helpers.hpp>
#include <hadesmem/write.hpp>

// TODO: Add context support.

// TODO: Add page breakpoint hooks (PAGE_GUARD, PAGE_NOACCESS, etc.).

namespace hadesmem
{
template <typename TargetFuncT> class PatchVeh : public PatchDetour<TargetFuncT>
{
public:
  explicit PatchVeh(Process const& process,
                    TargetFuncT target,
                    DetourFuncT const& detour)
    : PatchDetour{process, target, detour}
  {
    Initialize();
  }

  explicit PatchVeh(Process const&& process,
                    TargetFuncT target,
                    DetourFuncT const& detour) = delete;

  PatchVeh(PatchVeh const& other) = delete;

  PatchVeh& operator=(PatchVeh const& other) = delete;

  PatchVeh(PatchVeh&& other) : PatchDetour{std::move(other)}
  {
  }

  PatchVeh& operator=(PatchVeh&& other)
  {
    PatchDetour::operator=(std::move(other));
    return *this;
  }

  static void InitializeStatics()
  {
    GetInitialized();
    GetSrwLock();
    GetVehHooks();
    GetDrHooks();
  }

protected:
  virtual std::size_t GetPatchSize() const override
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{} << ErrorString{"Unimplemented."});
  }

  virtual void WritePatch() override
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{} << ErrorString{"Unimplemented."});
  }

  virtual void RemovePatch() override
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{} << ErrorString{"Unimplemented."});
  }

  virtual bool CanHookChainImpl() const noexcept override
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(Error{} << ErrorString{"Unimplemented."});
  }

  static void Initialize()
  {
    auto& initialized = GetInitialized();
    if (initialized)
    {
      return;
    }

    auto const veh_handle = ::AddVectoredExceptionHandler(1, &VectoredHandler);
    if (!veh_handle)
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error{} << ErrorString{"AddVectoredExceptionHandler failed."}
                << ErrorCodeWinLast{last_error});
    }
    static detail::SmartRemoveVectoredExceptionHandler const remove_veh(
      veh_handle);

    initialized = true;
  }

  static LONG CALLBACK VectoredHandler(PEXCEPTION_POINTERS exception_pointers)
  {
    switch (exception_pointers->ExceptionRecord->ExceptionCode)
    {
    case EXCEPTION_BREAKPOINT:
      return HandleBreakpoint(exception_pointers);

    case EXCEPTION_SINGLE_STEP:
      return HandleSingleStep(exception_pointers);

    default:
      return EXCEPTION_CONTINUE_SEARCH;
    }
  }

  static LONG CALLBACK HandleBreakpoint(PEXCEPTION_POINTERS exception_pointers)
  {
    hadesmem::detail::AcquireSRWLock const lock(
      &GetSrwLock(), hadesmem::detail::SRWLockType::Shared);

    auto& veh_hooks = GetVehHooks();
    auto const iter =
      veh_hooks.find(exception_pointers->ExceptionRecord->ExceptionAddress);
    if (iter == std::end(veh_hooks))
    {
      return EXCEPTION_CONTINUE_SEARCH;
    }

    PatchVeh* patch = iter->second;
#if defined(HADESMEM_DETAIL_ARCH_X64)
    exception_pointers->ContextRecord->Rip =
      reinterpret_cast<std::uintptr_t>(patch->stub_gate_->GetBase());
#elif defined(HADESMEM_DETAIL_ARCH_X86)
    exception_pointers->ContextRecord->Eip =
      reinterpret_cast<std::uintptr_t>(patch->stub_gate_->GetBase());
#else
#error "[HadesMem] Unsupported architecture."
#endif

    return EXCEPTION_CONTINUE_EXECUTION;
  }

  static LONG CALLBACK HandleSingleStep(PEXCEPTION_POINTERS exception_pointers)
  {
    hadesmem::detail::AcquireSRWLock const lock(
      &GetSrwLock(), hadesmem::detail::SRWLockType::Shared);

    auto& veh_hooks = GetVehHooks();
    auto const iter =
      veh_hooks.find(exception_pointers->ExceptionRecord->ExceptionAddress);
    if (iter == std::end(veh_hooks))
    {
      return EXCEPTION_CONTINUE_SEARCH;
    }

    auto& dr_hooks = GetDrHooks();
    auto const dr_hook_iter = dr_hooks.find(::GetCurrentThreadId());
    if (dr_hook_iter == std::end(dr_hooks))
    {
      return EXCEPTION_CONTINUE_SEARCH;
    }

    std::uintptr_t const dr_index = dr_hook_iter->second;
    if (!(exception_pointers->ContextRecord->Dr6 & (1ULL << dr_index)))
    {
      return EXCEPTION_CONTINUE_SEARCH;
    }

    // Reset status register
    exception_pointers->ContextRecord->Dr6 = 0;
    // Set resume flag
    exception_pointers->ContextRecord->EFlags |= (1ULL << 16);

    PatchVeh* patch = iter->second;
#if defined(HADESMEM_DETAIL_ARCH_X64)
    exception_pointers->ContextRecord->Rip =
      reinterpret_cast<std::uintptr_t>(patch->stub_gate_->GetBase());
#elif defined(HADESMEM_DETAIL_ARCH_X86)
    exception_pointers->ContextRecord->Eip =
      reinterpret_cast<std::uintptr_t>(patch->stub_gate_->GetBase());
#else
#error "[HadesMem] Unsupported architecture."
#endif

    return EXCEPTION_CONTINUE_EXECUTION;
  }

  static bool& GetInitialized()
  {
    static bool initialized = false;
    return initialized;
  }

  static std::map<void*, PatchVeh*>& GetVehHooks()
  {
    static std::map<void*, PatchVeh*> veh_hooks;
    return veh_hooks;
  }

  static std::map<DWORD, std::uintptr_t>& GetDrHooks()
  {
    static std::map<DWORD, std::uintptr_t> dr_hooks;
    return dr_hooks;
  }

  static SRWLOCK& GetSrwLock()
  {
    static SRWLOCK srw_lock = SRWLOCK_INIT;
    return srw_lock;
  }
};
}
