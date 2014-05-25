// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <condition_variable>
#include <mutex>
#include <vector>

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/smart_handle.hpp>

#include "radar.hpp"
#include "window.hpp"

#define WM_APP_UPDATE_RADAR WM_APP

HWND& GetWindowHandle();

hadesmem::detail::SmartHandle& GetThreadHandle();

int PASCAL WindowThread(HINSTANCE instance,
                        HINSTANCE prev_instance,
                        LPSTR cmd_line,
                        int show_cmd);

class RootWindow : public Window
{
public:
  using super = Window;

  explicit RootWindow(HINSTANCE instance) : Window(instance)
  {
  }

  virtual LPCWSTR ClassName() const final
  {
    return L"Scratch";
  }

  static RootWindow* Create(HINSTANCE instance);

protected:
  virtual LRESULT HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam) final;

  LRESULT OnCreate();

  LRESULT OnFocus();

  LRESULT OnSize(LPARAM lparam);

  LRESULT OnNotify(LPARAM lparam);

private:
  HWND CreateTabControl(HWND parent_wnd);

  HWND CreateRadarDisplayWindow(HWND tab_wnd);

  HWND CreateConfigDisplayWindow(HWND tab_wnd);

  void PaintRadar(HWND hwnd);

  void UpdateRadar(struct RadarData* radar_data);

  static LRESULT CALLBACK
    RadarWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

  struct Tab
  {
    enum
    {
      kRadar,
      kConfig
    };
  };

  HWND child_wnd_{};
  HWND tab_wnd_{};
  HWND radar_display_wnd_{};
  HWND config_display_wnd_{};
  WNDPROC radar_prev_wndproc_{};
  RadarData radar_data_;
};
