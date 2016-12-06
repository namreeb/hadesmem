// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <vector>

#include <windows.h>

#include <hadesmem/alloc.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/patch_code_gen.hpp>
#include <hadesmem/detail/patch_detour_stub.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/flush.hpp>
#include <hadesmem/local/patch_detour_base.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

namespace hadesmem
{
template <typename TargetFuncT, typename ContextT = void*>
class PatchFuncRva : public PatchDetourBase
{
public:
  using StubT = detail::PatchDetourStub<TargetFuncT>;
  using DetourFuncRawT = typename StubT::DetourFuncRawT;
  using DetourFuncT = typename StubT::DetourFuncT;

  HADESMEM_DETAIL_STATIC_ASSERT(detail::IsFunction<TargetFuncT>::value);
  HADESMEM_DETAIL_STATIC_ASSERT(detail::IsFunction<DetourFuncRawT>::value);

  explicit PatchFuncRva(Process const& process,
                        void* base,
                        DWORD* target,
                        DetourFuncT const& detour,
                        ContextT context = ContextT())
    : process_{&process},
      base_{base},
      target_{target},
      detour_{detour},
      context_(std::move(context)),
      stub_{std::make_unique<StubT>(this)}
  {
    if (process.GetId() != ::GetCurrentProcessId())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error{} << ErrorString{
          "PatchFuncRva only supported on local process."});
    }
  }

  explicit PatchFuncRva(Process const&& process,
                        void* base,
                        DWORD* target,
                        DetourFuncT const& detour,
                        ContextT context = ContextT()) = delete;

  PatchFuncRva(PatchFuncRva const& other) = delete;

  PatchFuncRva& operator=(PatchFuncRva const& other) = delete;

  PatchFuncRva(PatchFuncRva&& other)
    : process_{other.process_},
      applied_{other.applied_},
      base_{other.base_},
      target_{other.target_},
      detour_{std::move(other.detour_)},
      stub_gate_{std::move(other.stub_gate_)},
      orig_(other.orig_),
      ref_count_{other.ref_count_.load()},
      stub_{other.stub_},
      context_(std::move(other.context_))
  {
    other.process_ = nullptr;
    other.applied_ = false;
    other.base_ = nullptr;
    other.target_ = nullptr;
    other.orig_ = 0;
    other.stub_ = nullptr;
  }

  PatchFuncRva& operator=(PatchFuncRva&& other)
  {
    RemoveUnchecked();

    process_ = other.process_;
    other.process_ = nullptr;

    applied_ = other.applied_;
    other.applied_ = false;

    base_ = other.base_;
    other.base_ = nullptr;

    target_ = other.target_;
    other.target_ = nullptr;

    detour_ = std::move(other.detour_);

    stub_gate_ = std::move(other.stub_gate_);

    orig_ = other.orig_;
    other.orig_ = 0;

    ref_count_ = other.ref_count_.load();

    stub_ = other.stub_;
    other.stub_ = nullptr;

    context_ = std::move(other.context_);

    return *this;
  }

  virtual ~PatchFuncRva()
  {
    RemoveUnchecked();
  }

  virtual void Apply()
  {
    if (applied_)
    {
      return;
    }

    if (detached_)
    {
      HADESMEM_DETAIL_ASSERT(false);
      return;
    }

    stub_gate_ = nullptr;

    auto const detour_raw = detour_.target<DetourFuncRawT>();
    if (detour_raw || detour_)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "Target = %p, Detour = %p.", target_, detour_raw);
    }
    else
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A("Target = %p, Detour = INVALID.", target_);
    }

    stub_gate_ = detail::AllocatePageNear(*process_, base_);

    detail::WriteStubGate<TargetFuncT>(*process_,
                                       stub_gate_->GetBase(),
                                       &*stub_,
                                       &GetOriginalArbitraryUserPtrPtr,
                                       &GetReturnAddressPtrPtr);

    orig_ = Read<DWORD>(*process_, target_);

    WritePatch();

    applied_ = true;
  }

  virtual void Remove() override
  {
    if (!applied_)
    {
      return;
    }

    RemovePatch();

    // Don't free trampolines here. Do it in Apply/destructor. See comments in
    // Apply for the rationale.

    applied_ = false;
  }

  virtual void RemoveUnchecked() noexcept override
  {
    try
    {
      Remove();
    }
    catch (...)
    {
      // WARNING: Patch may not be removed if Remove fails.
      HADESMEM_DETAIL_TRACE_A(
        boost::current_exception_diagnostic_information().c_str());
      HADESMEM_DETAIL_ASSERT(false);

      process_ = nullptr;
      applied_ = false;

      target_ = nullptr;
      detour_ = nullptr;
      orig_ = 0;
    }
  }

  virtual void Detach() noexcept override
  {
    applied_ = false;

    detached_ = true;
  }

  virtual bool IsApplied() const noexcept override
  {
    return applied_;
  }

  virtual void* GetTrampoline() const noexcept override
  {
    return static_cast<std::uint8_t*>(base_) + orig_;
  }

  virtual std::atomic<std::uint32_t>& GetRefCount() override
  {
    return ref_count_;
  }

  virtual std::atomic<std::uint32_t> const& GetRefCount() const override
  {
    return ref_count_;
  }

  virtual bool CanHookChain() const noexcept override
  {
    return CanHookChainImpl();
  }

  virtual void* GetTarget() const noexcept override
  {
    return target_;
  }

  virtual void const* GetDetour() const noexcept override
  {
    return &detour_;
  }

  virtual void* GetContext() noexcept override
  {
    return &context_;
  }

  virtual void const* GetContext() const noexcept override
  {
    return &context_;
  }

protected:
  virtual std::size_t GetPatchSize() const
  {
    return sizeof(void*);
  }

  virtual void WritePatch()
  {
    HADESMEM_DETAIL_TRACE_A("Writing pointer to stub.");

    auto const stub_gate_rva =
      reinterpret_cast<std::uintptr_t>(base_) -
      reinterpret_cast<std::uintptr_t>(stub_gate_->GetBase());
    HADESMEM_DETAIL_ASSERT(stub_gate_rva ==
                           static_cast<std::uintptr_t>(static_cast<long>(
                             static_cast<std::intptr_t>(stub_gate_rva))));
    Write(*process_, target_, static_cast<DWORD>(stub_gate_rva));
  }

  virtual void RemovePatch()
  {
    HADESMEM_DETAIL_TRACE_A("Restoring original pointer.");

    Write(*process_, target_, orig_);
  }

  virtual bool CanHookChainImpl() const noexcept
  {
    return true;
  }

private:
  Process const* process_{};
  bool applied_{false};
  bool detached_{false};
  void* base_{};
  DWORD* target_{};
  DetourFuncT detour_{};
  std::unique_ptr<Allocator> stub_gate_{};
  DWORD orig_{};
  std::atomic<std::uint32_t> ref_count_{};
  std::unique_ptr<StubT> stub_{};
  ContextT context_;
};
}
