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
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>
#include <hadesmem/write.hpp>

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
    : process_{&process},
      class_base_{static_cast<void***>(target_class)},
      old_vmt_{Read<void**>(process, class_base_)},
      vmt_size_{vmt_size},
      new_vmt_{process, vmt_size_ * sizeof(void*)}
  {
    HADESMEM_DETAIL_ASSERT(vmt_size_);
    auto const old_vmt_contents =
      ReadVector<void*>(process, old_vmt_, vmt_size);
    WriteVector(process, new_vmt_.GetBase(), old_vmt_contents);
  }

  PatchVmt(hadesmem::Process const& process, void* target_class, TagUnsafe)
    : process_{&process},
      class_base_{static_cast<void***>(target_class)},
      old_vmt_{Read<void**>(process, class_base_)},
      vmt_size_{GetVmtSizeUnsafe(old_vmt_)},
      new_vmt_{process, vmt_size_ * sizeof(void*)}
  {
    HADESMEM_DETAIL_ASSERT(vmt_size_);
    auto const old_vmt_contents =
      ReadVector<void*>(process, old_vmt_, vmt_size_);
    WriteVector(process, new_vmt_.GetBase(), old_vmt_contents);
  }

  explicit PatchVmt(Process&& process,
                    void* target_class,
                    std::size_t vmt_size) = delete;

  explicit PatchVmt(Process&& process, void* target_class, TagUnsafe) = delete;

  PatchVmt(PatchVmt const& other) = delete;

  PatchVmt& operator=(PatchVmt const& other) = delete;

  PatchVmt(PatchVmt&& other)
    : process_{other.process_},
      class_base_{other.class_base_},
      old_vmt_{other.old_vmt_},
      vmt_size_(other.vmt_size_),
      new_vmt_(std::move(other.new_vmt_))
  {
    other.process_ = nullptr;
    other.class_base_ = false;
    other.old_vmt_ = nullptr;
    other.vmt_size_ = 0;
  }

  PatchVmt& operator=(PatchVmt&& other)
  {
    RemoveUnchecked();

    process_ = other.process_;
    other.process_ = nullptr;

    class_base_ = other.class_base_;
    other.class_base_ = nullptr;

    old_vmt_ = other.old_vmt_;
    other.old_vmt_ = nullptr;

    vmt_size_ = other.vmt_size_;
    other.vmt_size_ = 0;

    new_vmt_ = std::move(other.new_vmt_);

    return *this;
  }

  ~PatchVmt()
  {
    RemoveUnchecked();
  }

  void Apply()
  {
    HADESMEM_DETAIL_ASSERT(class_base_);
    HADESMEM_DETAIL_ASSERT(new_vmt_.GetBase());
    Write(*process_, class_base_, new_vmt_.GetBase());
  }

  void Remove()
  {
    HADESMEM_DETAIL_ASSERT(class_base_);
    HADESMEM_DETAIL_ASSERT(new_vmt_.GetBase());
    Write(*process_, class_base_, old_vmt_);
  }

  std::size_t GetSize()
  {
    return vmt_size_;
  }

  void* HookMethod(std::size_t idx, void* new_func)
  {
    HADESMEM_DETAIL_ASSERT(vmt_size_);
    HADESMEM_DETAIL_ASSERT(old_vmt_);
    HADESMEM_DETAIL_ASSERT(new_vmt_.GetBase());
    Write(*process_, static_cast<void**>(new_vmt_.GetBase()) + idx, new_func);
    return Read<void*>(*process_, old_vmt_ + idx);
  }

private:
  void RemoveUnchecked() HADESMEM_DETAIL_NOEXCEPT
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

  std::size_t GetVmtSizeUnsafe(void** vmt)
  {
    std::size_t i = 0;
    void* f = Read<void*>(*process_, vmt + i);
    for (; f; ++i)
    {
      try
      {
        if (!hadesmem::CanExecute(*process_, f))
        {
          break;
        }
      }
      catch (std::exception const& /*e*/)
      {
        break;
      }

      f = Read<void*>(*process_, vmt + i);
    }

    return i;
  }

  hadesmem::Process const* process_{};
  void*** class_base_{};
  void** old_vmt_{};
  std::size_t vmt_size_{};
  Allocator new_vmt_;
};
}
