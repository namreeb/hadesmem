// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <utility>
#include <functional>

#include <windows.h>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <anttweakbar.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>

namespace hadesmem
{
namespace cerberus
{
class AntTweakBarInterface;

typedef void
  OnAntTweakBarInitializeCallback(AntTweakBarInterface* ant_tweak_bar);

typedef void OnAntTweakBarCleanupCallback(AntTweakBarInterface* ant_tweak_bar);

class AntTweakBarInterface
{
public:
  virtual ~AntTweakBarInterface()
  {
  }

  virtual std::size_t RegisterOnInitialize(
    std::function<OnAntTweakBarInitializeCallback> const& callback) = 0;

  virtual void UnregisterOnInitialize(std::size_t id) = 0;

  virtual std::size_t RegisterOnCleanup(
    std::function<OnAntTweakBarCleanupCallback> const& callback) = 0;

  virtual void UnregisterOnCleanup(std::size_t id) = 0;

  virtual bool IsInitialized() = 0;

  virtual TwBar* TwNewBar(const char* bar_name) = 0;

  virtual int TwDeleteBar(TwBar* bar) = 0;

  virtual int TwAddButton(TwBar* bar,
                          const char* name,
                          TwButtonCallback callback,
                          void* client_data,
                          const char* def) = 0;

  virtual int TwAddVarRW(
    TwBar* bar, const char* name, TwType type, void* var, const char* def) = 0;

  virtual char const* TwGetLastError() = 0;
};

AntTweakBarInterface& GetAntTweakBarInterface() HADESMEM_DETAIL_NOEXCEPT;

typedef void OnFrameCallback();

class RenderInterface
{
public:
  virtual ~RenderInterface()
  {
  }

  virtual std::size_t
    RegisterOnFrame(std::function<OnFrameCallback> const& callback) = 0;

  virtual void UnregisterOnFrame(std::size_t id) = 0;

  virtual AntTweakBarInterface* GetAntTweakBarInterface() = 0;
};

RenderInterface& GetRenderInterface() HADESMEM_DETAIL_NOEXCEPT;

void InitializeRender();
}
}
