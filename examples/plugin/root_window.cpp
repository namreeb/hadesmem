// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "root_window.hpp"

#include <windows.h>
#include <windowsx.h>
#include <ole2.h>
#include <commctrl.h>

#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/error.hpp>

namespace
{

struct EnsureCoUninitialize
{
  ~EnsureCoUninitialize()
  {
    CoUninitialize();
  }
};
}

LRESULT RootWindow::OnCreate()
{
  tab_wnd_ = CreateTabControl(GetHandle());
  display_wnd_ = DoCreateDisplayWindow(tab_wnd_);

  return 0;
}

HWND RootWindow::CreateTabControl(HWND parent)
{
  INITCOMMONCONTROLSEX icex{};
  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC = ICC_TAB_CLASSES;
  if (!::InitCommonControlsEx(&icex))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"InitCommonControlsEx failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  RECT client_rect;
  if (!::GetClientRect(parent, &client_rect))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"GetClientRect failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }
  HWND const tab_wnd = ::CreateWindowW(WC_TABCONTROL,
                                       L"",
                                       WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
                                       0,
                                       0,
                                       client_rect.right,
                                       client_rect.bottom,
                                       parent,
                                       nullptr,
                                       GetInstance(),
                                       nullptr);
  if (!tab_wnd)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"CreateWindowW failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  TCITEMW tie;
  tie.mask = TCIF_TEXT | TCIF_IMAGE;
  tie.iImage = -1;

  std::size_t const kDaysInWeek = 7;
  for (std::size_t i = 0; i < kDaysInWeek; i++)
  {
    // API is not const-safe due to the same member being used as both an in
    // param or an out param depending on the context.
    tie.pszText = const_cast<wchar_t*>(GetDayName(i));
    if (TabCtrl_InsertItem(tab_wnd, i, &tie) == -1)
    {
      DWORD const last_error = ::GetLastError();

      ::DestroyWindow(tab_wnd);

      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"TabCtrl_InsertItem failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }
  }

  return tab_wnd;
}

HWND RootWindow::DoCreateDisplayWindow(HWND hwndTab)
{
  HWND display_wnd = ::CreateWindowW(WC_STATIC,
                                     L"",
                                     WS_CHILD | WS_VISIBLE | WS_BORDER,
                                     100,
                                     100,
                                     100,
                                     100,
                                     hwndTab,
                                     nullptr,
                                     GetInstance(),
                                     nullptr);
  if (display_wnd == nullptr)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"CreateWindowW failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  ::SendMessageW(
    display_wnd, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(GetDayName(0)));

  return display_wnd;
}

LRESULT RootWindow::OnSize(LPARAM lParam)
{
  if (child_wnd_)
  {
    if (!::SetWindowPos(child_wnd_,
                        nullptr,
                        0,
                        0,
                        GET_X_LPARAM(lParam),
                        GET_Y_LPARAM(lParam),
                        SWP_NOZORDER | SWP_NOACTIVATE))
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"SetWindowPos failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }
  }

  if (tab_wnd_)
  {
    if (!::SetWindowPos(tab_wnd_,
                        HWND_TOP,
                        0,
                        0,
                        GET_X_LPARAM(lParam),
                        GET_Y_LPARAM(lParam),
                        SWP_SHOWWINDOW))
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"SetWindowPos failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }
  }

  return 0;
}

LRESULT RootWindow::OnNotify(LPARAM lParam)
{
  auto const notif_hdr = reinterpret_cast<LPNMHDR>(lParam);
  switch (notif_hdr->code)
  {
  case TCN_SELCHANGING:
  {
    return FALSE;
  }

  case TCN_SELCHANGE:
  {
    int const page_num = TabCtrl_GetCurSel(tab_wnd_);

    if (!::SendMessageW(display_wnd_,
                        WM_SETTEXT,
                        0,
                        reinterpret_cast<LPARAM>(GetDayName(page_num))))
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"SendMessageW failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }
    break;
  }
  }
  return TRUE;
}

LRESULT RootWindow::OnFocus()
{
  if (child_wnd_)
  {
    if (!::SetFocus(child_wnd_))
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"SetFocus failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }
  }

  return 0;
}

LRESULT RootWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
  case WM_CREATE:
    return OnCreate();

  case WM_NCDESTROY:
    ::PostQuitMessage(0);
    break;

  case WM_SIZE:
    OnSize(lParam);
    return 0;

  case WM_SETFOCUS:
    return OnFocus();

  case WM_NOTIFY:
    return OnNotify(lParam);
  }

  return super::HandleMessage(uMsg, wParam, lParam);
}

RootWindow* RootWindow::Create(HINSTANCE hinst)
{
  auto self = std::make_unique<RootWindow>(hinst);
  if (self->WinCreateWindow(0,
                            L"Scratch",
                            WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            nullptr,
                            nullptr))
  {
    return self.release();
  }

  return nullptr;
}

int PASCAL WindowThread(HINSTANCE hinst, HINSTANCE, LPSTR, int nShowCmd)
{
  try
  {
    auto& thread = GetThreadHandle();
    thread = ::OpenThread(SYNCHRONIZE, FALSE, ::GetCurrentThreadId());
    if (!thread.IsValid())
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"OpenThread failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }

    if (FAILED(::CoInitialize(nullptr)))
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"CoInitialize failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }

    EnsureCoUninitialize co_uninitialize;

    ::InitCommonControls();

    RootWindow* prw = RootWindow::Create(hinst);
    if (!prw)
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{
                               "Failed to create root window."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }

    auto& hwnd = GetWindowHandle();
    hwnd = prw->GetHandle();

    // Ignore errors
    (void)::ShowWindow(hwnd, nShowCmd);

    MSG msg;
    while (::GetMessageW(&msg, nullptr, 0, 0))
    {
      ::TranslateMessage(&msg);
      ::DispatchMessageW(&msg);
    }

    return 0;
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);

    return 1;
  }
}

HWND& GetWindowHandle()
{
  static HWND handle{};
  return handle;
}

hadesmem::detail::SmartHandle& GetThreadHandle()
{
  static hadesmem::detail::SmartHandle handle{};
  return handle;
}
