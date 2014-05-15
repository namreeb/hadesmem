// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "window.hpp"

#include <windows.h>
#include <windowsx.h>
#include <ole2.h>
#include <commctrl.h>

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

class Window
{
public:
  explicit Window(HINSTANCE hinst) : hinst_(hinst)
  {
  }

  HWND GetHWND() const HADESMEM_DETAIL_NOEXCEPT
  {
    return hwnd_;
  }

  HINSTANCE GetHINSTANCE() const HADESMEM_DETAIL_NOEXCEPT
  {
    return hinst_;
  }

protected:
  virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

  virtual void PaintContent(PAINTSTRUCT* /*pps*/)
  {
  }

  virtual LPCWSTR ClassName() const HADESMEM_DETAIL_NOEXCEPT = 0;

  virtual void WinRegisterClass(WNDCLASS* pwc)
  {
    if (!::RegisterClassW(pwc))
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"RegisterClassW failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }
  }

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
                             hinst_,
                             this);
  }

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
  HINSTANCE hinst_{};
};

void Window::Register()
{
  WNDCLASS wc;
  wc.style = 0;
  wc.lpfnWndProc = Window::WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hinst_;
  wc.hIcon = nullptr;
  wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
  wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
  wc.lpszMenuName = nullptr;
  wc.lpszClassName = ClassName();
  WinRegisterClass(&wc);
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
      SetLastError(0);
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

LRESULT Window::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{

  switch (uMsg)
  {
  case WM_NCDESTROY:
  {
    LRESULT const lres = ::DefWindowProcW(hwnd_, uMsg, wParam, lParam);
    // Intentionally ignoring errors, because there's nothing we can do here
    // anyway.
    ::SetWindowLongPtrW(hwnd_, GWLP_USERDATA, 0);
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

class RootWindow : public Window
{
public:
  using super = Window;

  explicit RootWindow(HINSTANCE hinst) : Window(hinst)
  {
  }

  virtual LPCWSTR ClassName() const HADESMEM_DETAIL_NOEXCEPT final
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

LRESULT RootWindow::OnCreate()
{
  tab_wnd_ = CreateTabControl(GetHWND());
  display_wnd_ = DoCreateDisplayWindow(tab_wnd_);

  return 0;
}

HWND RootWindow::CreateTabControl(HWND hwndParent)
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
  if (!::GetClientRect(hwndParent, &client_rect))
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
                                       hwndParent,
                                       nullptr,
                                       GetHINSTANCE(),
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
      DestroyWindow(tab_wnd);

      DWORD const last_error = ::GetLastError();
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
                                     GetHINSTANCE(),
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
    hwnd = prw->GetHWND();

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
