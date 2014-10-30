// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "root_window.hpp"

#include <windows.h>
#include <windowsx.h>
#include <ole2.h>
#include <commctrl.h>

#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/scope_warden.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/error.hpp>

// Danger Will Robinson! I have no idea what I'm doing when it comes to Win32
// GUI programming, so this code is terrible and should only serve as an example
// of what not to do! Seriously, this code is bad and I should feel bad.

namespace
{
struct EnsureCoUninitialize
{
  ~EnsureCoUninitialize()
  {
    CoUninitialize();
  }
};

void SetDefaultFont(HWND hwnd)
{
  ::SendMessageW(hwnd,
                 WM_SETFONT,
                 reinterpret_cast<WPARAM>(::GetStockObject(DEFAULT_GUI_FONT)),
                 static_cast<LPARAM>(true));
}
}

LRESULT RootWindow::OnCreate()
{
  auto const hwnd = GetHandle();
  SetDefaultFont(hwnd);
  tab_wnd_ = CreateTabControl(hwnd);
  radar_display_wnd_ = CreateRadarDisplayWindow(hwnd);
  config_display_wnd_ = CreateConfigDisplayWindow(hwnd);

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

  RECT client_rect{};
  if (!::GetClientRect(parent, &client_rect))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"GetClientRect failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }
  HWND const tab_wnd =
    ::CreateWindowExW(0,
                      WC_TABCONTROL,
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

  SetDefaultFont(tab_wnd);

  TCITEMW tie;
  tie.mask = TCIF_TEXT | TCIF_IMAGE;
  tie.iImage = -1;

  // API is not const-safe.
  tie.pszText = const_cast<wchar_t*>(L"Radar");
  if (TabCtrl_InsertItem(tab_wnd, Tab::kRadar, &tie) == -1)
  {
    DWORD const last_error = ::GetLastError();

    ::DestroyWindow(tab_wnd);

    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TabCtrl_InsertItem failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  // API is not const-safe.
  tie.pszText = const_cast<wchar_t*>(L"Config");
  if (TabCtrl_InsertItem(tab_wnd, Tab::kConfig, &tie) == -1)
  {
    DWORD const last_error = ::GetLastError();

    ::DestroyWindow(tab_wnd);

    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TabCtrl_InsertItem failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  return tab_wnd;
}

HWND RootWindow::CreateRadarDisplayWindow(HWND parent)
{
  RECT tr{};
  if (!TabCtrl_GetItemRect(tab_wnd_, Tab::kRadar, &tr))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TabCtrl_GetItemRect failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  RECT cr{};
  if (!::GetClientRect(GetHandle(), &cr))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"GetClientRect failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  int const kBorderOffset = 10;
  HWND display_wnd =
    ::CreateWindowExW(0,
                      WC_STATIC,
                      nullptr,
                      WS_CHILD | WS_VISIBLE | WS_BORDER | SS_CENTER |
                        SS_CENTERIMAGE | SS_BLACKRECT,
                      kBorderOffset,
                      tr.bottom + kBorderOffset,
                      cr.right - kBorderOffset * 2,
                      cr.bottom - tr.bottom - kBorderOffset * 2,
                      parent,
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

  radar_prev_wndproc_ = reinterpret_cast<WNDPROC>(::SetWindowLongPtrW(
    display_wnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&RadarWndProc)));
  if (!radar_prev_wndproc_)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"SetWindowLongPtrW failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  SetDefaultFont(display_wnd);

  return display_wnd;
}

HWND RootWindow::CreateConfigDisplayWindow(HWND parent)
{
  RECT tr{};
  if (!TabCtrl_GetItemRect(tab_wnd_, Tab::kConfig, &tr))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TabCtrl_GetItemRect failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  HWND display_wnd =
    ::CreateWindowExW(0,
                      WC_STATIC,
                      L"Config",
                      WS_CHILD | WS_BORDER | SS_CENTER | SS_CENTERIMAGE,
                      5,
                      tr.bottom + 5,
                      200,
                      60,
                      parent,
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

  SetDefaultFont(display_wnd);

  return display_wnd;
}

void RootWindow::PaintRadar(HWND hwnd)
{
  auto const radar_wnd_dc = ::GetDC(hwnd);
  if (!radar_wnd_dc)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{}
                                    << hadesmem::ErrorString{"GetDC failed."}
                                    << hadesmem::ErrorCodeWinLast{last_error});
  }

  auto const cleanup_dc = [&]()
  {
    ::ReleaseDC(hwnd, radar_wnd_dc);
  };
  auto const scope_cleanup_dc = hadesmem::detail::MakeScopeWarden(cleanup_dc);

  RECT rr{};
  if (!::GetClientRect(hwnd, &rr))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"GetClientRect failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  auto const radar_width = rr.right - rr.left;
  auto const radar_height = rr.bottom - rr.top;

  hadesmem::detail::SmartDeleteDcHandle const mem_dc{
    ::CreateCompatibleDC(radar_wnd_dc)};
  if (!mem_dc.GetHandle())
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"CreateCompatibleDC failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  hadesmem::detail::SmartGdiObjectHandle const mem_bitmap{
    ::CreateCompatibleBitmap(radar_wnd_dc, radar_width, radar_height)};
  if (!mem_bitmap.GetHandle())
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{}
                                    << hadesmem::ErrorString{
                                         "CreateCompatibleBitmap failed."}
                                    << hadesmem::ErrorCodeWinLast{last_error});
  }

  HGDIOBJ const old_obj =
    ::SelectObject(mem_dc.GetHandle(), mem_bitmap.GetHandle());
  if (!old_obj || old_obj == HGDI_ERROR)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"SelectObject failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  auto const cleanup_obj = [&]()
  {
    ::SelectObject(mem_dc.GetHandle(), old_obj);
  };
  auto const scope_cleanup_obj = hadesmem::detail::MakeScopeWarden(cleanup_obj);

  HGDIOBJ old_obj_tmp =
    ::SelectObject(mem_dc.GetHandle(), ::GetStockObject(BLACK_BRUSH));
  if (!old_obj_tmp || old_obj_tmp == HGDI_ERROR)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"SelectObject failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  if (!::SetBkMode(mem_dc.GetHandle(), TRANSPARENT))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"SetBkMode failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  if (!::FillRect(
        mem_dc.GetHandle(), &rr, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1)))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{}
                                    << hadesmem::ErrorString{"FillRect failed."}
                                    << hadesmem::ErrorCodeWinLast{last_error});
  }

  auto const midpoint_x = radar_width / 2;
  auto const midpoint_y = radar_height / 2;

  double const max_x = 100.0;
  double const max_y = 100.0;

  auto const origin_point = radar_data_.player_.pos_;
  auto const origin_heading = radar_data_.player_.heading_;
  auto const pi = 3.14159265;
  auto const origin_heading_rads = origin_heading * pi / 180;

  for (auto const& unit_data : radar_data_.units_)
  {
    auto const point = unit_data.pos_;

    if (std::abs(point.x - origin_point.x) > max_x ||
        std::abs(point.y - origin_point.y) > max_y)
    {
      continue;
    }

    auto x = point.x - origin_point.x;
    auto y = point.y - origin_point.y;

    auto r = std::hypot(x, y);
    auto theta = std::atan2(y, x) + origin_heading_rads;

    x = static_cast<float>(r * std::cos(theta));
    y = static_cast<float>(r * std::sin(theta));

    auto const scaled_x = midpoint_x / (max_x / std::abs(x));
    auto draw_x =
      static_cast<int>(x > 0 ? midpoint_x + scaled_x : midpoint_x - scaled_x);
    auto const scaled_y = midpoint_y / (max_y / std::abs(y));
    auto draw_y =
      static_cast<int>(y > 0 ? midpoint_y + scaled_y : midpoint_y - scaled_y);

    old_obj_tmp =
      ::SelectObject(mem_dc.GetHandle(), ::GetStockObject(DC_BRUSH));
    if (!old_obj_tmp || old_obj_tmp == HGDI_ERROR)
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"SelectObject failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }

    if (::SetDCBrushColor(mem_dc.GetHandle(), unit_data.colour_) == CLR_INVALID)
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"SetDCBrushColor failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }

    if (!::Ellipse(
          mem_dc.GetHandle(), draw_x - 5, draw_y - 5, draw_x + 5, draw_y + 5))
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"Ellipse failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }

    if (::SetTextColor(mem_dc.GetHandle(), RGB(0, 0, 0)) == CLR_INVALID)
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"SetTextColor failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }

    RECT text_rect{};
    text_rect.top = draw_y - 5;
    text_rect.left = draw_x + 10;
    auto const name = unit_data.name_;
    if (!::DrawTextA(
          mem_dc.GetHandle(), name, -1, &text_rect, DT_SINGLELINE | DT_NOCLIP))
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"DrawTextA failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }
  }

  old_obj_tmp =
    ::SelectObject(mem_dc.GetHandle(), ::GetStockObject(GRAY_BRUSH));
  if (!old_obj_tmp || old_obj_tmp == HGDI_ERROR)
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"SelectObject failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  if (!::Ellipse(mem_dc.GetHandle(),
                 midpoint_x - 5,
                 midpoint_y - 5,
                 midpoint_x + 5,
                 midpoint_y + 5))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{}
                                    << hadesmem::ErrorString{"Ellipse failed."}
                                    << hadesmem::ErrorCodeWinLast{last_error});
  }

  if (!::BitBlt(radar_wnd_dc,
                0,
                0,
                radar_width,
                radar_height,
                mem_dc.GetHandle(),
                0,
                0,
                SRCCOPY))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(hadesmem::Error{}
                                    << hadesmem::ErrorString{"BitBlt failed."}
                                    << hadesmem::ErrorCodeWinLast{last_error});
  }
}

void RootWindow::UpdateRadar(RadarData* radar_data)
{
  radar_data_.player_ = radar_data->player_;
  radar_data_.units_ = std::move(radar_data->units_);

  RECT rr{};
  if (!::GetClientRect(radar_display_wnd_, &rr))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"GetClientRect failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }

  if (!::InvalidateRect(radar_display_wnd_, &rr, TRUE))
  {
    DWORD const last_error = ::GetLastError();
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"InvalidateRect failed."}
                        << hadesmem::ErrorCodeWinLast{last_error});
  }
}

LRESULT CALLBACK
  RootWindow::RadarWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  RootWindow* parent_wnd_class = nullptr;

  try
  {
    auto const parent_wnd =
      reinterpret_cast<HWND>(::GetWindowLongPtrW(hwnd, GWLP_HWNDPARENT));
    if (!parent_wnd)
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"GetWindowLongPtrW failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }

    parent_wnd_class = reinterpret_cast<RootWindow*>(
      ::GetWindowLongPtrW(parent_wnd, GWLP_USERDATA));
    if (!parent_wnd_class)
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"GetWindowLongPtrW failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }
  }
  catch (...)
  {
    HADESMEM_DETAIL_TRACE_A(
      boost::current_exception_diagnostic_information().c_str());
    HADESMEM_DETAIL_ASSERT(false);
  }

  if (!parent_wnd_class)
  {
    // Not sure what the best thing to do here is.
    return ::DefWindowProcW(hwnd, msg, wparam, lparam);
  }

  switch (msg)
  {
  case WM_PAINT:
    // Not bothering to call the original wndproc first because we're going to
    // overwrite the entire window anyway.
    try
    {
      parent_wnd_class->PaintRadar(hwnd);
    }
    catch (...)
    {
      HADESMEM_DETAIL_TRACE_A(
        boost::current_exception_diagnostic_information().c_str());
      HADESMEM_DETAIL_ASSERT(false);
    }
    return 0;
  }

  return parent_wnd_class->radar_prev_wndproc_(hwnd, msg, wparam, lparam);
}

LRESULT RootWindow::OnSize(LPARAM lparam)
{
  if (child_wnd_)
  {
    if (!::SetWindowPos(child_wnd_,
                        nullptr,
                        0,
                        0,
                        GET_X_LPARAM(lparam),
                        GET_Y_LPARAM(lparam),
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
                        GET_X_LPARAM(lparam),
                        GET_Y_LPARAM(lparam),
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

LRESULT RootWindow::OnNotify(LPARAM lparam)
{
  auto const notif_hdr = reinterpret_cast<LPNMHDR>(lparam);
  switch (notif_hdr->code)
  {
  case TCN_SELCHANGING:
  {
    return FALSE;
  }

  case TCN_SELCHANGE:
  {
    int const tab_index = TabCtrl_GetCurSel(tab_wnd_);
    switch (tab_index)
    {
    case Tab::kRadar:
      ShowWindow(config_display_wnd_, SW_HIDE);
      ShowWindow(radar_display_wnd_, SW_SHOW);
      break;

    case Tab::kConfig:
      ShowWindow(radar_display_wnd_, SW_HIDE);
      ShowWindow(config_display_wnd_, SW_SHOW);
      break;
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

LRESULT RootWindow::HandleMessage(UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch (msg)
  {
  case WM_CREATE:
    return OnCreate();

  case WM_NCDESTROY:
    GetRadarEnabled() = false;
    GetRadarFrameProcessed() = true;
    GetRadarFrameConditionVariable().notify_one();
    ::PostQuitMessage(0);
    break;

  case WM_SIZE:
    OnSize(lparam);
    return 0;

  case WM_SETFOCUS:
    return OnFocus();

  case WM_NOTIFY:
    return OnNotify(lparam);

  case WM_APP_UPDATE_RADAR:
  {
    std::lock_guard<std::mutex> lock(GetRadarFrameMutex());
    auto const cleanup_frame = [&]()
    {
      GetRadarFrameProcessed() = true;
      GetRadarFrameConditionVariable().notify_one();
    };
    auto const scope_cleanup_frame =
      hadesmem::detail::MakeScopeWarden(cleanup_frame);
    UpdateRadar(reinterpret_cast<RadarData*>(wparam));
    return 0;
  }
  }

  return super::HandleMessage(msg, wparam, lparam);
}

RootWindow* RootWindow::Create(HINSTANCE instance)
{
  RECT work_area{};
  int cx = CW_USEDEFAULT;
  int cy = CW_USEDEFAULT;
  if (::SystemParametersInfoW(SPI_GETWORKAREA, 0, &work_area, 0))
  {
    auto const height = work_area.bottom - work_area.top;
    auto const width = work_area.right - work_area.left;
    auto const dimensions = static_cast<int>((std::min)(height, width) * 0.9);
    cx = cy = dimensions;
  }

  auto self = std::make_unique<RootWindow>(instance);
  if (self->WinCreateWindow(0,
                            L"Scratch",
                            WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            cx,
                            cy,
                            nullptr,
                            nullptr))
  {
    return self.release();
  }

  return nullptr;
}

int PASCAL WindowThread(HINSTANCE instance,
                        HINSTANCE /*prev_instance*/,
                        LPSTR /*cmd_line*/,
                        int show_cmd)
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

    RootWindow* prw = RootWindow::Create(instance);
    if (!prw)
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{
                               "Failed to create root window."}
                          << hadesmem::ErrorCodeWinLast{last_error});
    }

    std::wstring const class_name = prw->ClassName();

    auto& hwnd = GetWindowHandle();
    hwnd = prw->GetHandle();

    (void)::ShowWindow(hwnd, show_cmd);

    MSG msg;
    while (::GetMessageW(&msg, nullptr, 0, 0))
    {
      ::TranslateMessage(&msg);
      ::DispatchMessageW(&msg);
    }

    if (!::UnregisterClassW(class_name.c_str(), instance))
    {
      DWORD const last_error = ::GetLastError();
      HADESMEM_DETAIL_THROW_EXCEPTION(
        hadesmem::Error{} << hadesmem::ErrorString{"UnregisterClassW failed."}
                          << hadesmem::ErrorCodeWinLast{last_error});
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
