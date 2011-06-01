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

// Hades
#include "DotNet.h"
#include "Kernel.h"
#include "Hades-Common/I18n.h"

// C++ Standard Library
#include <fstream>
#include <iostream>
#include <exception>
#include <stdexcept>

// Boost C++ Libraries
#pragma warning(push, 1)
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/function.hpp>
#pragma warning(pop)

// RapidXML
#include <RapidXML/rapidxml.hpp>

// Hades namespace
namespace Hades
{
  namespace Kernel
  {
    // Run Lua script. Exported to allow access in .NET layer.
    extern "C" __declspec(dllexport) char const* __stdcall RunLuaScript(
      char const* pScript, unsigned int Index)
    {
      try
      {
        // Get kernel instance and run script
        auto Results(Kernel::GetKernelInstance()->RunScript(pScript, false));
        if (Results.empty() || Index >= Results.size())
        {
          return nullptr;
        }

        // Get specified result
        std::string const& Result(Results[Index]);

        // Allocate memory for result
        PSTR pTopResult = static_cast<PSTR>(HeapAlloc(
          GetProcessHeap(), 
          HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS, 
          Result.size() + 1));

        // Copy result to new memory
        std::copy(Result.begin(), Result.end(), pTopResult);

        // Return pointer to result
        return pTopResult;
      }
      catch (boost::exception const& e)
      {
        std::cout << "RunLuaScript: Error! " << 
          boost::diagnostic_information(e) << std::endl; 
      }
      catch (std::exception const& e)
      {
        std::cout << "RunLuaScript: Error! " << e.what() << std::endl; 
      }
      catch (...)
      {
        std::cout << "RunLuaScript: Unknown error!" << std::endl;
      }

      return nullptr;
    }

    // .NET frame event callback list
    std::vector<DotNetMgr::FrameCallback> DotNetMgr::m_FrameEvents;

    // Constructor
    DotNetMgr::DotNetMgr(Kernel* pKernel, std::wstring const& Config) 
      : m_pClrMetaHost(), 
      m_pClrRuntimeInfo(), 
      m_pClrRuntimeHost(), 
      m_pClrControl(), 
      m_pClrHostControl(), 
      m_pKernel(pKernel), 
      m_pDomainMgr(nullptr)
    {
      // Open config file
      std::wifstream ConfigFile(Config.c_str());
      if (!ConfigFile)
      {
        BOOST_THROW_EXCEPTION(DotNetMgrError() << 
          ErrorFunction("DotNetMgr::DotNetMgr") << 
          ErrorString("Could not open config file."));
      }

      // Copy file to buffer
      std::istreambuf_iterator<wchar_t> ConfigFileBeg(ConfigFile);
      std::istreambuf_iterator<wchar_t> ConfigFileEnd;
      std::vector<wchar_t> ConfigFileBuf(ConfigFileBeg, ConfigFileEnd);
      ConfigFileBuf.push_back(L'\0');

      // Open XML document
      rapidxml::xml_document<wchar_t> ConfigDoc;
      ConfigDoc.parse<0>(&ConfigFileBuf[0]);

      // Get runtime information
      auto RuntimeTag = ConfigDoc.first_node(L"Runtime");
      if (!RuntimeTag)
      {
        BOOST_THROW_EXCEPTION(DotNetMgrError() << 
          ErrorFunction("DotNetMgr::DotNetMgr") << 
          ErrorString("Invalid config file. Could not get runtime info."));
      }
      auto RuntimeVerNode = RuntimeTag->first_attribute(L"Version");
      std::wstring const RuntimeVer(RuntimeVerNode ? RuntimeVerNode->value() : 
        L"");
      if (RuntimeVer.empty())
      {
        BOOST_THROW_EXCEPTION(DotNetMgrError() << 
          ErrorFunction("DotNetMgr::DotNetMgr") << 
          ErrorString("Invalid config file. Unspecified runtime version."));
      }

      // Get domain manager information
      auto DomainMgrTag = ConfigDoc.first_node(L"DomainManager");
      if (!DomainMgrTag)
      {
        BOOST_THROW_EXCEPTION(DotNetMgrError() << 
          ErrorFunction("DotNetMgr::DotNetMgr") << 
          ErrorString("Invalid config file. Could not get domain manager "
            "info."));
      }
      auto DomainMgrAssemblyNode = DomainMgrTag->first_attribute(L"Assembly");
      std::wstring const DomainMgrAssembly(DomainMgrAssemblyNode ? 
        DomainMgrAssemblyNode->value() : L"");
      if (DomainMgrAssembly.empty())
      {
        BOOST_THROW_EXCEPTION(DotNetMgrError() << 
          ErrorFunction("DotNetMgr::DotNetMgr") << 
          ErrorString("Invalid config file. Unspecified domain manager "
            "assembly."));
      }
      auto DomainMgrTypeNode = DomainMgrTag->first_attribute(L"Type");
      std::wstring const DomainMgrType(DomainMgrTypeNode ? 
        DomainMgrTypeNode->value() : L"");
      if (DomainMgrType.empty())
      {
        BOOST_THROW_EXCEPTION(DotNetMgrError() << 
          ErrorFunction("DotNetMgr::DotNetMgr") << 
          ErrorString("Invalid config file. Unspecified domain manager "
            "type."));
      }

      // Create CLR meta host
      HRESULT ClrCreateResult = CLRCreateInstance(
        CLSID_CLRMetaHost,
        IID_ICLRMetaHost, 
        reinterpret_cast<LPVOID*>(&m_pClrMetaHost.p));
      if (FAILED(ClrCreateResult))
      {
        BOOST_THROW_EXCEPTION(DotNetMgrError() << 
          ErrorFunction("DotNetMgr::DotNetMgr") << 
          ErrorString("Could not create CLR Meta Host.") << 
          ErrorCodeWin(ClrCreateResult));
      }

      // Create CLR runtime info
      HRESULT GetRuntimeResult = m_pClrMetaHost->GetRuntime(
        RuntimeVer.c_str(), 
        IID_ICLRRuntimeInfo, 
        reinterpret_cast<LPVOID*>(&m_pClrRuntimeInfo.p));
      if (FAILED(GetRuntimeResult))
      {
        BOOST_THROW_EXCEPTION(DotNetMgrError() << 
          ErrorFunction("DotNetMgr::DotNetMgr") << 
          ErrorString("Could not get CLR Runtime Info.") << 
          ErrorCodeWin(GetRuntimeResult));
      }
      
      // Bind as legacy runtime
      HRESULT BindLegacyResult = m_pClrRuntimeInfo->BindAsLegacyV2Runtime();
      if (FAILED(BindLegacyResult))
      {
        BOOST_THROW_EXCEPTION(DotNetMgrError() << 
          ErrorFunction("DotNetMgr::DotNetMgr") << 
          ErrorString("Could not bind as legacy runtime.") << 
          ErrorCodeWin(BindLegacyResult));
      }

      // Create CLR runtime host
      HRESULT GetRuntimeHostResult = m_pClrRuntimeInfo->GetInterface(
        CLSID_CLRRuntimeHost, 
        IID_ICLRRuntimeHost, 
        reinterpret_cast<LPVOID*>(&m_pClrRuntimeHost.p));
      if (FAILED(GetRuntimeHostResult))
      {
        BOOST_THROW_EXCEPTION(DotNetMgrError() << 
          ErrorFunction("DotNetMgr::DotNetMgr") << 
          ErrorString("Could create CLR Runtime Host.") << 
          ErrorCodeWin(GetRuntimeHostResult));
      }

      // Create host control
      m_pClrHostControl.reset(new HadesHostControl());

      // Set host control
      HRESULT SetHostControlResult = m_pClrRuntimeHost->SetHostControl(
        static_cast<IHostControl*>(&*m_pClrHostControl));
      if (FAILED(SetHostControlResult))
      {
        BOOST_THROW_EXCEPTION(DotNetMgrError() << 
          ErrorFunction("DotNetMgr::DotNetMgr") << 
          ErrorString("Could set host control.") << 
          ErrorCodeWin(SetHostControlResult));
      }

      // Get CLR control interface
      HRESULT GetClrControlResult = m_pClrRuntimeHost->GetCLRControl(
        &m_pClrControl.p);
      if (FAILED(GetClrControlResult))
      {
        BOOST_THROW_EXCEPTION(DotNetMgrError() << 
          ErrorFunction("DotNetMgr::DotNetMgr") << 
          ErrorString("Could not get CLR control.") << 
          ErrorCodeWin(GetClrControlResult));
      }

      // Associate domain manager with CLR instance
      HRESULT SetAppDomainResult = m_pClrControl->SetAppDomainManagerType(
        DomainMgrAssembly.c_str(), 
        DomainMgrType.c_str());
      if (FAILED(SetAppDomainResult))
      {
        BOOST_THROW_EXCEPTION(DotNetMgrError() << 
          ErrorFunction("DotNetMgr::DotNetMgr") << 
          ErrorString("Could not set domain manager type.") << 
          ErrorCodeWin(SetAppDomainResult));
      }

      // Start CLR
      HRESULT StartClrResult = m_pClrRuntimeHost->Start();
      if (FAILED(StartClrResult))
      {
        BOOST_THROW_EXCEPTION(DotNetMgrError() << 
          ErrorFunction("DotNetMgr::DotNetMgr") << 
          ErrorString("Could not start CLR.") << 
          ErrorCodeWin(StartClrResult));
      }

      // Get domain manager
      m_pDomainMgr = m_pClrHostControl->GetDomainManagerForDefaultDomain();
      if (!m_pDomainMgr)
      {
        BOOST_THROW_EXCEPTION(DotNetMgrError() << 
          ErrorFunction("DotNetMgr::DotNetMgr") << 
          ErrorString("Could not get domain manager instance."));
      }

      // Call into the default application domain to attach the frame event
#pragma warning(push)
#pragma warning(disable: 4244)
      m_pDomainMgr->RegisterOnFrame(reinterpret_cast<DWORD_PTR>(
        &DotNetMgr::SubscribeFrameEvent));
#pragma warning(pop)

      // Register OnFrame event for callback system
      m_pKernel->GetD3D9Mgr()->RegisterOnFrame(std::bind(
        &DotNetMgr::OnFrameEvent, this, std::placeholders::_1, 
        std::placeholders::_2));

      // Debug output
      std::wcout << "DotNetMgr::DotNetMgr: Initialized." << std::endl;
    }

    // Destructor
    DotNetMgr::~DotNetMgr()
    {
      // Stop CLR
      HRESULT StopClrResult = m_pClrRuntimeHost->Stop();
      if (FAILED(StopClrResult))
      {
        std::wcout << boost::wformat(L"DotNetMgr::~DotNetMgr: Could not stop "
          L"CLR. Result = %p.") %StopClrResult << std::endl;
      }
    }

    // Load an assembly in the context of the current process
    void DotNetMgr::LoadAssembly(const std::wstring& Assembly, 
      const std::wstring& Parameters, 
      const std::wstring& Domain)
    {
      // Run assembly using domain manager
      m_pDomainMgr->RunAssembly(Domain.c_str(), Assembly.c_str(), 
        Parameters.c_str());
    }

    // Subscribe for OnFrame event
    void __stdcall DotNetMgr::SubscribeFrameEvent(FrameCallback Function)
    {
      // Add callback to list
      m_FrameEvents.push_back(Function);
    }

    // Hades OnFrame callback
    void DotNetMgr::OnFrameEvent(IDirect3DDevice9* /*pDevice*/, 
      D3D9::D3D9HelperPtr /*pHelper*/)
    {
      // Run all callbacks
      std::for_each(m_FrameEvents.begin(), m_FrameEvents.end(), 
        [] (FrameCallback Current) 
      {
        Current();
      });
    }
  }
}
