// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "window.hpp"

#include <windows.h>
#include <windowsx.h>
#include <ole2.h>
#include <commctrl.h>

#include <hadesmem/detail/trace.hpp>
#include <hadesmem/error.hpp>

class Window
{
public:
  explicit Window(HINSTANCE hinst) : hinst_(hinst)
  {
  }

  HWND GetHWND()
  {
    return hwnd_;
  }

protected:
  virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

  virtual void PaintContent(PAINTSTRUCT* /*pps*/)
  {
  }

  virtual LPCWSTR ClassName() = 0;

  virtual BOOL WinRegisterClass(WNDCLASS* pwc)
  {
    return RegisterClassW(pwc);
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

  static LRESULT CALLBACK
    WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
  HWND hwnd_;
  HINSTANCE hinst_;
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
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
  wc.lpszMenuName = nullptr;
  wc.lpszClassName = ClassName();

  WinRegisterClass(&wc);
}

LRESULT CALLBACK
  Window::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  Window* self;
  if (uMsg == WM_NCCREATE)
  {
    LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
    self = reinterpret_cast<Window*>(lpcs->lpCreateParams);
    self->hwnd_ = hwnd;
    ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LPARAM>(self));
  }
  else
  {
    self = reinterpret_cast<Window*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
  }

  if (self)
  {
    return self->HandleMessage(uMsg, wParam, lParam);
  }
  else
  {
    return ::DefWindowProcW(hwnd, uMsg, wParam, lParam);
  }
}

LRESULT Window::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  LRESULT lres;

  switch (uMsg)
  {
  case WM_NCDESTROY:
    lres = ::DefWindowProcW(hwnd_, uMsg, wParam, lParam);
    ::SetWindowLongPtrW(hwnd_, GWLP_USERDATA, 0);
    delete this;
    return lres;

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
  ::BeginPaint(hwnd_, &ps);
  PaintContent(&ps);
  ::EndPaint(hwnd_, &ps);
}

void Window::OnPrintClient(HDC hdc)
{
  PAINTSTRUCT ps;
  ps.hdc = hdc;
  ::GetClientRect(hwnd_, &ps.rcPaint);
  PaintContent(&ps);
}

class RootWindow : public Window
{
public:
  using super = Window;

  explicit RootWindow(HINSTANCE hinst) : Window(hinst)
  {
  }

  virtual LPCWSTR ClassName()
  {
    return L"Scratch";
  }

  static RootWindow* Create(HINSTANCE hinst);

protected:
  LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

  LRESULT OnCreate();

private:
  HWND hwnd_Child;
};

LRESULT RootWindow::OnCreate()
{
  return 0;
}

LRESULT RootWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
  case WM_CREATE:
    return OnCreate();

  case WM_NCDESTROY:
    // Death of the root window ends the thread
    ::PostQuitMessage(0);
    break;

  case WM_SIZE:
    if (hwnd_Child)
    {
      ::SetWindowPos(hwnd_Child,
                     nullptr,
                     0,
                     0,
                     GET_X_LPARAM(lParam),
                     GET_Y_LPARAM(lParam),
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
    return 0;

  case WM_SETFOCUS:
    if (hwnd_Child)
    {
      ::SetFocus(hwnd_Child);
    }
    return 0;
  }

  return super::HandleMessage(uMsg, wParam, lParam);
}

RootWindow* RootWindow::Create(HINSTANCE hinst)
{
  RootWindow* self = new RootWindow(hinst);
  if (self &&
      self->WinCreateWindow(0,
                            L"Scratch",
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            nullptr,
                            nullptr))
  {
    return self;
  }

  delete self;

  return nullptr;
}

int PASCAL WindowThread(HINSTANCE hinst, HINSTANCE, LPSTR, int nShowCmd)
{
  try
  {
    auto& thread = GetThreadHandle();
    thread = ::OpenThread(SYNCHRONIZE, FALSE, GetCurrentThreadId());
    if (!thread)
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"OpenThread failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }

    if (SUCCEEDED(::CoInitialize(nullptr)))
    {
      ::InitCommonControls();

      RootWindow* prw = RootWindow::Create(hinst);
      if (prw)
      {
        auto& hwnd = GetWindowHandle();
        hwnd = prw->GetHWND();

        ::ShowWindow(hwnd, nShowCmd);

        MSG msg;
        while (::GetMessageW(&msg, nullptr, 0, 0))
        {
          ::TranslateMessage(&msg);
          ::DispatchMessageW(&msg);
        }
      }

      ::CoUninitialize();
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

HANDLE& GetThreadHandle()
{
  static HANDLE handle{};
  return handle;
}
