/*
This file is part of HadesMem.
Copyright © 2010 Cypherjb (aka Chazwazza, aka Cypher). 
<http://www.cypherjb.com/> <cypher.jb@gmail.com>

HadesMem is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

HadesMem is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with HadesMem.  If not, see <http://www.gnu.org/licenses/>.
*/

// C++ Standard Library
#include <iostream>

// Hades
#include "InputMgr.h"

namespace Hades
{
  namespace Input
  {
    // Hades manager
    Kernel::Kernel* InputMgr::m_pKernel = nullptr;

    // Target windows
    std::map<HWND, WNDPROC> InputMgr::m_TargetWindows;

    // Callback managers
    InputMgr::OnWindowMessageCallbacks InputMgr::m_CallsOnWndMsg;
    InputMgr::OnSetCursorCallbacks InputMgr::m_CallsOnSetCursor;
    InputMgr::OnGetCursorPosCallbacks InputMgr::m_CallsOnGetCursorPos;
    InputMgr::OnSetCursorPosCallbacks InputMgr::m_CallsOnSetCursorPos;

    // SetCursor hook
    std::shared_ptr<Memory::PatchDetour> InputMgr::m_pSetCursorHk;

    // SetCursorPos hook
    std::shared_ptr<Memory::PatchDetour> InputMgr::m_pSetCursorPosHk;

    // GetCursorPos hook
    std::shared_ptr<Memory::PatchDetour> InputMgr::m_pGetCursorPosHk;

    // Initialize input subsystem
    void InputMgr::Startup(Kernel::Kernel* pKernel)
    {
      // Set HadesMgr pointer
      m_pKernel = pKernel;

      // Load User32
      // Todo: Defer hooking until game loads User32 (e.g. via LoadLibrary 
      // hook).
      HMODULE User32Mod = LoadLibrary(L"User32.dll");
      if (!User32Mod)
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(InputMgrError() << 
          ErrorFunction("InputMgr::Startup") << 
          ErrorString("Could not load User32.") << 
          ErrorCodeWin(LastError));
      }

      // Hook if required
      if (!m_pSetCursorHk && m_pKernel->IsHookEnabled(L"user32.dll!SetCursor"))
      {
        // Get address of SetCursor
        FARPROC pSetCursor = GetProcAddress(User32Mod, "SetCursor");
        if (!pSetCursor)
        {
          DWORD LastError = GetLastError();
          BOOST_THROW_EXCEPTION(InputMgrError() << 
            ErrorFunction("InputMgr::Startup") << 
            ErrorString("Could not get address of SetCursor.") << 
            ErrorCodeWin(LastError));
        }

        // Target and detour pointer
        PBYTE Target = reinterpret_cast<PBYTE>(pSetCursor);
        PBYTE Detour = reinterpret_cast<PBYTE>(&SetCursor_Hook);

        // Debug output
        std::wcout << "InputMgr::Startup: Hooking user32.dll!SetCursor." << 
          std::endl;
        std::wcout << boost::wformat(L"InputMgr::Startup: Target = %p, "
          L"Detour = %p.") %Target %Detour << std::endl;

        // Hook user32.dll!SetCursor
        m_pSetCursorHk.reset(new Hades::Memory::PatchDetour(*pKernel->
          GetMemoryMgr(), Target, Detour));
        m_pSetCursorHk->Apply();
      }

      // Hook if required
      if (!m_pGetCursorPosHk && m_pKernel->IsHookEnabled(
        L"user32.dll!GetCursorPos"))
      {
        // Get address of GetCursorPos
        FARPROC pGetCursorPos = GetProcAddress(User32Mod, "GetCursorPos");
        if (!pGetCursorPos)
        {
          DWORD LastError = GetLastError();
          BOOST_THROW_EXCEPTION(InputMgrError() << 
            ErrorFunction("InputMgr::Startup") << 
            ErrorString("Could not get address of GetCursorPos.") << 
            ErrorCodeWin(LastError));
        }

        // Target and detour pointer
        PBYTE Target = reinterpret_cast<PBYTE>(pGetCursorPos);
        PBYTE Detour = reinterpret_cast<PBYTE>(&GetCursorPos_Hook);

        // Debug output
        std::wcout << "InputMgr::Startup: Hooking user32.dll!GetCursorPos." << 
          std::endl;
        std::wcout << boost::wformat(L"InputMgr::Startup: Target = %p, "
          L"Detour = %p.") %Target %Detour << std::endl;

        // Hook user32.dll!GetCursorPos
        m_pGetCursorPosHk.reset(new Hades::Memory::PatchDetour(*pKernel->
          GetMemoryMgr(), Target, Detour));
        m_pGetCursorPosHk->Apply();
      }

      // Hook if required
      if (!m_pSetCursorPosHk && m_pKernel->IsHookEnabled(
        L"user32.dll!SetCursorPos"))
      {
        // Get address of SetCursorPos
        FARPROC pSetCursorPos = GetProcAddress(User32Mod, "SetCursorPos");
        if (!pSetCursorPos)
        {
          DWORD LastError = GetLastError();
          BOOST_THROW_EXCEPTION(InputMgrError() << 
            ErrorFunction("InputMgr::Startup") << 
            ErrorString("Could not get address of SetCursorPos.") << 
            ErrorCodeWin(LastError));
        }

        // Target and detour pointer
        PBYTE Target = reinterpret_cast<PBYTE>(pSetCursorPos);
        PBYTE Detour = reinterpret_cast<PBYTE>(&SetCursorPos_Hook);

        // Debug output
        std::wcout << "InputMgr::Startup: Hooking user32.dll!SetCursorPos." << 
          std::endl;
        std::wcout << boost::wformat(L"InputMgr::Startup: Target = %p, "
          L"Detour = %p.") %Target %Detour << std::endl;

        // Hook user32.dll!SetCursorPos
        m_pSetCursorPosHk.reset(new Hades::Memory::PatchDetour(*pKernel->
          GetMemoryMgr(), Target, Detour));
        m_pSetCursorPosHk->Apply();
      }
    }

    // Window hook procedure
    LRESULT CALLBACK InputMgr::MyWindowProc(HWND hwnd, UINT uMsg, 
      WPARAM wParam, LPARAM lParam)
    {
      // Call registered callbacks and block input if requested
      return *m_CallsOnWndMsg(hwnd, uMsg, wParam, lParam) ? CallWindowProc(
        m_TargetWindows[hwnd], hwnd, uMsg, wParam, lParam) : 0;
    }

    // Hook target window
    void InputMgr::HookWindow(HWND Window)
    {
      try
      {
        // Get target window procedure
        auto CurProc = reinterpret_cast<WNDPROC>(GetWindowLongPtr(Window, 
          GWLP_WNDPROC));
        if (CurProc == &MyWindowProc)
        {
          std::wcout << "InputMgr::HookWindow: Warning! Attempting to hook an "
            "already hooked window." << std::endl;
          return;
        }

        // Set target window and hook
        auto OrigProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(
          Window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&MyWindowProc)));
        if (!OrigProc)
        {
          DWORD LastError = GetLastError();
          BOOST_THROW_EXCEPTION(InputMgrError() << 
            ErrorFunction("InputMgr::HookWindow") << 
            ErrorString("Could not set new window procedure.") << 
            ErrorCodeWin(LastError));
        }

        // Store window procedure
        m_TargetWindows[Window] = OrigProc;
      }
      catch (boost::exception const& e)
      {
        // Debug output
        std::cout << boost::format("InputMgr::HookWindow: Error! %s.") 
          %boost::diagnostic_information(e) << std::endl;
      }
      catch (std::exception const& e)
      {
        // Debug output
        std::cout << boost::format("InputMgr::HookWindow: Error! %s.") 
          %e.what() << std::endl;
      }
    }

    boost::signals2::connection InputMgr::RegisterOnWindowMessage(
      OnWindowMessageCallbacks::slot_type const& Subscriber)
    {
      // Register callback and return connection
      return m_CallsOnWndMsg.connect(Subscriber);
    }

    boost::signals2::connection InputMgr::RegisterOnSetCursor(
      OnSetCursorCallbacks::slot_type const& Subscriber)
    {
      // Register callback and return connection
      return m_CallsOnSetCursor.connect(Subscriber);
    }

    boost::signals2::connection InputMgr::RegisterOnGetCursorPos(
      OnGetCursorPosCallbacks::slot_type const& Subscriber )
    {
      // Register callback and return connection
      return m_CallsOnGetCursorPos.connect(Subscriber);
    }

    boost::signals2::connection InputMgr::RegisterOnSetCursorPos(
      OnSetCursorPosCallbacks::slot_type const& Subscriber )
    {
      // Register callback and return connection
      return m_CallsOnSetCursorPos.connect(Subscriber);
    }

    HCURSOR WINAPI InputMgr::SetCursor_Hook(HCURSOR Cursor)
    {
      // Get trampoline6
      typedef HCURSOR (WINAPI* tSetCursor)(HCURSOR Cursor);
      auto pSetCursor = reinterpret_cast<tSetCursor>(m_pSetCursorHk->
        GetTrampoline());

      return *m_CallsOnSetCursor(Cursor) ? pSetCursor(Cursor) : nullptr;
    }

    // Hook function for GetCursorPos
    BOOL WINAPI InputMgr::GetCursorPos_Hook(LPPOINT lpPoint)
    {
      // Get trampoline6
      typedef BOOL (WINAPI* tGetCursorPos)(LPPOINT lpPoint);
      auto pGetCursorPos = reinterpret_cast<tGetCursorPos>(m_pGetCursorPosHk->
        GetTrampoline());

      return *m_CallsOnGetCursorPos(lpPoint) ? pGetCursorPos(lpPoint) : TRUE;
    }

    // Hook function for SetCursorPos
    BOOL WINAPI InputMgr::SetCursorPos_Hook(int X, int Y)
    {
      // Get trampoline6
      typedef BOOL (WINAPI* tSetCursorPos)(int X, int Y);
      auto pSetCursorPos = reinterpret_cast<tSetCursorPos>(m_pSetCursorPosHk->
        GetTrampoline());

      return *m_CallsOnSetCursorPos(X, Y) ? pSetCursorPos(X, Y) : TRUE;
    }
  }
}
