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

class AntTweakBarInterface
{
public:
  virtual ~AntTweakBarInterface()
  {
  }

  virtual TwBar* TwNewBar(const char* bar_name) = 0;

  virtual int TwAddButton(TwBar* bar,
                          const char* name,
                          TwButtonCallback callback,
                          void* client_data,
                          const char* def) = 0;
};

typedef void OnFrameCallback();

typedef void
  OnAntTweakBarInitializeCallback(AntTweakBarInterface* ant_tweak_bar);

typedef void OnAntTweakBarCleanupCallback(AntTweakBarInterface* ant_tweak_bar);

class RenderInterface
{
public:
  virtual ~RenderInterface()
  {
  }

  virtual std::size_t
    RegisterOnFrame(std::function<OnFrameCallback> const& callback) = 0;

  virtual void UnregisterOnFrame(std::size_t id) = 0;

  virtual std::size_t RegisterOnAntTweakBarInitialize(
    std::function<OnAntTweakBarInitializeCallback> const& callback) = 0;

  virtual void UnregisterOnAntTweakBarInitialize(std::size_t id) = 0;

  virtual std::size_t RegisterOnAntTweakBarCleanup(
    std::function<OnAntTweakBarCleanupCallback> const& callback) = 0;

  virtual void UnregisterOnAntTweakBarCleanup(std::size_t id) = 0;
};

RenderInterface& GetRenderInterface() HADESMEM_DETAIL_NOEXCEPT;

void InitializeRender();

std::size_t
  RegisterOnFrameCallback(std::function<OnFrameCallback> const& callback);

void UnregisterOnFrameCallback(std::size_t id);

std::size_t RegisterOnAntTweakBarInitializeCallback(
  std::function<OnAntTweakBarInitializeCallback> const& callback);

void UnregisterOnAntTweakBarInitializeCallback(std::size_t id);

std::size_t RegisterOnAntTweakBarCleanupCallback(
  std::function<OnAntTweakBarCleanupCallback> const& callback);

void UnregisterOnAntTweakBarCleanupCallback(std::size_t id);
}
}
