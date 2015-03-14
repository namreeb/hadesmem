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

  virtual bool IsApplied() const HADESMEM_DETAIL_NOEXCEPT = 0;

  virtual void* GetTrampoline() const HADESMEM_DETAIL_NOEXCEPT = 0;

  virtual std::atomic<std::uint32_t>& GetRefCount() = 0;

  virtual std::atomic<std::uint32_t> const& GetRefCount() const = 0;

  virtual bool CanHookChain() const = 0;

  virtual void const* GetDetour() const HADESMEM_DETAIL_NOEXCEPT = 0;

  virtual void* GetContext() const HADESMEM_DETAIL_NOEXCEPT = 0;

  virtual void*
    GetOriginalArbitraryUserPtr() const HADESMEM_DETAIL_NOEXCEPT = 0;

  template <typename FuncT>
  FuncT GetTrampolineT() const HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_STATIC_ASSERT(detail::IsFunction<FuncT>::value ||
                                  std::is_pointer<FuncT>::value);
    return detail::AliasCastUnchecked<FuncT>(GetTrampoline());
  }
};
}
