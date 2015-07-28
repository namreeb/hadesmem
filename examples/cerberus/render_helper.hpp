// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <string>

#include <windows.h>

#include <hadesmem/config.hpp>

#define CERBERUS_DETAIL_RENDER_HELPER_MAP_NAME L"Local\\CerberusHelper_Render_"

namespace hadesmem
{
namespace cerberus
{
struct D3D9Offsets
{
  std::uintptr_t add_ref_;
  std::uintptr_t release_;
  std::uintptr_t present_;
  std::uintptr_t reset_;
  std::uintptr_t end_scene_;
  std::uintptr_t present_ex_;
  std::uintptr_t reset_ex_;
  std::uintptr_t swap_chain_present_;
};

struct DXGIOffsets
{
  std::uintptr_t present_;
  std::uintptr_t resize_buffers_;
  std::uintptr_t present_1_;
};

struct RenderOffsets
{
  D3D9Offsets d3d9_offsets_;
  DXGIOffsets dxgi_offsets_;
};

inline std::wstring GenerateRenderHelperMapName(DWORD pid)
{
  return CERBERUS_DETAIL_RENDER_HELPER_MAP_NAME + std::to_wstring(pid);
}

inline std::wstring GenerateRenderHelperMapName(std::wstring pid)
{
  return CERBERUS_DETAIL_RENDER_HELPER_MAP_NAME + pid;
}

class RenderHelperInterface
{
public:
  virtual ~RenderHelperInterface()
  {
  }

  virtual RenderOffsets const* GetRenderOffsets() = 0;
};

RenderHelperInterface& GetRenderHelperInterface() noexcept;

void InitializeRenderHelper();
}
}
