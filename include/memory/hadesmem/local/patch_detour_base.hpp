// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <atomic>
#include <cstdint>
#include <type_traits>

#include <hadesmem/alloc.hpp>
#include <hadesmem/detail/alias_cast.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/thread_aux.hpp>
#include <hadesmem/detail/type_traits.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>

// TODO: Move all 'local' code to 'local' namespace.

namespace hadesmem
{
class PatchDetourBase
{
public:
  virtual void Apply() = 0;

  virtual void Remove() = 0;

  virtual void RemoveUnchecked() noexcept = 0;

  virtual void Detach() noexcept = 0;

  virtual bool IsApplied() const noexcept = 0;

  virtual void* GetTrampoline() const noexcept = 0;

  virtual std::atomic<std::uint32_t>& GetRefCount() = 0;

  virtual std::atomic<std::uint32_t> const& GetRefCount() const = 0;

  virtual bool CanHookChain() const noexcept = 0;

  virtual void* GetTarget() const noexcept = 0;

  virtual void const* GetDetour() const noexcept = 0;

  virtual void* GetContext() noexcept = 0;

  virtual void const* GetContext() const noexcept = 0;

  virtual void* GetOriginalArbitraryUserPtr() const noexcept
  {
    return *GetOriginalArbitraryUserPtrPtr();
  }

  virtual void* GetReturnAddressPtr() const noexcept
  {
    return *GetReturnAddressPtrPtr();
  }

  template <typename FuncT> FuncT GetTrampolineT() const noexcept
  {
    HADESMEM_DETAIL_STATIC_ASSERT(detail::IsFunction<FuncT>::value ||
                                  std::is_pointer<FuncT>::value);
    return detail::AliasCastUnchecked<FuncT>(GetTrampoline());
  }

protected:
  // WARNING! This will not work if TLS has not yet been initialized for the
  // thread.
  // TODO: Find a better way to implement this without the dependency on TLS.
  // Required for hooking APIs which are called very early on in the thread
  // initialization phase. E.g. RtlAllocateHeap.
  static void** GetOriginalArbitraryUserPtrPtr() noexcept
  {
    thread_local static void* orig_user_ptr = 0;
    return &orig_user_ptr;
  }

  static void** GetReturnAddressPtrPtr() noexcept
  {
    thread_local static void* ret_address_ptr = 0;
    return &ret_address_ptr;
  }
};
}