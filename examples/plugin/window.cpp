// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "window.hpp"

#include <windows.h>
#include <windowsx.h>
#include <ole2.h>
#include <commctrl.h>

#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/error.hpp>

LRESULT Window::HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch (msg)
  {
  case WM_NCDESTROY:
  {
    LRESULT const lres = ::DefWindowProcW(hwnd_, msg, wparam, lparam);
    (void)::SetWindowLongPtrW(hwnd_, GWLP_USERDATA, 0);
    delete this;
    return lres;
  }

  case WM_PAINT:
    OnPaint();
    return 0;

  case WM_PRINTCLIENT:
    OnPrintClient(reinterpret_cast<HDC>(wparam));
    return 0;
  }

  return ::DefWindowProcW(hwnd_, msg, wparam, lparam);
}

void Window::WinRegisterClass(WNDCLASSEX* pwc)
{
  if (!::RegisterClassExW(pwc))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"RegisterClassExW failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }
}

HWND Window::WinCreateWindow(DWORD ex_style,
                             LPCWSTR name,
                             DWORD style,
                             int x,
                             int y,
                             int cx,
                             int cy,
                             HWND parent_wnd,
                             HMENU hmenu)
{
  Register();
  return ::CreateWindowExW(ex_style,
                           ClassName(),
                           name,
                           style,
                           x,
                           y,
                           cx,
                           cy,
                           parent_wnd,
                           hmenu,
                           instance_,
                           this);
}

void Window::Register()
{
  WNDCLASSEX wc{};
  wc.cbSize = sizeof(wc);
  wc.style = CS_DBLCLKS;
  wc.lpfnWndProc = Window::WndProc;
  wc.hInstance = instance_;
  wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
  wc.hbrBackground = ::GetSysColorBrush(COLOR_BTNFACE);
  wc.lpszClassName = ClassName();
  WinRegisterClass(&wc);
}

void Window::OnPaint()
{
  PAINTSTRUCT ps{};
  if (!::BeginPaint(hwnd_, &ps))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"BeginPaint failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  try
  {
    PaintContent(&ps);
  }
  catch (...)
  {
    (void)::EndPaint(hwnd_, &ps);
    throw;
  }

  if (!::EndPaint(hwnd_, &ps))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{}
                                    << hadesmem::ErrorString{"EndPaint failed."}
                                    << hadesmem::ErrorCodeWinLast{last_error});
  }
}

void Window::OnPrintClient(HDC hdc)
{
  PAINTSTRUCT ps{};
  ps.hdc = hdc;

  if (!::GetClientRect(hwnd_, &ps.rcPaint))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"GetClientRect failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  PaintContent(&ps);
}

LRESULT CALLBACK Window::WndProc(HWND hwnd,
                                 UINT msg,
                                 WPARAM wparam,
                                 LPARAM lparam) HADESMEM_DETAIL_NOEXCEPT
{
  Window* self = nullptr;

  try
  {
    if (msg == WM_NCCREATE)
    {
      LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lparam);
      self = reinterpret_cast<Window*>(lpcs->lpCreateParams);
      self->hwnd_ = hwnd;
      ::SetLastError(0);
      if (!::SetWindowLongPtrW(
            hwnd, GWLP_USERDATA, reinterpret_cast<LPARAM>(self)) &&
          GetLastError() != 0)
      {
        DWORD const last_error = ::GetLastError();
        HADESMEM_DETAIL_THROW_EXCEPTION(
          hadesmem::Error{}
          << hadesmem::ErrorString{"SetWindowLongPtrW failed."}
          << hadesmem::ErrorCodeWinLast{last_error});
      }
    }
    else
    {
      self =
        reinterpret_cast<Window*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    return self ? self->HandleMessage(msg, wparam, lparam)
                : ::DefWindowProcW(hwnd, msg, wparam, lparam);
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);

    return ::DefWindowProcW(hwnd, msg, wparam, lparam);
  }
}
