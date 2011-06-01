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
#include <array>
#include <iostream>

// Boost
#include <boost/bind.hpp>
#include <boost/format.hpp>

// Windows API
#include <Windows.h>

// RapidXML
#include <RapidXML/rapidxml.hpp>

// Hades
#include "Kernel.h"
#include "Loader.h"
#include "DotNet.h"
#include "Wrappers.h"
#include "Scripting.h"
#include "Hades-D3D9/GuiMgr.h"
#include "Hades-Common/I18n.h"
#include "Hades-D3D9/D3D9Mgr.h"
#include "Hades-Input/InputMgr.h"
#include "Hades-Common/Filesystem.h"

namespace Hades
{
  namespace Kernel
  {
    // Static kernel instance
    Kernel* Kernel::m_pKernel = nullptr;

    // Constructor
    Kernel::Kernel() 
      : m_Memory(new Hades::Memory::MemoryMgr(GetCurrentProcessId())), 
      m_PathToSelfDir(Hades::Windows::GetSelfDirPath().file_string()), 
      m_pInputMgr(nullptr), 
      m_pD3D9Mgr(nullptr), 
      m_pGuiMgr(nullptr), 
      m_LuaMgr(), 
      m_pDotNetMgr(nullptr), 
      m_SessionId(0), 
      m_HookConfig()
    { }

    // Initialize kernel
    void Kernel::Initialize()
    {
      // Initialize static kernel instance
      m_pKernel = this;

      // Get string to binary we're injected into
      DWORD const BinPathSize = MAX_PATH;
      std::wstring BinPath;
      if (!GetModuleFileName(nullptr, Util::MakeStringBuffer(BinPath, 
        BinPathSize), BinPathSize))
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(KernelError() << 
          ErrorFunction("Kernel::Initialize") << 
          ErrorString("Could not get path to current binary.") << 
          ErrorCodeWin(LastError));
      }

      // Debug output
      std::wcout << boost::wformat(L"Kernel::Initialize: Path to current "
        L"binary = \"%ls\".") %BinPath << std::endl;

      // Path to self
      auto const PathToSelf(Hades::Windows::GetSelfPath().file_string());

      // Debug output
      std::wcout << boost::wformat(L"Kernel::Initialize: Path to Self "
        L"(Full): = \"%ls\", Path To Self (Dir): = \"%ls\".") %PathToSelf 
        %m_PathToSelfDir << std::endl;

      // Initialize hook config data
      LoadHookConfig(m_PathToSelfDir + L"/Config/Hook.xml");

      // Initialize Loader
      Loader::Initialize(this);
      Loader::LoadConfig(m_PathToSelfDir + L"/Config/Loader.xml");

      // Start aux modules
#if defined(_M_X64)
      std::wstring const D3D9ModName(L"Hades-D3D9_AMD64.dll");
      std::wstring const InputModName(L"Hades-Input_AMD64.dll");
#elif defined(_M_IX86)
      std::wstring const D3D9ModName(L"Hades-D3D9_IA32.dll");
      std::wstring const InputModName(L"Hades-Input_IA32.dll");
#else
#error Unsupported platform!
#endif
      LoadModule(m_PathToSelfDir + L"\\" + InputModName);
      LoadModule(m_PathToSelfDir + L"\\" + D3D9ModName);

      // Initialize .NET
      m_pDotNetMgr.reset(new DotNetMgr(this, m_PathToSelfDir + 
        L"/Config/DotNet.xml"));

      // Expose Hades API
      luabind::module(m_LuaMgr.GetState(), "Hades")
      [
        luabind::def("WriteLn", luabind::tag_function<void (
          std::string const&)>(Wrappers::WriteLn(this)))
        ,luabind::def("LoadExt", luabind::tag_function<void (
          std::string const&)>(Wrappers::LoadExt(this)))
        ,luabind::def("DotNet", luabind::tag_function<void (std::string const&, 
          std::string const&, std::string const&)>(Wrappers::DotNet(
          &*m_pDotNetMgr)))
        ,luabind::def("Exit", luabind::tag_function<void ()>(Wrappers::Exit()))
        ,luabind::def("GetSessionId", luabind::tag_function<unsigned int ()>(
          Wrappers::SessionId(this)))
        ,luabind::def("SetSessionId", luabind::tag_function<void 
          (unsigned int)>(Wrappers::SessionId(this)))
        ,luabind::def("GetSessionName", luabind::tag_function<std::string ()>(
          Wrappers::SessionName(this)))
        ,luabind::def("EnableWatermark", luabind::tag_function<void ()>(
          Wrappers::EnableWatermark(this)))
        ,luabind::def("DisableWatermark", luabind::tag_function<void ()>(
          Wrappers::DisableWatermark(this)))
      ];

      // Debug output
      std::wcout << "Kernel::Initialize: Hades-Kernel initialized." 
        << std::endl;
    }

    // Get memory manager
    std::shared_ptr<Hades::Memory::MemoryMgr> Kernel::GetMemoryMgr() 
    {
      return m_Memory;
    }

    // Load and initialize a Hades helper module
    void Kernel::LoadModule(std::wstring const& Module) 
    {
      // Load module
      HMODULE const MyModule = LoadLibrary(Module.c_str());
      if (!MyModule)
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(KernelError() << 
          ErrorFunction("Kernel::LoadModule") << 
          ErrorString("Could not find module \"" + 
          boost::lexical_cast<std::string>(Module) + "\".") << 
          ErrorCodeWin(LastError));
      }

      // Get address of Initialize export. Should be exported by all Hades 
      // extensions and modules.
      typedef void (__stdcall* tInitialize)(Kernel* pKernel);
      auto const pInitialize = reinterpret_cast<tInitialize>(GetProcAddress(
        MyModule, "_Initialize@4"));
      if (!pInitialize)
      {
        DWORD LastError = GetLastError();
        BOOST_THROW_EXCEPTION(KernelError() << 
          ErrorFunction("Kernel::LoadModule") << 
          ErrorString("Could not find '_Initialize@4' in module \"" + 
          boost::lexical_cast<std::string>(Module) + "\".") << 
          ErrorCodeWin(LastError));
      }

      // Call initialization routine
      pInitialize(this);
    }

    // Load and initialize a Hades extension
    void Kernel::LoadExtension(std::wstring const& Module)
    {
      // Load module from extension directory
      LoadModule(m_PathToSelfDir + L"/Extensions/" + Module);
    }

    // Get D3D9 manager wrapper
    D3D9::D3D9MgrWrapper* Kernel::GetD3D9Mgr()
    {
      return m_pD3D9Mgr;
    }

    // Set D3D9 manager wrapper
    void Kernel::SetD3D9Mgr(D3D9::D3D9MgrWrapper* pD3D9Mgr)
    {
      // Sanity check
      if (m_pD3D9Mgr)
      {
        std::wcout << "Kernel::SetD3D9Mgr: Warning! Attempt to overwrite "
          "existing D3D9Mgr instance." << std::endl;
        return;
      }

      // Set D3D9 manager
      m_pD3D9Mgr = pD3D9Mgr;
    }

    // Get input manager wrapper
    Input::InputMgrWrapper* Kernel::GetInputMgr()
    {
      return m_pInputMgr;
    }

    // Set input manager wrapper
    void Kernel::SetInputMgr(Input::InputMgrWrapper* pInputMgr)
    {
      // Sanity check
      if (m_pInputMgr)
      {
        std::wcout << "Kernel::SetInputMgr: Warning! Attempt to overwrite "
          "existing InputMgr instance." << std::endl;
        return;
      }

      // Set input manager
      m_pInputMgr = pInputMgr;
    }

    // Set GUI manager
    void Kernel::SetGuiMgr(D3D9::GuiMgr* pGuiMgr)
    {
      // Sanity check
      if (m_pGuiMgr)
      {
        std::wcout << "Kernel::SetGuiMgr: Warning! Attempt to overwrite "
          "existing GuiMgr instance." << std::endl;
        return;
      }

      // Set GUI manager
      m_pGuiMgr = pGuiMgr;
      m_pGuiMgr->RegisterOnConsoleInput(std::bind(&Kernel::OnConsoleInput, 
        this, std::placeholders::_1));
    }

    // Get GUI manager
    D3D9::GuiMgr* Kernel::GetGuiMgr()
    {
      return m_pGuiMgr;
    }

    // GUI manager OnConsoleInput callback
    void Kernel::OnConsoleInput(std::string const& Input)
    {
      // Run lua
      RunScript(Input, true);
    }

    // Get session ID
    unsigned int Kernel::GetSessionId()
    {
      return m_SessionId;
    }

    // Set session ID
    void Kernel::SetSessionId(unsigned int SessionId)
    {
      // Sanity check
      if (m_SessionId)
      {
        std::wcout << "Kernel::SetSessionId: Warning! Attempt to overwrite "
          "an existing session ID." << std::endl;
        return;
      }

      // Debug output
      std::wcout << "Kernel::SetSessionId: Assigning session ID '" << 
        SessionId << "'." << std::endl;
      
      // Set session ID
      m_SessionId = SessionId;
    }

    // Run script
    std::vector<std::string> Kernel::RunScript(std::string const& Script, 
      bool EchoToConsole)
    {
      // Debug output
      std::cout << "Kernel::RunScript: Script = \"" << Script << "\"." << 
        std::endl;

      try
      {
        // Run lua
        auto const Results(m_LuaMgr.RunString(Script));

        // Print results
        std::for_each(Results.begin(), Results.end(), 
          [this, EchoToConsole] (std::string const& Current)
        {
          if (EchoToConsole)
          {
            m_pGuiMgr->Print(Current);
          }

          std::cout << "Kernel::RunScript: Current Result = \"" << Current 
            << "\"." << std::endl;
        });

        // Return results
        return Results;
      }
      catch (boost::exception const& e)
      {
        // Print error information
        if (EchoToConsole && m_pGuiMgr)
        {
          m_pGuiMgr->Print(boost::diagnostic_information(e));
        }
        else
        {
          std::cout << "Kernel::RunScript: Error! " << 
            boost::diagnostic_information(e) << std::endl;
        }
      }
      catch (std::exception const& e)
      {
        // Print error information
        if (EchoToConsole && m_pGuiMgr)
        {
          m_pGuiMgr->Print(e.what());
        }
        else
        {
          std::cout << "Kernel::RunScript: Error! " << e.what() << std::endl;
        }
      }

      // No results
      return std::vector<std::string>();
    }

    // Run script file
    void Kernel::RunScriptFile(std::string const& Script)
    {
      // Debug output
      std::cout << "Kernel::RunScriptFile: \"" << Script << "\"." << std::endl;

      try
      {
        // Run lua
        m_LuaMgr.RunFile(Script);
      }
      catch (boost::exception const& e)
      {
        // Print error information
        if (m_pGuiMgr)
        {
          m_pGuiMgr->Print(boost::diagnostic_information(e));
        }
        else
        {
          std::cout << "Kernel::RunScriptFile: Error! " << 
            boost::diagnostic_information(e) << std::endl;
        }
      }
      catch (std::exception const& e)
      {
        // Print error information
        if (m_pGuiMgr)
        {
          m_pGuiMgr->Print(e.what());
        }
        else
        {
          std::cout << "Kernel::RunScriptFile: Error! " << e.what() << 
            std::endl;
        }
      }
    }

    // Load hook configuration data
    void Kernel::LoadHookConfig(std::wstring const& Path)
    {
      // Open config file
      std::wifstream ConfigFile(Path.c_str());
      if (!ConfigFile)
      {
        BOOST_THROW_EXCEPTION(KernelError() << 
          ErrorFunction("Kernel::LoadHookConfig") << 
          ErrorString("Could not open config file."));
      }

      // Copy file to buffer
      std::istreambuf_iterator<wchar_t> const ConfFileBeg(ConfigFile);
      std::istreambuf_iterator<wchar_t> const ConfFileEnd;
      std::vector<wchar_t> ConfFileBuf(ConfFileBeg, ConfFileEnd);
      ConfFileBuf.push_back(L'\0');

      // Open XML document
      rapidxml::xml_document<wchar_t> ConfigDoc;
      ConfigDoc.parse<0>(&ConfFileBuf[0]);

      // Ensure loader tag is found
      auto const HooksTag = ConfigDoc.first_node(L"Hooks");
      if (!HooksTag)
      {
        BOOST_THROW_EXCEPTION(KernelError() << 
          ErrorFunction("Kernel::LoadHookConfig") << 
          ErrorString("Invalid config file format."));
      }

      // Loop over all hooks
      for (auto Pattern = HooksTag->first_node(L"Hook"); Pattern; 
        Pattern = Pattern->next_sibling(L"Hook"))
      {
        // Get hook attributes
        auto const NameNode = Pattern->first_attribute(L"Name");
        auto const EnabledNode = Pattern->first_attribute(L"Enabled");
        std::wstring const Name(NameNode ? NameNode->value() : L"");
        std::wstring const Enabled(EnabledNode ? EnabledNode->value() : L"");

        // Ensure data is valid
        if (Name.empty() || Enabled.empty())
        {
          BOOST_THROW_EXCEPTION(KernelError() << 
            ErrorFunction("Kernel::LoadHookConfig") << 
            ErrorString("Invalid hook attributes."));
        }

        // Convert enabled string to bool
        bool EnabledReal = false;
        try
        {
          auto EnabledTemp = boost::lexical_cast<unsigned int>(Enabled);
          EnabledReal = EnabledTemp != 0;
        }
        catch (boost::bad_lexical_cast const& /*e*/)
        {
          BOOST_THROW_EXCEPTION(KernelError() << 
            ErrorFunction("Kernel::LoadHookConfig") << 
            ErrorString("Invalid hook attributes."));
        }
        
        // Debug output
        std::wcout << boost::wformat(L"Kernel::LoadHookConfig: Name = "
          L"\"%ls\", Enabled = %ls.") %Name %EnabledReal << std::endl;

        // Add current hook data
        m_HookConfig[Name] = EnabledReal;
      }
    }

    // Whether hook is enabled
    bool Kernel::IsHookEnabled(std::wstring const& Name)
    {
      // Get hook data
      auto const Iter = m_HookConfig.find(Name);
      if (Iter == m_HookConfig.end())
      {
        BOOST_THROW_EXCEPTION(KernelError() << 
          ErrorFunction("Kernel::IsHookEnabled") << 
          ErrorString("Invalid hook name."));
      }
      return Iter->second;
    }

    // Get static Kernel instance
    Kernel* Kernel::GetKernelInstance()
    {
      return m_pKernel;
    }

    // Get session name
    std::wstring Kernel::GetSessionName()
    {
      return L"Hades [Session " + boost::lexical_cast<std::wstring>(
        GetSessionId()) + L"]";
    }

    // Get lua manager
    LuaMgr& Kernel::GetLuaMgr()
    {
      return m_LuaMgr;
    }
  }
}
