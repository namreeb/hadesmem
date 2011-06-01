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
#include "Crosshair.h"

namespace Hades
{
  namespace HXGenHack
  {
    // Whether crosshair is enabled
    bool Crosshair::m_Enabled = false;

    // Initialize crosshair
    void Crosshair::Startup(Kernel::Kernel* pKernel)
    {
      // Register for OnFrame event
      pKernel->GetD3D9Mgr()->RegisterOnFrame(&Crosshair::OnFrame);

      // Expose API
      luabind::module(pKernel->GetLuaMgr().GetState(), "HXGenHack")
      [
        luabind::def("EnableCrosshair", &Crosshair::Enable)
        ,luabind::def("DisableCrosshair", &Crosshair::Disable)
      ];

      // Debug output
      std::wcout << "Crosshair::Startup: Crosshair initialized." << std::endl;
    }

    // Enable crosshair
    void Crosshair::Enable()
    {
      m_Enabled = true;
    }

    // Disable crosshair
    void Crosshair::Disable()
    {
      m_Enabled = false;
    }

    // OnFrame callback (for crosshair drawing)
    void Crosshair::OnFrame(IDirect3DDevice9* pDevice, 
      D3D9::D3D9HelperPtr pHelper)
    {
      try
      {
        // Only continue if crosshair enabled
        if (!m_Enabled)
        {
          return;
        }

        // Get viewport
        D3DVIEWPORT9 MyView;
        HRESULT ViewportError = pDevice->GetViewport(&MyView);
        if (FAILED(ViewportError))
        {
          BOOST_THROW_EXCEPTION(CrosshairError() << 
            ErrorFunction("Crosshair::OnFrame") << 
            ErrorString("Could not get viewport.") << 
            ErrorCodeWin(ViewportError));
        }

        // Get screen center
        float ScreenCenterX = static_cast<float>(MyView.Width / 2);
        float ScreenCenterY = static_cast<float>(MyView.Height / 2);

        // Draw crosshair
        pHelper->DrawLine(Math::Vec2f(ScreenCenterX - 10.f, ScreenCenterY), 
          Math::Vec2f(ScreenCenterX + 10.f, ScreenCenterY), 1.f, 
          D3DCOLOR_ARGB(255, 255, 0, 0));
        pHelper->DrawLine(Math::Vec2f(ScreenCenterX, ScreenCenterY - 10.f), 
          Math::Vec2f(ScreenCenterX, ScreenCenterY + 10.f), 1.f, 
          D3DCOLOR_ARGB(255, 255, 0, 0));
      }
      catch (boost::exception const& e)
      {
        // Debug output
        std::cout << boost::format("Crosshair::OnFrame: Error! %s.") 
          %boost::diagnostic_information(e) << std::endl;
      }
      catch (std::exception const& e)
      {
        // Debug output
        std::cout << boost::format("Crosshair::OnFrame: Error! %s.") 
          %e.what() << std::endl;
      }
    }
  }
}
