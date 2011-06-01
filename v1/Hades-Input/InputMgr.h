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

#pragma once

// C++ Standard Library
#include <map>

// Windows API
#include <Windows.h>
#include <atlbase.h>
#include <atlwin.h>

// Boost
#pragma warning(push, 1)
#pragma warning(disable: 4267)
#include <boost/signals2.hpp>
#pragma warning(pop)

// Hades
#include "Hades-Common/Error.h"
#include "Hades-Kernel/Kernel.h"
#include "Hades-Memory/Patcher.h"

namespace Hades
{
  namespace Input
  {
    // InputMgr exception type
    class InputMgrError : public virtual HadesError 
    { };

    // Input managing class
    class InputMgr
    {
    public:
      // Initialize input subsystem
      static void Startup(Kernel::Kernel* pKernel);

      // Hook window for input
      static void HookWindow(HWND Window);

      // Callback types
      typedef boost::signals2::signal<bool (HWND hwnd, UINT uMsg, WPARAM 
        wParam, LPARAM lParam)> OnWindowMessageCallbacks;
      typedef boost::signals2::signal<bool (HCURSOR hCursor)> 
        OnSetCursorCallbacks;
      typedef boost::signals2::signal<bool (LPPOINT lpPoint)> 
        OnGetCursorPosCallbacks;
      typedef boost::signals2::signal<bool (int X, int Y)> 
        OnSetCursorPosCallbacks;

      // Register callback for OnWindowMsg event
      static boost::signals2::connection RegisterOnWindowMessage(
        OnWindowMessageCallbacks::slot_type const& Subscriber);

      // Register callback for OnSetCursor event
      static boost::signals2::connection RegisterOnSetCursor(
        OnSetCursorCallbacks::slot_type const& Subscriber);

      // Register callback for OnGetCursorPos event
      static boost::signals2::connection RegisterOnGetCursorPos(
        OnGetCursorPosCallbacks::slot_type const& Subscriber);

      // Register callback for OnSetCursorPos event
      static boost::signals2::connection RegisterOnSetCursorPos(
        OnSetCursorPosCallbacks::slot_type const& Subscriber);

    private:
      // SetCursor hook
      static HCURSOR WINAPI SetCursor_Hook(HCURSOR Cursor);

      // GetCursorPos hook
      static BOOL WINAPI GetCursorPos_Hook(LPPOINT lpPoint);

      // SetCursorPos hook
      static BOOL WINAPI SetCursorPos_Hook(int X, int Y);

      // Window hook procedure
      static LRESULT CALLBACK MyWindowProc(
        HWND hwnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam);

      // Hades manager
      static Kernel::Kernel* m_pKernel;

      // Target windows and previous window procedures
      static std::map<HWND, WNDPROC> m_TargetWindows;

      // Callback managers
      static OnWindowMessageCallbacks m_CallsOnWndMsg;
      static OnSetCursorCallbacks m_CallsOnSetCursor;
      static OnGetCursorPosCallbacks m_CallsOnGetCursorPos;
      static OnSetCursorPosCallbacks m_CallsOnSetCursorPos;

      // SetCursor hook
      static std::shared_ptr<Memory::PatchDetour> m_pSetCursorHk;

      // GetCursorPos hook
      static std::shared_ptr<Memory::PatchDetour> m_pGetCursorPosHk;

      // SetCursorPos hook
      static std::shared_ptr<Memory::PatchDetour> m_pSetCursorPosHk;
    };

    // Input managing class wrapper
    class InputMgrWrapper
    {
    public:
      virtual void Startup(Kernel::Kernel* pHades)
      {
        return InputMgr::Startup(pHades);
      }

      virtual void HookWindow(HWND Window)
      {
        return InputMgr::HookWindow(Window);
      }

      virtual boost::signals2::connection RegisterOnWindowMessage(
        const InputMgr::OnWindowMessageCallbacks::slot_type& Subscriber)
      {
        return InputMgr::RegisterOnWindowMessage(Subscriber);
      }

      virtual boost::signals2::connection RegisterOnSetCursor(
        InputMgr::OnSetCursorCallbacks::slot_type const& Subscriber)
      {
        return InputMgr::RegisterOnSetCursor(Subscriber);
      }

      virtual boost::signals2::connection RegisterOnGetCursorPos(
        InputMgr::OnGetCursorPosCallbacks::slot_type const& Subscriber)
      {
        return InputMgr::RegisterOnGetCursorPos(Subscriber);
      }

      virtual boost::signals2::connection RegisterOnSetCursorPos(
        InputMgr::OnSetCursorPosCallbacks::slot_type const& Subscriber)
      {
        return InputMgr::RegisterOnSetCursorPos(Subscriber);
      }
    };
  }
}
