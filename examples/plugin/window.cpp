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

LRESULT Window::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
  case WM_NCDESTROY:
  {
    LRESULT const lres = ::DefWindowProcW(hwnd_, uMsg, wParam, lParam);
    (void)::SetWindowLongPtrW(hwnd_, GWLP_USERDATA, 0);
    delete this;
    return lres;
  }

  case WM_PAINT:
    OnPaint();
    return 0;

  case WM_PRINTCLIENT:
    OnPrintClient(reinterpret_cast<HDC>(wParam));
    return 0;
  }

  return ::DefWindowProcW(hwnd_, uMsg, wParam, lParam);
}

void Window::WinRegisterClass(WNDCLASS* pwc)
{
  if (!::RegisterClassW(pwc))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"RegisterClassW failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }
}

HWND Window::WinCreateWindow(DWORD dwExStyle,
                             LPCWSTR pszName,
                             DWORD dwStyle,
                             int x,
                             int y,
                             int cx,
                             int cy,
                             HWND hwndParent,
                             HMENU hmenu)
{
  Register();
  return ::CreateWindowExW(dwExStyle,
                           ClassName(),
                           pszName,
                           dwStyle,
                           x,
                           y,
                           cx,
                           cy,
                           hwndParent,
                           hmenu,
                           instance_,
                           this);
}

void Window::Register()
{
  WNDCLASS wc;
  wc.style = 0;
  wc.lpfnWndProc = Window::WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = instance_;
  wc.hIcon = nullptr;
  wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
  wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
  wc.lpszMenuName = nullptr;
  wc.lpszClassName = ClassName();
  WinRegisterClass(&wc);
}

void Window::OnPaint()
{
  PAINTSTRUCT ps;
  if (!::BeginPaint(hwnd_, &ps))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"BeginPaint failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  PaintContent(&ps);

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
  PAINTSTRUCT ps;
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

LRESULT CALLBACK
  Window::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
  HADESMEM_DETAIL_NOEXCEPT
{
  Window* self = nullptr;

  try
  {
    if (uMsg == WM_NCCREATE)
    {
      LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
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

    return self ? self->HandleMessage(uMsg, wParam, lParam)
                : ::DefWindowProcW(hwnd, uMsg, wParam, lParam);
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);

    return ::DefWindowProcW(hwnd, uMsg, wParam, lParam);
  }
}
