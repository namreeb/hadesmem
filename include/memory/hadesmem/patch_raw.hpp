// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <vector>

#include <windows.h>

#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/patcher_aux.hpp>
#include <hadesmem/detail/thread_aux.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/flush.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/thread_helpers.hpp>
#include <hadesmem/write.hpp>

namespace hadesmem
{
class PatchRaw
{
public:
  explicit PatchRaw(Process const& process,
                    void* target,
                    std::vector<std::uint8_t> const& data)
    : process_{process}, target_{target}, data_(data)
  {
  }

  explicit PatchRaw(Process const&& process,
                    PVOID target,
                    std::vector<std::uint8_t> const& data) = delete;

  PatchRaw(PatchRaw const& other) = delete;

  PatchRaw& operator=(PatchRaw const& other) = delete;

  PatchRaw(PatchRaw&& other)
    : process_{std::move(other.process_)},
      applied_{other.applied_},
      target_{other.target_},
      data_(std::move(other.data_)),
      orig_(std::move(other.orig_))
  {
    other.applied_ = false;
    other.target_ = nullptr;
  }

  PatchRaw& operator=(PatchRaw&& other)
  {
    RemoveUnchecked();

    process_ = std::move(other.process_);

    applied_ = other.applied_;
    other.applied_ = false;

    target_ = other.target_;
    other.target_ = nullptr;

    data_ = std::move(other.data_);

    orig_ = std::move(other.orig_);

    return *this;
  }

  ~PatchRaw()
  {
    RemoveUnchecked();
  }

  bool IsApplied() const noexcept
  {
    return applied_;
  }

  void Apply()
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

    SuspendedProcess const suspended_process{process_.GetId()};

    detail::VerifyPatchThreads(process_.GetId(), target_, data_.size());

    orig_ = ReadVector<std::uint8_t>(process_, target_, data_.size());

    WriteVector(process_, target_, data_);

    FlushInstructionCache(process_, target_, data_.size());

    applied_ = true;
  }

  void Remove()
  {
    if (!applied_)
    {
      return;
    }

    SuspendedProcess const suspended_process{process_.GetId()};

    detail::VerifyPatchThreads(process_.GetId(), target_, data_.size());

    WriteVector(process_, target_, orig_);

    FlushInstructionCache(process_, target_, orig_.size());

    applied_ = false;
  }

  void Detach()
  {
    applied_ = false;

    detached_ = true;
  }

private:
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

      applied_ = false;

      target_ = nullptr;
      data_.clear();
      orig_.clear();
    }
  }

  Process process_;
  bool applied_{false};
  bool detached_{false};
  PVOID target_;
  std::vector<BYTE> data_;
  std::vector<std::uint8_t> orig_;
};
}
