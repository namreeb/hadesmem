#pragma once

// Windows API
#include <Windows.h>

// ATL/WTL
#include <atlbase.h>
#include <atlapp.h>

namespace Hades
{
  namespace GUISandbox
  {
    class GameHandler
    {
    public:
      virtual BOOL IsPaused() = 0;
      virtual HRESULT OnUpdateFrame() = 0;
    };

    class GameLoop : public CMessageLoop
    {
    public:
      GameLoop() 
        : m_pGameHandler(nullptr)
      { }

      GameHandler* GetGameHandler()
      {
        return m_pGameHandler;
      }

      void SetGameHandler(GameHandler* pGameHandler)
      {
        m_pGameHandler = pGameHandler;
      }

      virtual int Run()
      {
        bool IsActive = true;

        for (;;)
        {

          if (PeekMessage(&m_msg, NULL, 0, 0, PM_REMOVE))
          {
            if (WM_QUIT == m_msg.message)
            {
              break;
            }

            if (IsIdleMessage(&m_msg))
            {
              IsActive = true;
            }

            if(!PreTranslateMessage(&m_msg))
            {
              ::TranslateMessage(&m_msg);
              ::DispatchMessage(&m_msg);
            }
          }
          else if (IsActive)
          {
            OnIdle(0);

            IsActive = false;
          }
          else if (m_pGameHandler)
          {
            if (m_pGameHandler->IsPaused())
            {
              WaitMessage();
            }
            else
            {
              m_pGameHandler->OnUpdateFrame();
            }
          }
        }

        return static_cast<int>(m_msg.wParam);
      }

    private:
      GameHandler* m_pGameHandler;
    };
  }
}
