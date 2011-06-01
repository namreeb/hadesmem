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
#include <memory>
#include <string>
#include <vector>

// Windows API
#include <Windows.h>
#include <atlbase.h>
#include <MetaHost.h>

// DirectX API
#include <d3d9.h>

// Hades
#include "CLRHostControl.h"
#include "Hades-D3D9/D3D9Mgr.h"
#include "Hades-Common/Error.h"

// Hades namespace
namespace Hades
{
  namespace Kernel
  {
    // DotNetMgr exception type
    class DotNetMgrError : public virtual HadesError 
    { };

    // DotNet related code
    class DotNetMgr
    {
    public:
      // Constructor
      DotNetMgr(class Kernel* pKernel, std::wstring const& Config);

      // Destructor
      ~DotNetMgr();

      // Load an assembly in the context of the current process
      void LoadAssembly(const std::wstring& Assembly, 
        const std::wstring& Parameters, 
        const std::wstring& Domain);

    private:
      // .NET OnFrame callback type
      typedef void (__stdcall* FrameCallback)();

      // Subscribe for OnFrame event
      static void __stdcall SubscribeFrameEvent(FrameCallback Function);

      // Hades OnFrame callback
      void OnFrameEvent(IDirect3DDevice9* pDevice, 
        D3D9::D3D9HelperPtr pHelper);

      // CLR Meta Host
      CComPtr<ICLRMetaHost> m_pClrMetaHost;
      // CLR Runtime Info
      CComPtr<ICLRRuntimeInfo> m_pClrRuntimeInfo;
      // CLR Runtime Host
      CComPtr<ICLRRuntimeHost> m_pClrRuntimeHost;
      // CLR Control
      CComPtr<ICLRControl> m_pClrControl;
      // Hades CLR host control
      std::shared_ptr<class HadesHostControl> m_pClrHostControl;
      // Kernel instance
      class Kernel* m_pKernel;
      // Hades domain manager instance
      HadesAD::IHadesVM* m_pDomainMgr;

      // .NET frame event callback list
      static std::vector<FrameCallback> m_FrameEvents;
    };
  }
}
