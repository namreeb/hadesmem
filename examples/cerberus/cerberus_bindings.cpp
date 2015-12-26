// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "cerberus_bindings.hpp"

#include <hadesmem/detail/trace.hpp>

#include "render.hpp"
#include "imgui.hpp"
#include "imgui_log.hpp"

namespace hadesmem
{
namespace cerberus
{
chaiscript::ModulePtr GetCerberusModule()
{
  chaiscript::ModulePtr chai_mod(new chaiscript::Module());

  chai_mod->add(chaiscript::fun([](std::string const& s)
                                {
                                  auto& log = GetImGuiLogWindow();
                                  log.AddLog("%s\n", s.c_str());

                                  HADESMEM_DETAIL_TRACE_A(s.c_str());
                                }),
                "Log");

  chai_mod->add(chaiscript::user_type<RenderApi>(), "RenderApi");
  chai_mod->add_global_const(chaiscript::const_var(RenderApi::kD3D9),
                             "RenderApi_kD3D9");
  chai_mod->add_global_const(chaiscript::const_var(RenderApi::kD3D10),
                             "RenderApi_kD3D10");
  chai_mod->add_global_const(chaiscript::const_var(RenderApi::kD3D11),
                             "RenderApi_kD3D11");
  chai_mod->add_global_const(chaiscript::const_var(RenderApi::kOpenGL32),
                             "RenderApi_kOpenGL32");
  chai_mod->add_global_const(chaiscript::const_var(RenderApi::kInvalidMaxValue),
                             "RenderApi_kInvalidMaxValue");

  chai_mod->add(chaiscript::user_type<RenderInterface>(), "RenderInterface");
  auto const register_on_frame = [](
    RenderInterface* render,
    std::function<void(RenderApi, std::uintptr_t)> const& callback)
    -> std::size_t
  {
    return render->RegisterOnFrame2(
      [callback](RenderApi api, void* device)
      {
        return callback(api, reinterpret_cast<std::uintptr_t>(device));
      });
    ;
  };
  chai_mod->add(chaiscript::fun(register_on_frame), "RegisterOnFrame");
  chai_mod->add(chaiscript::fun(&RenderInterface::UnregisterOnFrame2),
                "UnregisterOnFrame");
  auto const register_on_resize = [](
    RenderInterface* render,
    std::function<void(RenderApi, std::uintptr_t, UINT, UINT)> const& callback)
    -> std::size_t
  {
    return render->RegisterOnResize(
      [callback](RenderApi api, void* device, UINT width, UINT height)
      {
        return callback(
          api, reinterpret_cast<std::uintptr_t>(device), width, height);
      });
    ;
  };
  chai_mod->add(chaiscript::fun(register_on_resize), "RegisterOnResize");
  chai_mod->add(chaiscript::fun(&RenderInterface::UnregisterOnResize),
                "UnregisterOnResize");
  chai_mod->add(chaiscript::fun(&RenderInterface::RegisterOnSetGuiVisibility),
                "RegisterOnSetGuiVisibility");
  chai_mod->add(chaiscript::fun(&RenderInterface::UnregisterOnSetGuiVisibility),
                "UnregisterOnSetGuiVisibility");
  auto const register_on_initialize_gui = [](
    RenderInterface* render,
    std::function<void(RenderApi, std::uintptr_t)> const& callback)
    -> std::size_t
  {
    return render->RegisterOnInitializeGui(
      [callback](RenderApi api, void* device)
      {
        return callback(api, reinterpret_cast<std::uintptr_t>(device));
      });
    ;
  };
  chai_mod->add(chaiscript::fun(register_on_initialize_gui),
                "RegisterOnInitializeGui");
  chai_mod->add(chaiscript::fun(&RenderInterface::UnregisterOnInitializeGui),
                "UnregisterOnInitializeGui");
  chai_mod->add(chaiscript::fun(&RenderInterface::RegisterOnCleanupGui),
                "RegisterOnCleanupGui");
  chai_mod->add(chaiscript::fun(&RenderInterface::UnregisterOnCleanupGui),
                "UnregisterOnCleanupGui");

  chai_mod->add(chaiscript::fun(&GetRenderInterface), "GetRenderInterface");

  chai_mod->add(chaiscript::fun(&GetGuiVisible), "GetGuiVisible");
  chai_mod->add(chaiscript::fun(&SetGuiVisible), "SetGuiVisible");

  chai_mod->add(chaiscript::user_type<ImguiInterface>(), "ImguiInterface");
  chai_mod->add(chaiscript::fun(&ImguiInterface::RegisterOnInitialize),
                "RegisterOnInitialize");
  chai_mod->add(chaiscript::fun(&ImguiInterface::UnregisterOnInitialize),
                "UnregisterOnInitialize");
  chai_mod->add(chaiscript::fun(&ImguiInterface::UnregisterOnInitialize),
                "RegisterOnCleanup");
  chai_mod->add(chaiscript::fun(&ImguiInterface::UnregisterOnInitialize),
                "UnregisterOnCleanup");
  chai_mod->add(chaiscript::fun(&ImguiInterface::RegisterOnFrame),
                "RegisterOnFrame");
  chai_mod->add(chaiscript::fun(&ImguiInterface::UnregisterOnFrame),
                "UnregisterOnFrame");
  chai_mod->add(chaiscript::fun(&ImguiInterface::IsInitialized),
                "IsInitialized");

  chai_mod->add(chaiscript::fun(&GetImguiInterface), "GetImguiInterface");

  return chai_mod;
}
}
}
