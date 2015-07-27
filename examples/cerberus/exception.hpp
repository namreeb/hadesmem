// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <functional>

#include <windows.h>

#include <hadesmem/config.hpp>

namespace hadesmem
{
namespace cerberus
{
typedef void OnRtlAddVectoredExceptionHandlerCallback(
  ULONG first_handler,
  PVECTORED_EXCEPTION_HANDLER vectored_handler,
  bool* handled);

typedef void OnSetUnhandledExceptionFilterCallback(
  LPTOP_LEVEL_EXCEPTION_FILTER top_level_exception_filter, bool* handled);

class ExceptionInterface
{
public:
  virtual ~ExceptionInterface()
  {
  }

  virtual std::size_t RegisterOnRtlAddVectoredExceptionHandler(std::function<
    OnRtlAddVectoredExceptionHandlerCallback> const& callback) = 0;

  virtual void UnregisterOnRtlAddVectoredExceptionHandler(std::size_t id) = 0;

  virtual std::size_t RegisterOnSetUnhandledExceptionFilter(
    std::function<OnSetUnhandledExceptionFilterCallback> const& callback) = 0;

  virtual void UnregisterOnSetUnhandledExceptionFilter(std::size_t id) = 0;
};

ExceptionInterface& GetExceptionInterface() noexcept;

void InitializeException();

void DetourNtdllForException(HMODULE base);

void DetourKernelBaseForException(HMODULE base);

void UndetourNtdllForException(bool remove);

void UndetourKernelBaseForException(bool remove);
}
}
