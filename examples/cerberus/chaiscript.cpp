// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#include "chaiscript.hpp"

#include <string>
#include <vector>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <chaiscript/chaiscript_stdlib.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/detail/str_conv.hpp>
#include <hadesmem/detail/trace.hpp>

#include "imgui.hpp"
#include "imgui_log.hpp"
#include "imgui_bindings.hpp"

// TODO: Decouple ChaiScript and ImGui components.

namespace
{
// TODO: Support this properly instead of using this hack. We both want to allow
// multiple callbacks, and also add suport for automatic cleanup when the script
// unloads (unless it proves to be more work than it's worth given we're using
// an 'unload' export anyway).
using OnFrameFn = std::function<void()>;

OnFrameFn& GetOnFrameCallback()
{
  static OnFrameFn on_frame;
  return on_frame;
}

void SetOnFrameCallback(OnFrameFn const& fn)
{
  GetOnFrameCallback() = fn;
}

void ClearOnFrameCallback()
{
  GetOnFrameCallback() = nullptr;
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
  // TODO: Fix code duplication between here and other places we init a
  // chaiscript context.

  chai_->add(GetCerberusModule());
  chai_->add(GetImGuiChaiScriptModule());

  // TODO: Remove the need for this. Right now just for quick and dirty testing
  // until we actually expose the Cerberus (and hadesmem) APIs.
  chai_->add(chaiscript::fun(&SetOnFrameCallback), "SetOnFrameCallback");
  chai_->add(chaiscript::fun(&ClearOnFrameCallback), "ClearOnFrameCallback");

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

// TODO: Implement this.
chaiscript::ModulePtr GetCerberusModule()
{
  auto cerberus = chaiscript::ModulePtr(new chaiscript::Module());
  return cerberus;
}

chaiscript::ChaiScript& GetGlobalChaiScriptContext()
{
  static chaiscript::ChaiScript chai(chaiscript::Std_Lib::library());
  return chai;
}

void InitializeChaiScript()
{
  auto& imgui = GetImguiInterface();
  imgui.RegisterOnFrame([]()
                        {
                          if (auto& callback = GetOnFrameCallback())
                          {
                            callback();
                          }
                        });
}
}
}
