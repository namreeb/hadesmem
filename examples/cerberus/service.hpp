// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <functional>

#include <windows.h>
#include <winternl.h>

#include <hadesmem/config.hpp>

namespace hadesmem
{
namespace cerberus
{
typedef void OnCreateServiceACallback(SC_HANDLE sc_manager,
                                      LPCSTR service_name,
                                      LPCSTR display_name,
                                      DWORD desired_access,
                                      DWORD service_type,
                                      DWORD start_type,
                                      DWORD error_control,
                                      LPCSTR binary_path_name,
                                      LPCSTR load_order_group,
                                      LPDWORD tag_id,
                                      LPCSTR dependencies,
                                      LPCSTR service_start_name,
                                      LPCSTR password,
                                      bool* handled);

typedef void OnCreateServiceWCallback(SC_HANDLE sc_manager,
                                      LPCWSTR service_name,
                                      LPCWSTR display_name,
                                      DWORD desired_access,
                                      DWORD service_type,
                                      DWORD start_type,
                                      DWORD error_control,
                                      LPCWSTR binary_path_name,
                                      LPCWSTR load_order_group,
                                      LPDWORD tag_id,
                                      LPCWSTR dependencies,
                                      LPCWSTR service_start_name,
                                      LPCWSTR password,
                                      bool* handled);

typedef void OnOpenServiceACallback(SC_HANDLE sc_manager,
                                    LPCSTR service_name,
                                    DWORD desired_access,
                                    bool* handled);

typedef void OnOpenServiceWCallback(SC_HANDLE sc_manager,
                                    LPCWSTR service_name,
                                    DWORD desired_access,
                                    bool* handled);

class ServiceInterface
{
public:
  virtual ~ServiceInterface()
  {
  }

  virtual std::size_t RegisterOnCreateServiceA(
    std::function<OnCreateServiceACallback> const& callback) = 0;

  virtual void UnregisterOnCreateServiceA(std::size_t id) = 0;

  virtual std::size_t RegisterOnCreateServiceW(
    std::function<OnCreateServiceWCallback> const& callback) = 0;

  virtual void UnregisterOnCreateServiceW(std::size_t id) = 0;

  virtual std::size_t RegisterOnOpenServiceA(
    std::function<OnOpenServiceACallback> const& callback) = 0;

  virtual void UnregisterOnOpenServiceA(std::size_t id) = 0;

  virtual std::size_t RegisterOnOpenServiceW(
    std::function<OnOpenServiceWCallback> const& callback) = 0;

  virtual void UnregisterOnOpenServiceW(std::size_t id) = 0;
};

ServiceInterface& GetServiceInterface() noexcept;

void InitializeService();

void DetourSechostForService(HMODULE base);

void UndetourSechostForService(bool remove);
}
}
