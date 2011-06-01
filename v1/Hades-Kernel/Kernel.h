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
#include <string>
#include <memory>

// Boost
#include <boost/noncopyable.hpp>

// Hades
#include "Scripting.h"
#include "Hades-Memory/Memory.h"

namespace Hades
{
  namespace D3D9
  {
    class D3D9MgrWrapper;
    class GuiMgr;
  }

  namespace Input
  {
    class InputMgrWrapper;
  }

  namespace Kernel
  {
    class DotNetMgr;

    // Kernel exception type
    class KernelError : public virtual HadesError 
    { };

    // Hades kernel
    class Kernel : private boost::noncopyable
    {
    public:
      // Constructor
      Kernel();

      // Initialize kernel
      virtual void Initialize();

      // Get memory manager
      virtual std::shared_ptr<Memory::MemoryMgr> GetMemoryMgr();

      // Load and initialize a Hades helper module
      virtual void LoadModule(std::wstring const& Module);

      // Load and initialize a Hades extension
      virtual void LoadExtension(std::wstring const& Module);

      // Get D3D9 manager wrapper
      virtual D3D9::D3D9MgrWrapper* GetD3D9Mgr();

      // Set D3D9 manager wrapper
      virtual void SetD3D9Mgr(D3D9::D3D9MgrWrapper* pD3D9Mgr);

      // Get input manager wrapper
      virtual Input::InputMgrWrapper* GetInputMgr();

      // Set input manager wrapper
      virtual void SetInputMgr(Input::InputMgrWrapper* pD3D9Mgr);

      // Set GUI manager
      virtual void SetGuiMgr(D3D9::GuiMgr* pGuiMgr);

      // Get GUI manager
      virtual D3D9::GuiMgr* GetGuiMgr();

      // Get session ID
      virtual unsigned int GetSessionId();

      // Set session ID
      virtual void SetSessionId(unsigned int SessionId);

      // Run script
      virtual std::vector<std::string> RunScript(std::string const& Script, 
        bool EchoToConsole);

      // Run script file
      virtual void RunScriptFile(std::string const& Script);

      // Whether hook is enabled
      virtual bool IsHookEnabled(std::wstring const& Name);
      
      // Get session name
      virtual std::wstring GetSessionName();

      // Get lua manager
      virtual LuaMgr& GetLuaMgr();

      // Get static Kernel instance
      static Kernel* GetKernelInstance();

    private:
      // GUI manager OnConsoleInput callback
      void OnConsoleInput(std::string const& Input);

      // Load hook configuration data
      void LoadHookConfig(std::wstring const& Path);

      // Memory manager
      std::shared_ptr<Memory::MemoryMgr> m_Memory;

      // Path to self dir
      std::wstring const m_PathToSelfDir;

      // Input manager wrapper
      Input::InputMgrWrapper* m_pInputMgr;

      // D3D9 manager wrapper
      D3D9::D3D9MgrWrapper* m_pD3D9Mgr;

      // GUI manager
      D3D9::GuiMgr* m_pGuiMgr;

      // Lua manager
      LuaMgr m_LuaMgr;

      // DotNet manager
      std::shared_ptr<DotNetMgr> m_pDotNetMgr;

      // Session ID
      unsigned int m_SessionId;

      // Hook configuration data
      std::map<std::wstring, bool> m_HookConfig;

      // Static kernel instance
      static Kernel* m_pKernel;
    };
  }
}
