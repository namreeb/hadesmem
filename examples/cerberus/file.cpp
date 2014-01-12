// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <iterator>
#include <memory>
#include <string>

#include <windows.h>
#include <winnt.h>
#include <winternl.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/detail/last_error.hpp>
#include <hadesmem/find_procedure.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/patcher.hpp>
#include <hadesmem/process.hpp>

#include "main.hpp"

namespace winternl = hadesmem::detail::winternl;

namespace
{

std::unique_ptr<hadesmem::PatchDetour>& GetNtQueryDirectoryFileDetour()
{
  static std::unique_ptr<hadesmem::PatchDetour> detour;
  return detour;
}

template <winternl::FILE_INFORMATION_CLASS kInfoClass> struct InfoClassToBuffer;

template <> struct InfoClassToBuffer<winternl::FileDirectoryInformation>
{
  using type = winternl::FILE_DIRECTORY_INFORMATION;
};

template <> struct InfoClassToBuffer<winternl::FileFullDirectoryInformation>
{
  using type = winternl::FILE_FULL_DIR_INFORMATION;
};

template <> struct InfoClassToBuffer<winternl::FileBothDirectoryInformation>
{
  using type = winternl::FILE_BOTH_DIR_INFORMATION;
};

template <> struct InfoClassToBuffer<winternl::FileNamesInformation>
{
  using type = winternl::FILE_NAMES_INFORMATION;
};

template <> struct InfoClassToBuffer<winternl::FileIdBothDirectoryInformation>
{
  using type = winternl::FILE_ID_BOTH_DIR_INFORMATION;
};

template <> struct InfoClassToBuffer<winternl::FileIdFullDirectoryInformation>
{
  using type = winternl::FILE_ID_FULL_DIR_INFORMATION;
};

template <winternl::FILE_INFORMATION_CLASS kInfoClass>
using InfoClassToBufferT = typename InfoClassToBuffer<kInfoClass>::type;

template <winternl::FILE_INFORMATION_CLASS kInfoClass,
          typename BufferT = InfoClassToBufferT<kInfoClass>>
class DirectoryFileInformationEnum
{
public:
  explicit DirectoryFileInformationEnum(
    void* buffer, ULONG buffer_len, NTSTATUS* status) HADESMEM_DETAIL_NOEXCEPT
    : beg_(static_cast<BufferT*>(buffer)),
      buffer_(static_cast<BufferT*>(buffer)),
      prev_(nullptr),
      buffer_len_(buffer_len),
      unlinked_(false),
      status_(status)
  {
    HADESMEM_DETAIL_ASSERT(buffer_);
  }

  DirectoryFileInformationEnum(DirectoryFileInformationEnum const&) = delete;

  DirectoryFileInformationEnum& operator=(DirectoryFileInformationEnum const&) =
    delete;

  void Advance() HADESMEM_DETAIL_NOEXCEPT
  {
    if (!unlinked_)
    {
      prev_ = buffer_;
    }

    unlinked_ = false;

    if (buffer_->NextEntryOffset)
    {
      buffer_ = reinterpret_cast<BufferT*>(
        reinterpret_cast<DWORD_PTR>(buffer_) + buffer_->NextEntryOffset);
    }
    else
    {
      buffer_ = nullptr;
    }
  }

  bool IsValid() const HADESMEM_DETAIL_NOEXCEPT
  {
    return buffer_ != nullptr;
  }

  void Unlink() HADESMEM_DETAIL_NOEXCEPT
  {
    HADESMEM_DETAIL_ASSERT(buffer_);

    HADESMEM_DETAIL_ASSERT(!unlinked_);

    if (prev_)
    {
      if (buffer_->NextEntryOffset)
      {
        prev_->NextEntryOffset += buffer_->NextEntryOffset;
      }
      else
      {
        prev_->NextEntryOffset = 0UL;
      }

      unlinked_ = true;
    }
    else
    {
      if (buffer_->NextEntryOffset)
      {
        auto in_beg =
          reinterpret_cast<unsigned char*>(buffer_) + buffer_->NextEntryOffset;
        ;
        auto in_end = reinterpret_cast<unsigned char*>(beg_) + buffer_len_ -
                      reinterpret_cast<DWORD_PTR>(in_beg);
        auto out_beg = reinterpret_cast<unsigned char*>(buffer_);
        std::copy(in_beg, in_end, out_beg);
      }
      else
      {
        *status_ = HADESMEM_DETAIL_STATUS_NO_SUCH_FILE;
      }
    }
  }

  bool HasName() const HADESMEM_DETAIL_NOEXCEPT
  {
    return buffer_->FileNameLength != 0;
  }

  std::wstring GetName() const
  {
    return {buffer_->FileName,
            buffer_->FileName + (buffer_->FileNameLength / sizeof(WCHAR))};
  }

  NTSTATUS GetStatus() const
  {
    return status_;
  }

private:
  BufferT* beg_;
  BufferT* buffer_;
  BufferT* prev_;
  ULONG buffer_len_;
  bool unlinked_;
  NTSTATUS* status_;
};

template <winternl::FILE_INFORMATION_CLASS kInfoClass,
          typename BufferT = InfoClassToBufferT<kInfoClass>>
void EnumFiles(PVOID file_information, ULONG length, NTSTATUS* status)
{
  HADESMEM_DETAIL_ASSERT(length >= sizeof(BufferT));

  HADESMEM_DETAIL_TRACE_A("Enumerating files.");
  for (DirectoryFileInformationEnum<kInfoClass, BufferT> directory_info{
         file_information, length, status};
       directory_info.IsValid();
       directory_info.Advance())
  {
    if (directory_info.HasName())
    {
      auto const file_name = directory_info.GetName();
      HADESMEM_DETAIL_TRACE_FORMAT_W(L"Name: [%s].", file_name.c_str());
      if (file_name == L"hades.exe")
      {
        HADESMEM_DETAIL_TRACE_A("Unlinking file.");
        directory_info.Unlink();
      }
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("WARNING! Invalid name.");
    }
  }
}

extern "C" NTSTATUS WINAPI NtQueryDirectoryFileDetour(
  HANDLE file_handle,
  HANDLE event,
  PIO_APC_ROUTINE apc_routine,
  PVOID apc_context,
  PIO_STATUS_BLOCK io_status_block,
  PVOID file_information,
  ULONG length,
  winternl::FILE_INFORMATION_CLASS file_information_class,
  BOOLEAN return_single_entry,
  PUNICODE_STRING file_name,
  BOOLEAN restart_scan) HADESMEM_DETAIL_NOEXCEPT
{
  hadesmem::detail::LastErrorPreserver last_error;
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Args: [%p] [%p] [%p] [%p] [%p] [%p] [%lu] [%d] [%d] [%p] [%d].",
    file_handle,
    event,
    apc_routine,
    apc_context,
    io_status_block,
    file_information,
    length,
    file_information_class,
    return_single_entry,
    file_name,
    restart_scan);
  auto& detour = GetNtQueryDirectoryFileDetour();
  auto const nt_query_directory_file =
    detour->GetTrampoline<decltype(&NtQueryDirectoryFileDetour)>();
  last_error.Revert();
  auto ret = nt_query_directory_file(file_handle,
                                     event,
                                     apc_routine,
                                     apc_context,
                                     io_status_block,
                                     file_information,
                                     length,
                                     file_information_class,
                                     return_single_entry,
                                     file_name,
                                     restart_scan);
  last_error.Update();
  HADESMEM_DETAIL_TRACE_FORMAT_A("Ret: [%ld].", ret);

  if (file_information_class != winternl::FileDirectoryInformation &&
      file_information_class != winternl::FileFullDirectoryInformation &&
      file_information_class != winternl::FileBothDirectoryInformation &&
      file_information_class != winternl::FileNamesInformation &&
      file_information_class != winternl::FileIdBothDirectoryInformation &&
      file_information_class != winternl::FileIdFullDirectoryInformation)
  {
    HADESMEM_DETAIL_TRACE_A("WARNING! Unhandled information class.");
    return ret;
  }

  if (apc_routine)
  {
    HADESMEM_DETAIL_TRACE_A("WARNING! Unhandled asynchronous call.");
    return ret;
  }

  if (!NT_SUCCESS(ret))
  {
    HADESMEM_DETAIL_TRACE_A("Trampoline returned failure.");
    return ret;
  }

  try
  {
    for (;;)
    {
      switch (file_information_class)
      {
      case winternl::FileDirectoryInformation:
        EnumFiles<winternl::FileDirectoryInformation>(
          file_information, length, &ret);
        break;
      case winternl::FileFullDirectoryInformation:
        EnumFiles<winternl::FileFullDirectoryInformation>(
          file_information, length, &ret);
        break;
      case winternl::FileBothDirectoryInformation:
        EnumFiles<winternl::FileBothDirectoryInformation>(
          file_information, length, &ret);
        break;
      case winternl::FileNamesInformation:
        EnumFiles<winternl::FileNamesInformation>(
          file_information, length, &ret);
        break;
      case winternl::FileIdBothDirectoryInformation:
        EnumFiles<winternl::FileIdBothDirectoryInformation>(
          file_information, length, &ret);
        break;
      case winternl::FileIdFullDirectoryInformation:
        EnumFiles<winternl::FileIdFullDirectoryInformation>(
          file_information, length, &ret);
        break;
      default:
        HADESMEM_DETAIL_ASSERT(false);
        break;
      }

      // In the case where a single file is requested and we hide it, we try
      // again in case there are more entries to be returned, or if the call
      // to the original API fails we simply return that error.
      if (ret == HADESMEM_DETAIL_STATUS_NO_SUCH_FILE && return_single_entry)
      {
        ret = nt_query_directory_file(file_handle,
                                      event,
                                      apc_routine,
                                      apc_context,
                                      io_status_block,
                                      file_information,
                                      length,
                                      file_information_class,
                                      return_single_entry,
                                      file_name,
                                      FALSE);
        if (!NT_SUCCESS(ret))
        {
          break;
        }
      }
      // No need to retry.
      else
      {
        break;
      }
    }
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }

  return ret;
}
}

void DetourNtQueryDirectoryFile()
{
  hadesmem::Module const ntdll(GetThisProcess(), L"ntdll.dll");
  auto const nt_query_directory_file =
    hadesmem::FindProcedure(GetThisProcess(), ntdll, "NtQueryDirectoryFile");
  auto const nt_query_directory_file_ptr =
    hadesmem::detail::UnionCast<void*>(nt_query_directory_file);
  auto const nt_query_directory_file_detour =
    hadesmem::detail::UnionCast<void*>(&NtQueryDirectoryFileDetour);
  auto& detour = GetNtQueryDirectoryFileDetour();
  detour.reset(new hadesmem::PatchDetour(GetThisProcess(),
                                         nt_query_directory_file_ptr,
                                         nt_query_directory_file_detour));
  detour->Apply();
  HADESMEM_DETAIL_TRACE_A("NtQueryDirectoryFile detoured.");
}
