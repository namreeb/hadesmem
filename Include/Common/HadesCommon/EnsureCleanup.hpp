/*
This file is part of HadesMem.
Copyright (C) 2010 Joshua Boyce (aka RaptorFactor, Cypherjb, Cypher, Chazwazza).
<http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

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
#include <string>

// Boost
#pragma warning(push, 1)
#include <boost/noncopyable.hpp>
#pragma warning(pop)

// Windows API
#include <Windows.h>
#include <Objbase.h>

// Notice: Modified version of EnsureCleanup library provided in the 'Windows
// via C/C++' sample code. Originally copyright Jeffrey Richter and
// Christophe Nasarre.

namespace Hades
{
  namespace Windows
  {
    // Windows RAII helper class template
    // HandleT = Handle type (e.g. 'HANDLE')
    // FuncT = Function prototype (e.g. 'BOOL (WINAPI*) (HANDLE)' )
    // CleanupFn = Cleanup function (e.g. 'CloseHandle')
    // Invalid = Invalid handle value (e.g. '0')
    template <typename HandleT, typename FuncT, FuncT CleanupFn, 
      HandleT Invalid>
    class EnsureCleanup : private boost::noncopyable
    {
    public:
      // Ensure size of handle type is valid. Under Windows all handles are 
      // the size of a pointer.
      static_assert(sizeof(HandleT) == sizeof(DWORD_PTR), 
        "Size of handle type is invalid.");

      // Constructor
      EnsureCleanup(HandleT Handle = Invalid)
        : m_Handle(Handle)
      { }

      // Move constructor
      EnsureCleanup(EnsureCleanup&& MyEnsureCleanup)
        : m_Handle(Invalid)
      {
        *this = std::move(MyEnsureCleanup);
      }

      // Move assignment operator
      EnsureCleanup& operator= (EnsureCleanup&& MyEnsureCleanup)
      {
        Cleanup();

        this->m_Handle = MyEnsureCleanup.m_Handle;
        MyEnsureCleanup.m_Handle = Invalid;

        return *this;
      }

      // Assignment operator (for HandleT values)
      EnsureCleanup& operator= (HandleT Handle)
      {
        Cleanup();

        m_Handle = Handle;

        return *this;
      }

      // The destructor performs the cleanup.
      ~EnsureCleanup()
      {
        Cleanup();
      }

      // Whether object is valid
      BOOL IsValid() const
      {
        return m_Handle != Invalid;
      }

      // Whether object is invalid
      BOOL IsInvalid() const
      {
        return !IsValid();
      }

      // Implicit conversion operator for HandleT
      operator HandleT() const
      {
        return m_Handle;
      }

      // Cleanup the object if the value represents a valid object
      void Cleanup()
      {
        if (IsValid())
        {
          // Close the object.
          CleanupFn(m_Handle);

          // We no longer represent a valid object.
          m_Handle = Invalid;
        }
      }

    private:
      // Handle being managed
      HandleT m_Handle;
    };
    
    // Instances of the template C++ class for common data types.
    typedef EnsureCleanup<HANDLE, BOOL(WINAPI*)(HANDLE), FindClose, nullptr> 
      EnsureFindClose;
    typedef EnsureCleanup<HANDLE, BOOL(WINAPI*)(HANDLE), CloseHandle, nullptr> 
      EnsureCloseHandle;
    typedef EnsureCleanup<HANDLE, BOOL(WINAPI*)(HANDLE), CloseHandle, 
      INVALID_HANDLE_VALUE> EnsureCloseSnap;
    typedef EnsureCleanup<HLOCAL, HLOCAL(WINAPI*)(HLOCAL), LocalFree, nullptr> 
      EnsureLocalFree;
    typedef EnsureCleanup<HGLOBAL, HGLOBAL(WINAPI*)(HGLOBAL), GlobalFree, 
      nullptr> EnsureGlobalFree;
    typedef EnsureCleanup<HGLOBAL, BOOL(WINAPI*)(HGLOBAL), GlobalUnlock, 
      nullptr> EnsureGlobalUnlock;
    typedef EnsureCleanup<HKEY, LONG(WINAPI*)(HKEY), RegCloseKey, nullptr> 
      EnsureRegCloseKey;
    typedef EnsureCleanup<SC_HANDLE, BOOL(WINAPI*)(SC_HANDLE), 
      CloseServiceHandle, 0> EnsureCloseServiceHandle;
    typedef EnsureCleanup<HWINSTA, BOOL(WINAPI*)(HWINSTA), CloseWindowStation, 
      0> EnsureCloseWindowStation;
    typedef EnsureCleanup<HDESK, BOOL(WINAPI*)(HDESK), CloseDesktop, nullptr> 
      EnsureCloseDesktop;
    typedef EnsureCleanup<LPCVOID, BOOL(WINAPI*)(LPCVOID), UnmapViewOfFile, 
      nullptr> EnsureUnmapViewOfFile;
    typedef EnsureCleanup<HMODULE, BOOL(WINAPI*)(HMODULE), FreeLibrary, 
      nullptr> EnsureFreeLibrary;
    typedef EnsureCleanup<PVOID, ULONG(WINAPI*)(PVOID), 
      RemoveVectoredExceptionHandler, 0> EnsureRemoveVEH;
    typedef EnsureCleanup<HANDLE, DWORD(WINAPI*)(HANDLE), ResumeThread, 
      nullptr> EnsureResumeThread;
    typedef EnsureCleanup<HANDLE, BOOL(WINAPI*)(HANDLE), CloseHandle, 
      INVALID_HANDLE_VALUE> EnsureCloseFile;
    typedef EnsureCleanup<HHOOK, BOOL(WINAPI*)(HHOOK), UnhookWindowsHookEx, 
      nullptr> EnsureUnhookWindowsHookEx;
    typedef EnsureCleanup<HWND, BOOL(WINAPI*)(HWND), DestroyWindow, nullptr> 
      EnsureDestroyWindow;
    typedef EnsureCleanup<PSID, PVOID(WINAPI*)(PSID), FreeSid, nullptr> 
      EnsureFreeSid;
    typedef EnsureCleanup<HGLOBAL, BOOL(WINAPI*)(HGLOBAL), FreeResource, 
      nullptr> EnsureFreeResource;
    typedef EnsureCleanup<HDC, BOOL(WINAPI*)(HDC), DeleteDC, nullptr> 
      EnsureDeleteDc;
    typedef EnsureCleanup<HBITMAP, BOOL(WINAPI*)(HGDIOBJ), DeleteObject, 
      nullptr> EnsureDeleteObject;
    typedef EnsureCleanup<HICON, BOOL(WINAPI*)(HICON), DestroyIcon, nullptr> 
      EnsureDestroyIcon;
    typedef EnsureCleanup<HMENU, BOOL(WINAPI*)(HMENU), DestroyMenu, nullptr> 
      EnsureDestroyMenu;
      
    // Special class for ensuring the 'LastError' is restored in hooks
    class EnsureLastError : private boost::noncopyable
    {
    public:
      EnsureLastError() 
        : m_LastError(GetLastError())
      { }
      
      ~EnsureLastError()
      {
        SetLastError(m_LastError);
      }
    
    private:
      DWORD m_LastError;
    };

    // Special class for ensuring COM is uninitialized
    class EnsureCoUninitialize : private boost::noncopyable
    {
    public:
      ~EnsureCoUninitialize()
      {
        CoUninitialize();
      }
    };

    // Special class for releasing a reserved region.
    class EnsureReleaseRegion : private boost::noncopyable
    {
    public:
      // Constructor
      EnsureReleaseRegion(PVOID pv = nullptr)
        : m_pv(pv)
      { }

      // Destructor
      ~EnsureReleaseRegion()
      {
        Cleanup();
      }

      // Move constructor
      EnsureReleaseRegion(EnsureReleaseRegion&& MyEnsureCleanup)
        : m_pv(nullptr)
      {
        *this = std::move(MyEnsureCleanup);
      }

      // Move assignment operator
      EnsureReleaseRegion& operator= (EnsureReleaseRegion&& MyEnsureCleanup)
      {
        Cleanup();

        m_pv = MyEnsureCleanup.m_pv;

        MyEnsureCleanup.m_pv = 0;

        return *this;
      }

      // Assignment operator (for PVOID values) 
      EnsureReleaseRegion& operator= (PVOID pv)
      {
        Cleanup();

        m_pv = pv;

        return *this;
      }

      // Implicit conversion operator for PVOID
      operator PVOID() const
      {
        return m_pv;
      }

      // Cleanup the object if the value represents a valid object
      void Cleanup()
      {
        if (m_pv != nullptr)
        {
          VirtualFree(m_pv, 0, MEM_RELEASE);

          m_pv = nullptr;
        }
      }

    private:
      // Handle being managed
      PVOID m_pv;
    };

    // Special class for releasing a reserved region.
    class EnsureEndUpdateResource : private boost::noncopyable
    {
    public:
      // Constructor
      EnsureEndUpdateResource(HANDLE File = nullptr) 
        : m_File(File)
      { }

      // Move constructor
      EnsureEndUpdateResource(EnsureEndUpdateResource&& MyEnsureCleanup)
        : m_File(nullptr)
      {
        *this = std::move(MyEnsureCleanup);
      }

      // Move assignment operator
      EnsureEndUpdateResource& operator= (EnsureEndUpdateResource&&
        MyEnsureCleanup)
      {
        Cleanup();

        m_File = MyEnsureCleanup.m_File;

        MyEnsureCleanup.m_File = nullptr;

        return *this;
      }

      // Destructor
      ~EnsureEndUpdateResource()
      {
        Cleanup();
      }

      // Assignment operator (for HANDLE values) 
      EnsureEndUpdateResource& operator= (HANDLE File)
      {
        Cleanup();

        m_File = File;

        return *this;
      }

      // Implicit conversion operator for HANDLE
      operator HANDLE() const
      {
        return m_File;
      }

      // Cleanup the object if the value represents a valid object
      void Cleanup()
      {
        if (m_File != nullptr)
        {
          EndUpdateResource(m_File, FALSE);

          m_File = nullptr;
        }
      }

    private:
      // Handle being managed
      HANDLE m_File;
    };

    // Special class for freeing a block from a heap
    class EnsureHeapFree : private boost::noncopyable
    {
    public:
      // Constructor
      EnsureHeapFree(PVOID pv = nullptr, HANDLE hHeap = GetProcessHeap())
        : m_pv(pv), m_hHeap(hHeap)
      { }

      // Move constructor
      EnsureHeapFree(EnsureHeapFree&& MyEnsureCleanup)
        : m_pv(nullptr),
        m_hHeap(nullptr)
      {
        *this = std::move(MyEnsureCleanup);
      }

      // Move assignment operator
      EnsureHeapFree& operator= (EnsureHeapFree&& MyEnsureCleanup)
      {
        Cleanup();

        m_pv = MyEnsureCleanup.m_pv;
        m_hHeap = MyEnsureCleanup.m_hHeap;

        MyEnsureCleanup.m_pv = nullptr;
        MyEnsureCleanup.m_hHeap = nullptr;

        return *this;
      }

      // Destructor
      ~EnsureHeapFree()
      {
        Cleanup();
      }

      // Assignment operator (for PVOID values)
      EnsureHeapFree& operator= (PVOID pv)
      {
        Cleanup();

        m_pv = pv;

        return *this;
      }

      // Implicit conversion operator for PVOID
      operator PVOID() const
      {
        return m_pv;
      }

      // Cleanup the object if the value represents a valid object
      void Cleanup()
      {
        if (m_pv != nullptr)
        {
          HeapFree(m_hHeap, 0, m_pv);

          m_pv = nullptr;
        }
      }

    private:
      // Handles being managed
      PVOID m_pv;
      HANDLE m_hHeap;
    };

    // Special class for releasing a remote reserved region
    class EnsureReleaseRegionEx : private boost::noncopyable
    {
    public:
      // Constructor
      EnsureReleaseRegionEx(PVOID pv = nullptr, HANDLE proc = nullptr)
        : m_pv(pv), 
        m_proc(proc)
      { }

      // Move constructor
      EnsureReleaseRegionEx(EnsureReleaseRegionEx&& MyEnsureCleanup)
        : m_pv(nullptr),
        m_proc(nullptr)
      {
        *this = std::move(MyEnsureCleanup);
      }

      // Move assignment operator
      EnsureReleaseRegionEx& operator= (EnsureReleaseRegionEx&&
        MyEnsureCleanup)
      {
        Cleanup();

        m_pv = MyEnsureCleanup.m_pv;
        m_proc = MyEnsureCleanup.m_proc;

        MyEnsureCleanup.m_pv = nullptr;
        MyEnsureCleanup.m_proc = nullptr;

        return *this;
      }

      // Destructor
      ~EnsureReleaseRegionEx()
      {
        Cleanup();
      }

      // Assignment operator (for PVOID values)
      EnsureReleaseRegionEx& operator= (PVOID pv)
      {
        Cleanup();

        m_pv = pv;

        return *this;
      }

      // Implicit conversion operator for PVOID
      operator PVOID() const
      {
        return m_pv;
      }

      // Cleanup the object if the value represents a valid object
      void Cleanup()
      {
        if (m_pv != nullptr && m_proc != nullptr)
        {
          VirtualFreeEx(m_proc, m_pv, 0, MEM_RELEASE);

          m_pv = nullptr;
        }
      }

    private:
      // Handles being managed
      PVOID m_pv;
      HANDLE m_proc;
    };

    // Special class for closing the clipboard
    class EnsureCloseClipboard : private boost::noncopyable
    {
    public:
      // Constructor
      EnsureCloseClipboard(BOOL Success) 
        : m_Success(Success)
      { }
      
      // Move constructor
      EnsureCloseClipboard(EnsureCloseClipboard&& MyEnsureCleanup)
        : m_Success(FALSE)
      {
        *this = std::move(MyEnsureCleanup);
      }

      // Move assignment operator
      EnsureCloseClipboard& operator= (EnsureCloseClipboard&& MyEnsureCleanup)
      {
        Cleanup();

        m_Success = MyEnsureCleanup.m_Success;

        MyEnsureCleanup.m_Success = FALSE;

        return *this;
      }

      // Destructor
      ~EnsureCloseClipboard()
      {
        Cleanup();
      }

      // Assignment operator (for BOOL values)
      EnsureCloseClipboard& operator= (BOOL Success)
      {
        Cleanup();

        m_Success = Success;

        return *this;
      }

      // Implicit conversion operator for BOOL
      operator BOOL() const
      {
        return m_Success;
      }

      // Cleanup the object if the value represents a valid object
      void Cleanup()
      {
        if (m_Success)
        {
          CloseClipboard();

          m_Success = FALSE;
        }
      }

    private:
      // 'Handle' being managed
      BOOL m_Success;
    };

    // Special class for releasing a window class
    class EnsureUnregisterClass : private boost::noncopyable
    {
    public:
      // Constructor
      EnsureUnregisterClass(std::wstring const& ClassName, 
        HINSTANCE Instance)
        : m_ClassName(ClassName),
        m_Instance(Instance)
      { }

      // Move constructor
      EnsureUnregisterClass(EnsureUnregisterClass&& MyEnsureCleanup)
        : m_ClassName(),
        m_Instance()
      {
        *this = std::move(MyEnsureCleanup);
      }

      // Move assignment operator
      EnsureUnregisterClass& operator= (EnsureUnregisterClass&&
        MyEnsureCleanup)
      {
        Cleanup();

        m_ClassName = std::move(MyEnsureCleanup.m_ClassName);
        m_Instance = MyEnsureCleanup.m_Instance;

        MyEnsureCleanup.m_ClassName = std::wstring();
        MyEnsureCleanup.m_Instance = nullptr;

        return *this;
      }

      // Destructor
      ~EnsureUnregisterClass()
      {
        Cleanup();
      }

      // Cleanup the object if the value represents a valid object
      void Cleanup()
      {
        if (!m_ClassName.empty() && m_Instance)
        {
          UnregisterClass(m_ClassName.c_str(), m_Instance);

          m_ClassName.clear();
          m_Instance = 0;
        }
      }

    private:
      // 'Handles' being managed
      std::wstring m_ClassName;
      HINSTANCE m_Instance;
    };


    // Special class for releasing a DC
    class EnsureReleaseDc
    {
    public:
      // Constructor
      EnsureReleaseDc(HWND Wnd = nullptr, HDC Dc = nullptr)
        : m_Wnd(Wnd),
        m_Dc(Dc)
      { }

      // Move constructor
      EnsureReleaseDc(EnsureReleaseDc&& MyEnsureCleanup)
        : m_Wnd(nullptr),
        m_Dc(nullptr)
      {
        *this = std::move(MyEnsureCleanup);
      }

      // Move assignment operator
      EnsureReleaseDc& operator= (EnsureReleaseDc&& MyEnsureCleanup)
      {
        Cleanup();

        m_Wnd = MyEnsureCleanup.m_Wnd;
        m_Dc = MyEnsureCleanup.m_Dc;

        MyEnsureCleanup.m_Wnd = nullptr;
        MyEnsureCleanup.m_Dc = nullptr;

        return *this;
      }

      // Destructor
      ~EnsureReleaseDc()
      {
        Cleanup();
      }

      // Assignment operator (for HDC values)
      EnsureReleaseDc& operator= (HDC Dc)
      {
        Cleanup();

        m_Dc = Dc;

        return *this;
      }

      // Implicit conversion operator for BOOL
      operator HDC() const
      {
        return m_Dc;
      }

      // Cleanup the object if the value represents a valid object
      void Cleanup()
      {
        if (m_Wnd != nullptr && m_Dc != nullptr)
        {
          ReleaseDC(m_Wnd, m_Dc);

          m_Wnd = nullptr;
          m_Dc = nullptr;
        }
      }

    private:
      // Handles being managed
      HWND m_Wnd;
      HDC m_Dc;
    };
  }
}
