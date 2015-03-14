// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include <windows.h>

#include <hadesmem/detail/trace.hpp>
#include <hadesmem/detail/type_traits.hpp>
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/patch_detour_base.hpp>

namespace hadesmem
{
namespace detail
{
template <typename TargetFuncT> class PatchDetourStub;

#define HADESMEM_DETAIL_MAKE_PATCH_DETOUR_STUB(call_conv)                      \
  \
template<typename R, typename... Args> \
class PatchDetourStub<R(call_conv*)(Args...)>                                  \
  \
{                                                                         \
  \
public:                                                                        \
    using DetourFuncRawT = R(call_conv*)(PatchDetourBase*, Args...);           \
    using DetourFuncT = std::function<R(PatchDetourBase*, Args...)>;           \
                                                                               \
    explicit PatchDetourStub(PatchDetourBase* patch) : patch_{patch}           \
    {                                                                          \
    }                                                                          \
                                                                               \
    static R call_conv Stub(Args... args)                                      \
    {                                                                          \
      auto const stub = static_cast<PatchDetourStub*>(                         \
        winternl::GetCurrentTeb()->NtTib.ArbitraryUserPointer);                \
      return stub->StubImpl(std::forward<Args>(args)...);                      \
    }                                                                          \
  \
private:                                                                       \
    R StubImpl(Args... args)                                                   \
    {                                                                          \
      HADESMEM_DETAIL_STATIC_ASSERT(IsFunction<DetourFuncRawT>::value);        \
      winternl::GetCurrentTeb()->NtTib.ArbitraryUserPointer =                  \
        patch_->GetOriginalArbitraryUserPtr();                                 \
      auto const detour =                                                      \
        static_cast<DetourFuncT const*>(patch_->GetDetour());                  \
      return (*detour)(patch_, std::forward<Args>(args)...);                   \
    }                                                                          \
                                                                               \
    PatchDetourBase* patch_;                                                   \
  \
};                                                                             \
  \
\
template<typename R, typename... Args> \
class PatchDetourStub<R call_conv(Args...)>                                    \
  \
{                                                                         \
  \
public:                                                                        \
    using DetourFuncRawT = R call_conv(PatchDetourBase*, Args...);             \
    using DetourFuncT = std::function<R(PatchDetourBase*, Args...)>;           \
                                                                               \
    explicit PatchDetourStub(PatchDetourBase* patch) : patch_{patch}           \
    \
{                                                                       \
    \
}                                                                       \
    \
\
static R call_conv Stub(Args... args)                                          \
    \
{                                                                       \
      \
auto const stub = static_cast<PatchDetourStub*>(\
winternl::GetCurrentTeb()->NtTib.ArbitraryUserPointer);                        \
      \
return stub->StubImpl(std::forward<Args>(args)...);                            \
    \
}                                                                       \
  \
\
private:                                                                       \
    R StubImpl(Args... args)                                                   \
    \
{                                                                       \
      \
HADESMEM_DETAIL_STATIC_ASSERT(IsFunction<DetourFuncRawT>::value);              \
      \
winternl::GetCurrentTeb()->NtTib.ArbitraryUserPointer = \
patch_->GetOriginalArbitraryUserPtr();                                         \
      \
auto const detour = static_cast<DetourFuncT const*>(patch_->GetDetour());      \
      \
return (*detour)(patch_, std::forward<Args>(args)...);                         \
    \
}                                                                       \
    \
\
PatchDetourBase* patch_;                                                       \
  \
};

#if defined(HADESMEM_DETAIL_ARCH_X64)

HADESMEM_DETAIL_MAKE_PATCH_DETOUR_STUB(__fastcall)

#elif defined(HADESMEM_DETAIL_ARCH_X86)

HADESMEM_DETAIL_MAKE_PATCH_DETOUR_STUB(__cdecl)
HADESMEM_DETAIL_MAKE_PATCH_DETOUR_STUB(__stdcall)
HADESMEM_DETAIL_MAKE_PATCH_DETOUR_STUB(__fastcall)

#else
#error "[HadesMem] Unsupported architecture."
#endif

#if !defined(HADESMEM_DETAIL_NO_VECTORCALL)

HADESMEM_DETAIL_MAKE_PATCH_DETOUR_STUB(__vectorcall)

#endif

#undef HADESMEM_DETAIL_MAKE_PATCH_DETOUR_STUB
}
}
