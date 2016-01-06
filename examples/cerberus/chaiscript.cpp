// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "chaiscript.hpp"

#include <string>
#include <vector>
#include <mutex>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <chaiscript/chaiscript_stdlib.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/detail/str_conv.hpp>
#include <hadesmem/detail/trace.hpp>

#include "callbacks.hpp"
#include "cerberus_bindings.hpp"
#include "imgui.hpp"
#include "imgui_log.hpp"
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
};

void InitializeChaiScriptContext(chaiscript::ChaiScript& chai)
{
  chai.add(hadesmem::cerberus::GetCerberusModule());
  chai.add(hadesmem::cerberus::GetImGuiChaiScriptModule());

  auto const& callbacks = GetOnInitializeChaiScriptContextCallbacks();
  callbacks.Run(chai);
}

std::unique_ptr<chaiscript::ChaiScript>& GetGlobalChaiScriptContextPtr()
{
  static auto chai =
    std::make_unique<chaiscript::ChaiScript>(chaiscript::Std_Lib::library());
  static std::once_flag once;
  std::call_once(once,
                 [&]()
                 {
                   InitializeChaiScriptContext(*chai);
                 });
  return chai;
}
}

namespace hadesmem
{
namespace cerberus
{
ChaiScriptScript::ChaiScriptScript(std::string const& path)
  : chai_(
      std::make_unique<chaiscript::ChaiScript>(chaiscript::Std_Lib::library()))
{
  InitializeChaiScriptContext(*chai_);

  auto& log = GetImGuiLogWindow();

  try
  {
    auto const val = chai_->eval_file(path.c_str());
    if (!val.get_type_info().bare_equal(chaiscript::user_type<void>()))
    {
      try
      {
        auto const str = chai_->eval<
          std::function<std::string(const chaiscript::Boxed_Value& bv)>>(
          "to_string")(val);
        HADESMEM_DETAIL_TRACE_FORMAT_A("[Info]: %s.", str.c_str());
        log.AddLog("[Info]: %s\n", str.c_str());
      }
      catch (...)
      {
      }
    }

    chai_->eval("CerberusScriptStart()");
  }
  catch (const chaiscript::exception::eval_error& ee)
  {
    if (ee.call_stack.size() > 0)
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "[Error]: %s during evaluation at (%d,%d).",
        boost::current_exception_diagnostic_information().c_str(),
        ee.call_stack[0]->start().line,
        ee.call_stack[0]->start().column);
      log.AddLog("[Error]: %s during evaluation at (%d,%d)\n",
                 boost::current_exception_diagnostic_information().c_str(),
                 ee.call_stack[0]->start().line,
                 ee.call_stack[0]->start().column);
    }
    else
    {
      HADESMEM_DETAIL_TRACE_FORMAT_A(
        "[Error]: %s.",
        boost::current_exception_diagnostic_information().c_str());
      log.AddLog("[Error]: %s\n",
                 boost::current_exception_diagnostic_information().c_str());
    }
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_FORMAT_A(
      "[Error]: %s.",
      boost::current_exception_diagnostic_information().c_str());
    log.AddLog("[Error]: %s\n",
               boost::current_exception_diagnostic_information().c_str());
  }
}

ChaiScriptScript::~ChaiScriptScript()
{
  try
  {
    if (chai_)
    {
      chai_->eval("CerberusScriptStop()");
    }
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
  }
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

void ReloadDefaultChaiScriptContext()
{
  auto& chai = GetGlobalChaiScriptContextPtr();
  chai =
    std::make_unique<chaiscript::ChaiScript>(chaiscript::Std_Lib::library());
  InitializeChaiScriptContext(*chai);
}
}
}
