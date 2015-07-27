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

  template <typename FuncT>
  FuncT GetTrampolineT() const noexcept
  {
    HADESMEM_DETAIL_STATIC_ASSERT(detail::IsFunction<FuncT>::value ||
                                  std::is_pointer<FuncT>::value);
    return detail::AliasCastUnchecked<FuncT>(GetTrampoline());
  }

protected:
  static void** GetOriginalArbitraryUserPtrPtr() noexcept
  {
    static __declspec(thread) void* orig_user_ptr = 0;
    return &orig_user_ptr;
  }
};
}
