// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <condition_variable>
#include <mutex>
#include <vector>

#include <windows.h>

#include "../cerberus/plugin.hpp"

struct Vec3f
{
  float x, z, y;
};

struct RadarData
{
  struct LocalPlayerData
  {
    Vec3f pos_;
    double heading_;
  };

  LocalPlayerData player_;

  struct UnitData
  {
    Vec3f pos_;
    COLORREF colour_;
    char const* name_;
  };

  std::vector<UnitData> units_;
};

void InitializeRadar(hadesmem::cerberus::PluginInterface* cerberus);

void CleanupRadar(hadesmem::cerberus::PluginInterface* cerberus);

std::mutex& GetRadarFrameMutex();

std::condition_variable& GetRadarFrameConditionVariable();

bool& GetRadarFrameProcessed();

bool& GetRadarEnabled();
