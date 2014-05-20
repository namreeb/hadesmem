// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/smart_handle.hpp>

#include "window.hpp"

HWND& GetWindowHandle();

hadesmem::detail::SmartHandle& GetThreadHandle();

int PASCAL WindowThread(HINSTANCE hinst, HINSTANCE, LPSTR, int nShowCmd);

class RootWindow : public Window
{
public:
  using super = Window;

  explicit RootWindow(HINSTANCE hinst) : Window(hinst)
  {
  }

  virtual LPCWSTR ClassName() const final
  {
    return L"Scratch";
  }

  static RootWindow* Create(HINSTANCE hinst);

protected:
  LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) final;

  LRESULT OnCreate();

  LRESULT OnFocus();

  LRESULT OnSize(LPARAM lParam);

  LRESULT OnNotify(LPARAM lParam);

private:
  HWND CreateTabControl(HWND hwndParent);

  HWND DoCreateDisplayWindow(HWND hwndTab);

  wchar_t const* GetDayName(std::size_t i) const HADESMEM_DETAIL_NOEXCEPT
  {
    wchar_t const* const kDayNames[] = {L"Sunday",    L"Monday",   L"Tuesday",
                                        L"Wednesday", L"Thursday", L"Friday",
                                        L"Saturday"};
    return kDayNames[i];
  }

  HWND child_wnd_{};
  HWND tab_wnd_{};
  HWND display_wnd_{};
};
