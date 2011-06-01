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
#include <vector>
#include <string>
#include <memory>

// Windows API
#include <Windows.h>

// Hades
#pragma warning(push, 1)
#include "Hades-Memory/Injector.h"
#pragma warning(pop)

namespace Hades
{
  namespace Loader
  {
    class GameMgr
    {
    public:
      void LoadConfig(std::wstring const& Path);

      void LoadConfigDefault();

      struct MenuData
      {
        WORD MessageId;
        std::wstring Name;
        std::wstring Path;
        std::wstring Args;
        std::wstring Module;
        std::string Export;
      };

      std::shared_ptr<MenuData> GetDataForMessageId(WORD MessageId) const;

      std::shared_ptr<MenuData> GetDataForName(std::wstring const& Name) const;

      boost::shared_ptr<Hades::Memory::MemoryMgr> LaunchGame(
        MenuData const& Data) const;

      std::vector<std::shared_ptr<MenuData>> GetAllData() const;

    private:
      std::vector<std::shared_ptr<MenuData>> m_GameMenuList;

      static volatile WORD m_CurMsgId;
    };
  }
}
