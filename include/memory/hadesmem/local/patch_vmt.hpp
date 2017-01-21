// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <vector>

#include <windows.h>

#include <hadesmem/alloc.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/flush.hpp>
#include <hadesmem/local/patch_detour_base.hpp>
#include <hadesmem/local/patch_func_ptr.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

// TODO: Add context support.

namespace hadesmem
{
class PatchVmt
{
public:
  struct TagUnsafe
  {
  };

  PatchVmt(hadesmem::Process const& process,
           void* target_class,
           std::size_t vmt_size)
    : process_{process},
      class_base_{static_cast<void***>(target_class)},
      old_vmt_{Read<void**>(process, class_base_)},
      vmt_size_{vmt_size},
      new_vmt_{process, vmt_size_ * sizeof(void*) + 1}
  {
    if (process.GetId() != ::GetCurrentProcessId())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error{} << ErrorString{"PatchVmt only supported on local process."});
    }

    Initialize();
  }

  PatchVmt(hadesmem::Process const& process, void* target_class, TagUnsafe)
    : process_{process},
      class_base_{static_cast<void***>(target_class)},
      old_vmt_{Read<void**>(process, class_base_)},
      vmt_size_{GetVmtSizeUnsafe(old_vmt_)},
      new_vmt_{process, vmt_size_ * sizeof(void*)}
  {
    Initialize();
  }

  explicit PatchVmt(Process const&& process,
                    void* target_class,
                    std::size_t vmt_size) = delete;

  explicit PatchVmt(Process const&& process,
                    void* target_class,
                    TagUnsafe) = delete;

  PatchVmt(PatchVmt const& other) = delete;

  PatchVmt& operator=(PatchVmt const& other) = delete;

  PatchVmt(PatchVmt&& other)
    : process_{std::move(other.process_)},
      class_base_{other.class_base_},
      old_vmt_{other.old_vmt_},
      vmt_size_(other.vmt_size_),
      new_vmt_(std::move(other.new_vmt_)),
      new_vmt_base_{other.new_vmt_base_},
      hooks_(std::move(other.hooks_))
  {
    other.class_base_ = false;
    other.old_vmt_ = nullptr;
    other.vmt_size_ = 0;
    other.new_vmt_base_ = nullptr;
  }

  PatchVmt& operator=(PatchVmt&& other)
  {
    RemoveUnchecked();

    process_ = std::move(other.process_);

    class_base_ = other.class_base_;
    other.class_base_ = nullptr;

    old_vmt_ = other.old_vmt_;
    other.old_vmt_ = nullptr;

    vmt_size_ = other.vmt_size_;
    other.vmt_size_ = 0;

    new_vmt_ = std::move(other.new_vmt_);

    new_vmt_base_ = other.new_vmt_base_;
    other.new_vmt_base_ = nullptr;

    hooks_ = std::move(other.hooks_);

    return *this;
  }

  ~PatchVmt()
  {
    RemoveUnchecked();
  }

  void Apply()
  {
    HADESMEM_DETAIL_ASSERT(class_base_);
    HADESMEM_DETAIL_ASSERT(new_vmt_base_);
    Write(process_, class_base_, new_vmt_base_);
  }

  void Remove()
  {
    HADESMEM_DETAIL_ASSERT(class_base_);
    HADESMEM_DETAIL_ASSERT(new_vmt_base_);
    Write(process_, class_base_, old_vmt_);
  }

  std::size_t GetSize() const noexcept
  {
    return vmt_size_;
  }

  template <typename TargetFuncT>
  void HookMethod(std::size_t idx,
                  typename PatchFuncPtr<TargetFuncT>::DetourFuncT detour,
                  void* context = nullptr)
  {
    using TargetFuncRawT = typename PatchFuncPtr<TargetFuncT>::TargetFuncRawT;
    auto const target = reinterpret_cast<TargetFuncRawT*>(
      &reinterpret_cast<void**>(new_vmt_base_)[idx]);
    auto const patch =
      new PatchFuncPtr<TargetFuncT>(process_, target, detour, context);
    hooks_.emplace_back(patch);
    patch->Apply();
  }

private:
  void Initialize()
  {
    HADESMEM_DETAIL_ASSERT(vmt_size_);
    try
    {
      auto const rtti_ptr =
        Read<void*>(process_, reinterpret_cast<std::uint8_t*>(old_vmt_) - 4);
      Write(process_, new_vmt_.GetBase(), rtti_ptr);
      new_vmt_base_ = static_cast<std::uint8_t*>(new_vmt_.GetBase()) + 4;
    }
    catch (...)
    {
      new_vmt_base_ = new_vmt_.GetBase();
    }
    auto const old_vmt_contents =
      ReadVector<void*>(process_, old_vmt_, vmt_size_);
    WriteVector(process_, new_vmt_base_, old_vmt_contents);
  }

  void RemoveUnchecked() noexcept
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
    }
  }

  std::size_t GetVmtSizeUnsafe(void** vmt) noexcept
  {
    std::size_t i = 0;
    try
    {
      void* f = Read<void*>(process_, vmt + i);
      for (; f; ++i)
      {
        if (!hadesmem::CanExecute(process_, f))
        {
          break;
        }

        f = Read<void*>(process_, vmt + i);
      }
    }
    catch (...)
    {
    }

    return i;
  }

  hadesmem::Process process_;
  void*** class_base_{};
  void** old_vmt_{};
  std::size_t vmt_size_{};
  Allocator new_vmt_;
  void* new_vmt_base_{};
  std::vector<std::unique_ptr<PatchDetourBase>> hooks_;
};
}
