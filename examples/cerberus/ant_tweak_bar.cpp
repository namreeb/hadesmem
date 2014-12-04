// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "ant_tweak_bar.hpp"

#include "callbacks.hpp"
#include "cursor.hpp"
#include "hook_disabler.hpp"
#include "input.hpp"
#include "plugin.hpp"
#include "render.hpp"
#include "window.hpp"

namespace
{
hadesmem::cerberus::Callbacks<
  hadesmem::cerberus::OnAntTweakBarInitializeCallback>&
  GetOnAntTweakBarInitializeCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnAntTweakBarInitializeCallback> callbacks;
  return callbacks;
}

hadesmem::cerberus::Callbacks<hadesmem::cerberus::OnAntTweakBarCleanupCallback>&
  GetOnAntTweakBarCleanupCallbacks()
{
  static hadesmem::cerberus::Callbacks<
    hadesmem::cerberus::OnAntTweakBarCleanupCallback> callbacks;
  return callbacks;
}

class AntTweakBarImpl : public hadesmem::cerberus::AntTweakBarInterface
{
public:
  virtual std::size_t RegisterOnInitialize(std::function<
    hadesmem::cerberus::OnAntTweakBarInitializeCallback> const& callback) final
  {
    auto& callbacks = GetOnAntTweakBarInitializeCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnInitialize(std::size_t id) final
  {
    auto& callbacks = GetOnAntTweakBarInitializeCallbacks();
    return callbacks.Unregister(id);
  }

  virtual void CallOnInitialize() final
  {
    auto& callbacks = GetOnAntTweakBarInitializeCallbacks();
    callbacks.Run(this);
  }

  virtual std::size_t RegisterOnCleanup(std::function<
    hadesmem::cerberus::OnAntTweakBarCleanupCallback> const& callback) final
  {
    auto& callbacks = GetOnAntTweakBarCleanupCallbacks();
    return callbacks.Register(callback);
  }

  virtual void UnregisterOnCleanup(std::size_t id) final
  {
    auto& callbacks = GetOnAntTweakBarCleanupCallbacks();
    return callbacks.Unregister(id);
  }

  virtual void CallOnCleanup() final
  {
    auto& callbacks = GetOnAntTweakBarCleanupCallbacks();
    callbacks.Run(this);
  }

  virtual bool IsInitialized() final
  {
    return hadesmem::cerberus::AntTweakBarInitializedAny();
  }

  virtual TwBar* TwNewBar(const char* bar_name) final
  {
    return ::TwNewBar(bar_name);
  }

  virtual int TwDeleteBar(TwBar* bar) final
  {
    return ::TwDeleteBar(bar);
  }

  virtual int TwDeleteAllBars() final
  {
    return ::TwDeleteAllBars();
  }

  virtual int TwSetTopBar(TwBar const* bar) final
  {
    return ::TwSetTopBar(bar);
  }

  virtual TwBar* TwGetTopBar() final
  {
    return ::TwGetTopBar();
  }

  virtual int TwSetBottomBar(TwBar const* bar) final
  {
    return ::TwSetBottomBar(bar);
  }

  virtual TwBar* TwGetBottomBar() final
  {
    return ::TwGetBottomBar();
  }

  virtual char const* TwGetBarName(TwBar const* bar) final
  {
    return ::TwGetBarName(bar);
  }

  virtual int TwGetBarCount() final
  {
    return ::TwGetBarCount();
  }

  virtual TwBar* TwGetBarByIndex(int bar_index) final
  {
    return ::TwGetBarByIndex(bar_index);
  }

  virtual TwBar* TwGetBarByName(char const* bar_name) final
  {
    return ::TwGetBarByName(bar_name);
  }

  virtual int TwRefreshBar(TwBar* bar) final
  {
    return ::TwRefreshBar(bar);
  }

  virtual int TwAddVarRW(
    TwBar* bar, const char* name, TwType type, void* var, const char* def) final
  {
    return ::TwAddVarRW(bar, name, type, var, def);
  }

  virtual int TwAddVarRO(TwBar* bar,
                         char const* name,
                         TwType type,
                         void const* var,
                         char const* def) final
  {
    return ::TwAddVarRO(bar, name, type, var, def);
  }

  virtual int TwAddVarCB(TwBar* bar,
                         char const* name,
                         TwType type,
                         TwSetVarCallback set_callback,
                         TwGetVarCallback get_callback,
                         void* client_data,
                         char const* def) final
  {
    return ::TwAddVarCB(
      bar, name, type, set_callback, get_callback, client_data, def);
  }

  virtual int TwAddButton(TwBar* bar,
                          const char* name,
                          TwButtonCallback callback,
                          void* client_data,
                          const char* def) final
  {
    return ::TwAddButton(bar, name, callback, client_data, def);
  }

  virtual int
    TwAddSeparator(TwBar* bar, char const* name, char const* def) final
  {
    return ::TwAddSeparator(bar, name, def);
  }

  virtual int TwRemoveVar(TwBar* bar, char const* name) final
  {
    return ::TwRemoveVar(bar, name);
  }

  virtual int TwRemoveAllVars(TwBar* bar) final
  {
    return ::TwRemoveAllVars(bar);
  }

  virtual int TwDefine(char const* def) final
  {
    return ::TwDefine(def);
  }

  virtual TwType TwDefineEnum(char const* name,
                              TwEnumVal const* enum_values,
                              unsigned int nb_values) final
  {
    return ::TwDefineEnum(name, enum_values, nb_values);
  }

  virtual TwType TwDefineEnumFromString(char const* name,
                                        char const* enum_string) final
  {
    return ::TwDefineEnumFromString(name, enum_string);
  }

  virtual TwType TwDefineStruct(char const* name,
                                TwStructMember const* struct_members,
                                unsigned int nb_members,
                                size_t struct_size,
                                TwSummaryCallback summary_callback,
                                void* summary_client_data) final
  {
    return ::TwDefineStruct(name,
                            struct_members,
                            nb_members,
                            struct_size,
                            summary_callback,
                            summary_client_data);
  }

  virtual void
    TwCopyCDStringToClientFunc(TwCopyCDStringToClient copy_cd_string_func) final
  {
    return ::TwCopyCDStringToClientFunc(copy_cd_string_func);
  }

  virtual void TwCopyCDStringToLibrary(char** destination_library_string_ptr,
                                       char const* source_client_string) final
  {
    return ::TwCopyCDStringToLibrary(destination_library_string_ptr,
                                     source_client_string);
  }

  virtual void TwCopyStdStringToClientFunc(
    TwCopyStdStringToClient copy_std_string_to_client_func) final
  {
    return ::TwCopyStdStringToClientFunc(copy_std_string_to_client_func);
  }

  virtual void
    TwCopyStdStringToLibrary(std::string& destination_library_string,
                             std::string const& source_client_string) final
  {
    return ::TwCopyStdStringToLibrary(destination_library_string,
                                      source_client_string);
  }

  virtual int TwGetParam(TwBar* bar,
                         char const* var_name,
                         char const* param_name,
                         TwParamValueType param_value_type,
                         unsigned int out_value_max_count,
                         void* out_values) final
  {
    return ::TwGetParam(bar,
                        var_name,
                        param_name,
                        param_value_type,
                        out_value_max_count,
                        out_values);
  }

  virtual int TwSetParam(TwBar* bar,
                         char const* var_name,
                         char const* param_name,
                         TwParamValueType param_value_type,
                         unsigned int in_value_count,
                         void const* in_values) final
  {
    return ::TwSetParam(
      bar, var_name, param_name, param_value_type, in_value_count, in_values);
  }

  virtual int TwInit(TwGraphAPI graph_api, void* device) final
  {
    return ::TwInit(graph_api, device);
  }

  virtual int TwTerminate() final
  {
    return ::TwTerminate();
  }

  virtual int TwDraw() final
  {
    return ::TwDraw();
  }

  virtual int TwWindowSize(int width, int height) final
  {
    return ::TwWindowSize(width, height);
  }

  virtual int TwSetCurrentWindow(int window_id) final
  {
    return ::TwSetCurrentWindow(window_id);
  }

  virtual int TwGetCurrentWindow() final
  {
    return ::TwGetCurrentWindow();
  }

  virtual int TwWindowExists(int window_id) final
  {
    return ::TwWindowExists(window_id);
  }

  virtual int TwKeyPressed(int key, int modifiers) final
  {
    return ::TwKeyPressed(key, modifiers);
  }

  virtual int TwKeyTest(int key, int modifiers) final
  {
    return ::TwKeyTest(key, modifiers);
  }

  virtual int TwMouseButton(TwMouseAction action, TwMouseButtonID button) final
  {
    return ::TwMouseButton(action, button);
  }

  virtual int TwMouseMotion(int mouse_x, int mouse_y) final
  {
    return ::TwMouseMotion(mouse_x, mouse_y);
  }

  virtual int TwMouseWheel(int pos) final
  {
    return ::TwMouseWheel(pos);
  }

  virtual char const* TwGetLastError() final
  {
    return ::TwGetLastError();
  }

  virtual void TwHandleErrors(TwErrorHandler error_handler) final
  {
    return ::TwHandleErrors(error_handler);
  }
};

void SetAllTweakBarVisibility(bool visible, bool /*old_visible*/)
{
  for (int i = 0; i < ::TwGetBarCount(); ++i)
  {
    auto const bar = ::TwGetBarByIndex(i);
    auto const name = ::TwGetBarName(bar);
    auto const define = std::string(name) + " visible=" +
                        (visible ? std::string("true") : std::string("false"));
    ::TwDefine(define.c_str());
  }
}

void HandleInputQueueEntry(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  hadesmem::cerberus::HookDisabler disable_set_cursor_hook{
    &hadesmem::cerberus::GetDisableSetCursorHook()};

  ::TwEventWin(hwnd, msg, wparam, lparam);
}

std::string& GetPluginPathTw()
{
  static std::string path;
  return path;
}

void TW_CALL CopyStdStringToClientTw(std::string& dst, const std::string& src)
{
  dst = src;
}

void TW_CALL LoadPluginCallbackTw(void* /*client_data*/)
{
  HADESMEM_DETAIL_TRACE_FORMAT_A("Path: %s.", GetPluginPathTw().c_str());

  try
  {
    hadesmem::cerberus::LoadPlugin(
      hadesmem::detail::MultiByteToWideChar(GetPluginPathTw()));
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A("Failed to load plugin.");
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
  }
}

void TW_CALL UnloadPluginCallbackTw(void* /*client_data*/)
{
  HADESMEM_DETAIL_TRACE_FORMAT_A("Path: %s.", GetPluginPathTw().c_str());

  try
  {
    hadesmem::cerberus::UnloadPlugin(
      hadesmem::detail::MultiByteToWideChar(GetPluginPathTw()));
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A("Failed to unload plugin.");
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
  }
}
}

namespace hadesmem
{
namespace cerberus
{
void InitializeAntTweakBar(RenderApi api, void* device, bool& initialized)
{
  TwGraphAPI tw_api = [](hadesmem::cerberus::RenderApi api) -> TwGraphAPI
  {
    switch (api)
    {
    case RenderApi::D3D9:
      return TW_DIRECT3D9;
    case RenderApi::D3D10:
      return TW_DIRECT3D10;
    case RenderApi::D3D11:
      return TW_DIRECT3D11;
    case RenderApi::OpenGL32:
      return TW_OPENGL;
    default:
      HADESMEM_DETAIL_ASSERT(false);
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"Unknown render API."});
    }
  }(api);

  if (!::TwInit(tw_api, device))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TwInit failed."}
                        << hadesmem::ErrorStringOther{TwGetLastError()});
  }

  initialized = true;

  RECT wnd_rect{0, 0, 800, 600};
  if (auto const window = hadesmem::cerberus::GetCurrentWindow())
  {
    HADESMEM_DETAIL_TRACE_A("Have a window.");

    if (!::GetClientRect(window, &wnd_rect) || wnd_rect.right == 0 ||
        wnd_rect.bottom == 0)
    {
      HADESMEM_DETAIL_TRACE_A(
        "GetClientRect failed (or returned an invalid box).");

      wnd_rect = RECT{0, 0, 800, 600};
    }
    else
    {
      HADESMEM_DETAIL_TRACE_A("Got client rect.");
    }
  }
  else
  {
    HADESMEM_DETAIL_TRACE_A("Do not have a window.");
  }
  HADESMEM_DETAIL_TRACE_FORMAT_A(
    "Window size is %ldx%ld.", wnd_rect.right, wnd_rect.bottom);

  if (!::TwWindowSize(wnd_rect.right, wnd_rect.bottom))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TwWindowSize failed."}
                        << hadesmem::ErrorCodeWinLast{last_error}
                        << hadesmem::ErrorStringOther{TwGetLastError()});
  }

  ::TwCopyStdStringToClientFunc(CopyStdStringToClientTw);

  auto const bar = ::TwNewBar("HadesMem");
  if (!bar)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TwNewBar failed."}
                        << hadesmem::ErrorStringOther{TwGetLastError()});
  }

  auto const load_button = ::TwAddButton(bar,
                                         "LoadPluginBtn",
                                         &LoadPluginCallbackTw,
                                         nullptr,
                                         " label='Load Plugin' ");
  if (!load_button)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TwAddButton failed."}
                        << hadesmem::ErrorStringOther{TwGetLastError()});
  }

  auto const unload_button = ::TwAddButton(bar,
                                           "UnloadPluginBtn",
                                           &UnloadPluginCallbackTw,
                                           nullptr,
                                           " label='Unload Plugin' ");
  if (!unload_button)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TwAddButton failed."}
                        << hadesmem::ErrorStringOther{TwGetLastError()});
  }

  auto const plugin_path = ::TwAddVarRW(bar,
                                        "LoadPluginPath",
                                        TW_TYPE_STDSTRING,
                                        &GetPluginPathTw(),
                                        " label='Plugin Path' ");
  if (!plugin_path)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TwAddVarRW failed."}
                        << hadesmem::ErrorStringOther{TwGetLastError()});
  }
}

void CleanupAntTweakBar(bool& initialized)
{
  if (!::TwTerminate())
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TwTerminate failed."});
  }

  initialized = false;
}

AntTweakBarInterface& GetAntTweakBarInterface() HADESMEM_DETAIL_NOEXCEPT
{
  static AntTweakBarImpl ant_tweak_bar;
  return ant_tweak_bar;
}

void InitializeAntTweakBar()
{
  auto& input = GetInputInterface();
  input.RegisterOnInputQueueEntry(HandleInputQueueEntry);

  auto& render = GetRenderInterface();
  render.RegisterOnSetGuiVisibility(SetAllTweakBarVisibility);
}
}
}
