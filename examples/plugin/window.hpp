// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

#include <hadesmem/config.hpp>

class Window
{
public:
  explicit Window(HINSTANCE hinst) : instance_{hinst}
  {
  }

  HWND GetHandle() const HADESMEM_DETAIL_NOEXCEPT
  {
    return hwnd_;
  }

  HINSTANCE GetInstance() const HADESMEM_DETAIL_NOEXCEPT
  {
    return instance_;
  }

protected:
  virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

  virtual void PaintContent(PAINTSTRUCT* /*pps*/)
  {
  }

  virtual LPCWSTR ClassName() const = 0;

  virtual void WinRegisterClass(WNDCLASS* pwc);

  virtual ~Window()
  {
  }

  HWND WinCreateWindow(DWORD dwExStyle,
                       LPCWSTR pszName,
                       DWORD dwStyle,
                       int x,
                       int y,
                       int cx,
                       int cy,
                       HWND hwndParent,
                       HMENU hmenu);

private:
  void Register();

  void OnPaint();

  void OnPrintClient(HDC hdc);

  static LRESULT CALLBACK WndProc(HWND hwnd,
                                  UINT uMsg,
                                  WPARAM wParam,
                                  LPARAM lParam) HADESMEM_DETAIL_NOEXCEPT;

protected:
  HWND hwnd_{};
  HINSTANCE instance_{};
};
