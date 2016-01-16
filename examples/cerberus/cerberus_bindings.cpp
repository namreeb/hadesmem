// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "cerberus_bindings.hpp"

#include <hadesmem/detail/trace.hpp>

#include "render.hpp"
#include "imgui.hpp"
#include "imgui_log.hpp"

// TODO: Also expose hadesmem API. Try and replace a basic cerberus extension
// with a script (as much as possible) and prioritize exporting the most
// important APIs first.

namespace
{
chaiscript::ModulePtr GetRenderModule()
{
  chaiscript::ModulePtr mod(new chaiscript::Module());

  mod->add(chaiscript::user_type<hadesmem::cerberus::RenderApi>(),
           "Cerberus_RenderApi");
  mod->add_global_const(
    chaiscript::const_var(hadesmem::cerberus::RenderApi::kD3D9),
    "Cerberus_RenderApi_kD3D9");
  mod->add_global_const(
    chaiscript::const_var(hadesmem::cerberus::RenderApi::kD3D10),
    "Cerberus_RenderApi_kD3D10");
  mod->add_global_const(
    chaiscript::const_var(hadesmem::cerberus::RenderApi::kD3D11),
    "Cerberus_RenderApi_kD3D11");
  mod->add_global_const(
    chaiscript::const_var(hadesmem::cerberus::RenderApi::kOpenGL32),
    "Cerberus_RenderApi_kOpenGL32");
  mod->add_global_const(
    chaiscript::const_var(hadesmem::cerberus::RenderApi::kInvalidMaxValue),
    "Cerberus_RenderApi_kInvalidMaxValue");

  mod->add(chaiscript::user_type<hadesmem::cerberus::RenderInterface>(),
           "Cerberus_RenderInterface");
  auto const register_on_frame = [](
    hadesmem::cerberus::RenderInterface* render,
    std::function<void(hadesmem::cerberus::RenderApi, std::uintptr_t)> const&
      callback) -> std::size_t
  {
    return render->RegisterOnFrame2(
      [callback](hadesmem::cerberus::RenderApi api, void* device)
      {
        try
        {
          return callback(api, reinterpret_cast<std::uintptr_t>(device));
        }
        catch (...)
        {
          // TODO: Add this to all other callback registration functions.
          auto& imgui = hadesmem::cerberus::GetImguiInterface();
          imgui.Log(boost::current_exception_diagnostic_information());
        }
      });
    ;
  };
  mod->add(chaiscript::fun(register_on_frame), "RegisterOnFrame");
  mod->add(
    chaiscript::fun(&hadesmem::cerberus::RenderInterface::UnregisterOnFrame2),
    "UnregisterOnFrame");
  auto const register_on_resize = [](
    hadesmem::cerberus::RenderInterface* render,
    std::function<void(
      hadesmem::cerberus::RenderApi, std::uintptr_t, UINT, UINT)> const&
      callback) -> std::size_t
  {
    return render->RegisterOnResize(
      [callback](hadesmem::cerberus::RenderApi api,
                 void* device,
                 UINT width,
                 UINT height)
      {
        return callback(
          api, reinterpret_cast<std::uintptr_t>(device), width, height);
      });
    ;
  };
  mod->add(chaiscript::fun(register_on_resize), "RegisterOnResize");
  mod->add(
    chaiscript::fun(&hadesmem::cerberus::RenderInterface::UnregisterOnResize),
    "UnregisterOnResize");
  mod->add(chaiscript::fun(
             &hadesmem::cerberus::RenderInterface::RegisterOnSetGuiVisibility),
           "RegisterOnSetGuiVisibility");
  mod->add(
    chaiscript::fun(
      &hadesmem::cerberus::RenderInterface::UnregisterOnSetGuiVisibility),
    "UnregisterOnSetGuiVisibility");
  auto const register_on_initialize_gui = [](
    hadesmem::cerberus::RenderInterface* render,
    std::function<void(hadesmem::cerberus::RenderApi, std::uintptr_t)> const&
      callback) -> std::size_t
  {
    return render->RegisterOnInitializeGui(
      [callback](hadesmem::cerberus::RenderApi api, void* device)
      {
        return callback(api, reinterpret_cast<std::uintptr_t>(device));
      });
    ;
  };
  mod->add(chaiscript::fun(register_on_initialize_gui),
           "RegisterOnInitializeGui");
  mod->add(chaiscript::fun(
             &hadesmem::cerberus::RenderInterface::UnregisterOnInitializeGui),
           "UnregisterOnInitializeGui");
  mod->add(
    chaiscript::fun(&hadesmem::cerberus::RenderInterface::RegisterOnCleanupGui),
    "RegisterOnCleanupGui");
  mod->add(chaiscript::fun(
             &hadesmem::cerberus::RenderInterface::UnregisterOnCleanupGui),
           "UnregisterOnCleanupGui");

  mod->add(chaiscript::fun(&hadesmem::cerberus::GetRenderInterface),
           "Cerberus_GetRenderInterface");

  mod->add(chaiscript::fun(&hadesmem::cerberus::GetGuiVisible),
           "Cerberus_GetGuiVisible");
  mod->add(chaiscript::fun(&hadesmem::cerberus::SetGuiVisible),
           "Cerberus_SetGuiVisible");

  return mod;
}

chaiscript::ModulePtr GetImguiModule()
{
  chaiscript::ModulePtr mod(new chaiscript::Module());

  mod->add(chaiscript::user_type<hadesmem::cerberus::ImguiInterface>(),
           "Cerberus_ImguiInterface");
  mod->add(
    chaiscript::fun(&hadesmem::cerberus::ImguiInterface::RegisterOnInitialize),
    "RegisterOnInitialize");
  mod->add(chaiscript::fun(
             &hadesmem::cerberus::ImguiInterface::UnregisterOnInitialize),
           "UnregisterOnInitialize");
  mod->add(chaiscript::fun(
             &hadesmem::cerberus::ImguiInterface::UnregisterOnInitialize),
           "RegisterOnCleanup");
  mod->add(chaiscript::fun(
             &hadesmem::cerberus::ImguiInterface::UnregisterOnInitialize),
           "UnregisterOnCleanup");
  mod->add(
    chaiscript::fun(&hadesmem::cerberus::ImguiInterface::RegisterOnFrame),
    "RegisterOnFrame");
  mod->add(
    chaiscript::fun(&hadesmem::cerberus::ImguiInterface::UnregisterOnFrame),
    "UnregisterOnFrame");
  mod->add(chaiscript::fun(&hadesmem::cerberus::ImguiInterface::IsInitialized),
           "IsInitialized");

  mod->add(chaiscript::fun(&hadesmem::cerberus::GetImguiInterface),
           "Cerberus_GetImguiInterface");

  return mod;
}
}

namespace hadesmem
{
namespace cerberus
{
chaiscript::ModulePtr GetCerberusModule()
{
  chaiscript::ModulePtr chai_mod(new chaiscript::Module());

  chai_mod->add(chaiscript::fun([](std::string const& s)
                                {
                                  auto& imgui = GetImguiInterface();
                                  imgui.Log(s);
                                }),
                "Cerberus_Log");

  chai_mod->add(GetRenderModule());
  chai_mod->add(GetImguiModule());

  return chai_mod;
}
}
}
