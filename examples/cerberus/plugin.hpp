// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include "d3d11.hpp"

void LoadPlugins();

void UnloadPlugins();

struct CerberusInterface
{
  virtual std::size_t RegisterOnFrameCallback(std::function<OnFrameCallback> const& callback) = 0;

  virtual void UnregisterOnFrameCallback(std::size_t id) = 0;
};
