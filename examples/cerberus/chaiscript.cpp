// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "chaiscript.hpp"

#include <string>
#include <vector>
#include <mutex>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <chaiscript/chaiscript_stdlib.hpp>
#include <chaiscript/dispatchkit/bootstrap.hpp>
#include <chaiscript/dispatchkit/bootstrap_stl.hpp>
#include <chaiscript/dispatchkit/dispatchkit.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/detail/filesystem.hpp>
#include <hadesmem/detail/str_conv.hpp>
#include <hadesmem/detail/trace.hpp>

#include "callbacks.hpp"
#include "cerberus_bindings.hpp"
#include "imgui.hpp"
#include "imgui_bindings.hpp"

// TODO: Decouple ChaiScript and ImGui components.

// TODO: Add support for automatically running a script at game startup (perhaps
// two modes, normal and fatal - noraml simply logs errors, fatal will call
// std::terminate).

// TODO: Expose the entire Cerberus and HadesMem APIs.

// TODO: Figure out what to do on extension unload (now that we support
// extensions adding their own ChaiScript bindings). Probably stop all running
// scripts and reset all contexts. What about on extension load though? Probably
// just reset the default contexts?

namespace
{
hadesmem::cerberus::Callbacks<
  hadesmem::cerberus::OnInitializeChaiScriptContextCallback>&
  GetOnInitializeChaiScriptContextCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnInitializeChaiScriptContextCallback> callbacks;
  return callbacks;
}

class ChaiScriptImpl : public hadesmem::cerberus::ChaiScriptInterface
{
public:
  virtual std::size_t RegisterOnInitializeChaiScriptContext(
    std::function<
      hadesmem::cerberus::OnInitializeChaiScriptContextCallback> const&
      callback) final
  {
    auto& callbacks = GetOnInitializeChaiScriptContextCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnInitializeChaiScriptContext(std::size_t id) final
  {
    auto& callbacks = GetOnInitializeChaiScriptContextCallbacks();
    return callbacks.Unregister(id);
  }

  virtual chaiscript::ChaiScript& GetGlobalContext() final
  {
    return hadesmem::cerberus::GetGlobalChaiScriptContext();
  }
};

void InitializeChaiScriptContext(chaiscript::ChaiScript& chai,
                                 bool run_callbacks)
{
  chai.add(chaiscript::bootstrap::standard_library::string_type<std::wstring>(
    "wstring"));

  chai.add(hadesmem::cerberus::GetCerberusModule());
  chai.add(hadesmem::cerberus::GetImGuiChaiScriptModule());

  if (run_callbacks)
  {
    auto const& callbacks = GetOnInitializeChaiScriptContextCallbacks();
    callbacks.Run(chai);
  }
}

std::unique_ptr<chaiscript::ChaiScript>& GetGlobalChaiScriptContextPtr()
{
  static auto chai =
    std::make_unique<chaiscript::ChaiScript>(chaiscript::Std_Lib::library());
  static std::once_flag once;
  std::call_once(once,
                 [&]()
                 {
                   InitializeChaiScriptContext(*chai, true);
                 });
  return chai;
}

void LogBoxedValue(chaiscript::ChaiScript& chai,
                   chaiscript::Boxed_Value const& val,
                   std::string const& name)
{
  if (!val.get_type_info().bare_equal(chaiscript::user_type<void>()))
  {
    try
    {
      using ToStringFn =
        std::function<std::string(const chaiscript::Boxed_Value& bv)>;
      auto const str = chai.eval<ToStringFn>("to_string")(val);

      HADESMEM_DETAIL_TRACE_FORMAT_A("[Info]: %s.", str.c_str());

      auto& imgui = hadesmem::cerberus::GetImguiInterface();
      imgui.LogFormat("[Info]: %s: %s", name.c_str(), str.c_str());
    }
    catch (...)
    {
      HADESMEM_DETAIL_TRACE_NOISY_FORMAT_A(
        "%s", boost::current_exception_diagnostic_information().c_str());
    }
  }
}
}

namespace hadesmem
{
namespace cerberus
{
ChaiScriptScript::ChaiScriptScript(std::string const& path)
  : path_(path),
    chai_(
      std::make_unique<chaiscript::ChaiScript>(chaiscript::Std_Lib::library()))
{
  InitializeChaiScriptContext(*chai_, true);

  auto& imgui = GetImguiInterface();
  imgui.LogFormat("[Info]: Loading script. Name: [%s]. Path: [%s].",
                  hadesmem::detail::GetPathBaseName(path_).c_str(),
                  path_.c_str());

  try
  {
    LogBoxedValue(*chai_, chai_->eval_file(path.c_str()), "Load Result");
    LogBoxedValue(*chai_, chai_->eval("CerberusScriptStart()"), "Start Result");
  }
  catch (chaiscript::exception::eval_error const& e)
  {
    if (e.call_stack.size() > 0)
    {
      imgui.LogFormat("[Error]: Message: [%s]. Line: [%d]. Column: [%d].",
                      boost::current_exception_diagnostic_information().c_str(),
                      e.call_stack[0]->start().line,
                      e.call_stack[0]->start().column);
      for (std::size_t i = 1; i < e.call_stack.size(); ++i)
      {
        imgui.LogFormat("[Error]: Frame: [%Iu]. Line: [%d]. Column: [%d].",
                        i,
                        e.call_stack[i]->start().line,
                        e.call_stack[i]->start().column);
      }
    }
    else
    {
      imgui.LogFormat(
        "[Error]: %s.",
        boost::current_exception_diagnostic_information().c_str());
    }
  }
  catch (...)
  {
    imgui.LogFormat("[Error]: %s.",
                    boost::current_exception_diagnostic_information().c_str());
  }
}

ChaiScriptScript::~ChaiScriptScript()
{
  auto& imgui = GetImguiInterface();

  try
  {
    // Don't try and do anything if we've been moved from.
    if (chai_)
    {
      imgui.LogFormat("[Info]: Unloading script. Name: [%s]. Path: [%s].",
                      hadesmem::detail::GetPathBaseName(path_).c_str(),
                      path_.c_str());

      LogBoxedValue(*chai_, chai_->eval("CerberusScriptStop()"), "Stop Result");
    }
  }
  catch (...)
  {
    imgui.LogFormat("[Error]: %s.",
      boost::current_exception_diagnostic_information().c_str());
  }
}

std::map<std::string, ChaiScriptScript>& GetChaiScriptScripts()
{
  static std::map<std::string, hadesmem::cerberus::ChaiScriptScript> scripts;
  return scripts;
}

chaiscript::ChaiScript& GetGlobalChaiScriptContext()
{
  return *GetGlobalChaiScriptContextPtr();
}

ChaiScriptInterface& GetChaiScriptInterface() noexcept
{
  static ChaiScriptImpl chai;
  return chai;
}

void ReloadDefaultChaiScriptContext(bool run_callbacks)
{
  auto& chai = GetGlobalChaiScriptContextPtr();
  chai = nullptr;
  chai =
    std::make_unique<chaiscript::ChaiScript>(chaiscript::Std_Lib::library());
  InitializeChaiScriptContext(*chai, run_callbacks);

  auto& imgui = GetImguiInterface();
  imgui.Log("Reloaded defauled ChaiScript context.");
}
}
}
