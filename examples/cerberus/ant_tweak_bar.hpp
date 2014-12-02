// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <functional>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <anttweakbar.h>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>

namespace hadesmem
{
namespace cerberus
{
class AntTweakBarInterface;

typedef void
  OnAntTweakBarInitializeCallback(AntTweakBarInterface* ant_tweak_bar);

typedef void OnAntTweakBarCleanupCallback(AntTweakBarInterface* ant_tweak_bar);

class AntTweakBarInterface
{
public:
  virtual ~AntTweakBarInterface()
  {
  }

  virtual std::size_t RegisterOnInitialize(
    std::function<OnAntTweakBarInitializeCallback> const& callback) = 0;

  virtual void UnregisterOnInitialize(std::size_t id) = 0;

  virtual void CallOnInitialize() = 0;

  virtual std::size_t RegisterOnCleanup(
    std::function<OnAntTweakBarCleanupCallback> const& callback) = 0;

  virtual void UnregisterOnCleanup(std::size_t id) = 0;

  virtual void CallOnCleanup() = 0;

  virtual bool IsInitialized() = 0;

  virtual TwBar* TwNewBar(char const* bar_name) = 0;

  virtual int TwDeleteBar(TwBar* bar) = 0;

  virtual int TwDeleteAllBars() = 0;

  virtual int TwSetTopBar(TwBar const* bar) = 0;

  virtual TwBar* TwGetTopBar() = 0;

  virtual int TwSetBottomBar(TwBar const* bar) = 0;

  virtual TwBar* TwGetBottomBar() = 0;

  virtual char const* TwGetBarName(TwBar const* bar) = 0;

  virtual int TwGetBarCount() = 0;

  virtual TwBar* TwGetBarByIndex(int bar_index) = 0;

  virtual TwBar* TwGetBarByName(char const* bar_name) = 0;

  virtual int TwRefreshBar(TwBar* bar) = 0;

  virtual int TwAddVarRW(
    TwBar* bar, const char* name, TwType type, void* var, const char* def) = 0;

  virtual int TwAddVarRO(TwBar* bar,
                         char const* name,
                         TwType type,
                         void const* var,
                         char const* def) = 0;

  virtual int TwAddVarCB(TwBar* bar,
                         char const* name,
                         TwType type,
                         TwSetVarCallback set_callback,
                         TwGetVarCallback get_callback,
                         void* client_data,
                         char const* def) = 0;

  virtual int TwAddButton(TwBar* bar,
                          const char* name,
                          TwButtonCallback callback,
                          void* client_data,
                          const char* def) = 0;

  virtual int TwAddSeparator(TwBar* bar, char const* name, char const* def) = 0;

  virtual int TwRemoveVar(TwBar* bar, char const* name) = 0;

  virtual int TwRemoveAllVars(TwBar* bar) = 0;

  virtual int TwDefine(char const* def) = 0;

  virtual TwType TwDefineEnum(char const* name,
                              TwEnumVal const* enum_values,
                              unsigned int nb_values) = 0;

  virtual TwType TwDefineEnumFromString(char const* name,
                                        char const* enum_string) = 0;

  virtual TwType TwDefineStruct(char const* name,
                                TwStructMember const* struct_members,
                                unsigned int nb_members,
                                size_t struct_size,
                                TwSummaryCallback summary_callback,
                                void* summary_client_data) = 0;

  virtual void
    TwCopyCDStringToClientFunc(TwCopyCDStringToClient copy_cd_string_func) = 0;

  virtual void TwCopyCDStringToLibrary(char** destination_library_string_ptr,
                                       char const* source_client_string) = 0;

  virtual void TwCopyStdStringToClientFunc(
    TwCopyStdStringToClient copy_std_string_to_client_func) = 0;

  virtual void
    TwCopyStdStringToLibrary(std::string& destination_library_string,
                             std::string const& source_client_string) = 0;

  virtual int TwGetParam(TwBar* bar,
                         char const* var_name,
                         char const* param_name,
                         TwParamValueType param_value_type,
                         unsigned int out_value_max_count,
                         void* out_values) = 0;

  virtual int TwSetParam(TwBar* bar,
                         char const* var_name,
                         char const* param_name,
                         TwParamValueType param_value_type,
                         unsigned int in_value_count,
                         void const* in_values) = 0;

  virtual int TwInit(TwGraphAPI graph_api, void* device) = 0;

  virtual int TwTerminate() = 0;

  virtual int TwDraw() = 0;

  virtual int TwWindowSize(int width, int height) = 0;

  virtual int TwSetCurrentWindow(int window_id) = 0;

  virtual int TwGetCurrentWindow() = 0;

  virtual int TwWindowExists(int window_id) = 0;

  virtual int TwKeyPressed(int key, int modifiers) = 0;

  virtual int TwKeyTest(int key, int modifiers) = 0;

  virtual int TwMouseButton(TwMouseAction action, TwMouseButtonID button) = 0;

  virtual int TwMouseMotion(int mouse_x, int mouse_y) = 0;

  virtual int TwMouseWheel(int pos) = 0;

  virtual char const* TwGetLastError() = 0;

  virtual void TwHandleErrors(TwErrorHandler error_handler) = 0;
};

AntTweakBarInterface& GetAntTweakBarInterface() HADESMEM_DETAIL_NOEXCEPT;

}
}
