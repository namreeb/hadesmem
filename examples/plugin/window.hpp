// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <windows.h>

#include <hadesmem/config.hpp>

class Window
{
public:
  explicit Window(HINSTANCE instance) : instance_{instance}
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
  virtual LRESULT HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam);

  virtual void PaintContent(PAINTSTRUCT* /*pps*/)
  {
  }

  virtual LPCWSTR ClassName() const = 0;

  virtual void WinRegisterClass(WNDCLASSEX* pwc);

  virtual ~Window()
  {
  }

  HWND WinCreateWindow(DWORD ex_style,
                       LPCWSTR name,
                       DWORD style,
                       int x,
                       int y,
                       int cx,
                       int cy,
                       HWND parend_wnd,
                       HMENU menu);

private:
  void Register();

  void OnPaint();

  void OnPrintClient(HDC hdc);

  static LRESULT CALLBACK WndProc(HWND hwnd,
                                  UINT msg,
                                  WPARAM wparam,
                                  LPARAM lparam) HADESMEM_DETAIL_NOEXCEPT;

protected:
  HWND hwnd_{};
  HINSTANCE instance_{};
};
