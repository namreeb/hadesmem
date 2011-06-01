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
#include <fstream>
#include <algorithm>

// Boost
#pragma warning(push, 1)
#include <boost/format.hpp>
#pragma warning(pop)

// RapidXML
#include <RapidXML/rapidxml.hpp>

// Hades
#pragma warning(push, 1)
#include "Game.h"
#include "Hades-Common/I18n.h"
#include "Hades-Common/Error.h"
#include "Hades-Memory/Injector.h"
#include "Hades-Common/Filesystem.h"
#pragma warning(pop)

namespace Hades
{
  namespace Loader
  {
    volatile WORD GameMgr::m_CurMsgId = WM_USER;

    void GameMgr::LoadConfig(std::wstring const& Path)
    {
      // Open config file
      std::wifstream ConfigFile(Path.c_str());
      if (!ConfigFile)
      {
        BOOST_THROW_EXCEPTION(HadesError() << 
          ErrorFunction("GameMgr::LoadConfig") << 
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
      auto const GamesTag = ConfigDoc.first_node(L"Games");
      if (!GamesTag)
      {
        BOOST_THROW_EXCEPTION(HadesError() << 
          ErrorFunction("GameMgr::LoadConfig") << 
          ErrorString("Invalid config file format."));
      }

      // Loop over all targets
      for (auto Game = GamesTag->first_node(L"Game"); Game; 
        Game = Game->next_sibling(L"Game"))
      {
        // Get target attributes
        auto const NameNode = Game->first_attribute(L"Name");
        auto const PathNode = Game->first_attribute(L"Path");
        auto const ArgsNode = Game->first_attribute(L"Args");
        auto const ModuleNode = Game->first_attribute(L"Module");
        auto const ExportNode = Game->first_attribute(L"Export");
        std::wstring const Name(NameNode ? NameNode->value() : L"");
        std::wstring const Path(PathNode ? PathNode->value() : L"");
        std::wstring const Args(ArgsNode ? ArgsNode->value() : L"");
        std::wstring const Module(ModuleNode ? ModuleNode->value() : L"");
        std::wstring const Export(ExportNode ? ExportNode->value() : L"");

        // Ensure data is valid
        if (Name.empty() || Path.empty() || Module.empty())
        {
          BOOST_THROW_EXCEPTION(HadesError() << 
            ErrorFunction("GameMgr::LoadConfig") << 
            ErrorString("Invalid game attributes."));
        }

        // Initialize menu data
        MenuData MyMenuData;
        MyMenuData.MessageId = ++m_CurMsgId;
        MyMenuData.Name = Name;
        MyMenuData.Path = Path;
        MyMenuData.Args = Args;
        MyMenuData.Module = Module;
        MyMenuData.Export = boost::lexical_cast<std::string>(Export);

        // Add current game
        m_GameMenuList.push_back(std::make_shared<MenuData>(MyMenuData));
      }
    }

    std::shared_ptr<GameMgr::MenuData> GameMgr::GetDataForMessageId(
      WORD MessageId) const
    {
      auto Iter = std::find_if(m_GameMenuList.begin(), m_GameMenuList.end(), 
        [&] (std::shared_ptr<MenuData> Current)
      {
        return Current->MessageId == MessageId;
      });

      return Iter != m_GameMenuList.end() ? *Iter : nullptr;
    }

    std::shared_ptr<GameMgr::MenuData> GameMgr::GetDataForName(
      std::wstring const& Name) const
    {
      auto Iter = std::find_if(m_GameMenuList.begin(), m_GameMenuList.end(), 
        [&] (std::shared_ptr<MenuData> Current)
      {
        return Current->Name == Name;
      });

      return Iter != m_GameMenuList.end() ? *Iter : nullptr;
    }

    boost::shared_ptr<Hades::Memory::MemoryMgr> GameMgr::LaunchGame(
      MenuData const& Data) const
    {
      // Launch target
      HMODULE ModuleBase = NULL;
      DWORD ExportRet = 0;
      auto MyMemory(Hades::Memory::CreateAndInject(Data.Path, Data.Args, 
        Data.Module, Data.Export, &ModuleBase, &ExportRet));

      // Debug output
      std::wcout << boost::wformat(L"GameMgr::LaunchGame: Path = \"%ls\", "
        L"Args = \"%ls\", Module = \"%ls\", Export = \"%ls\".") %Data.Path 
        %Data.Args %Data.Module %boost::lexical_cast<std::wstring>(
        Data.Export) << std::endl;
      std::wcout << boost::wformat(L"GameMgr::LaunchGame: Module Base = "
        L"%p, Export Ret = %u.") %ModuleBase %ExportRet << std::endl;

      // Return memory manager instance
      return MyMemory;
    }

    void GameMgr::LoadConfigDefault()
    {
      LoadConfig(Hades::Windows::GetSelfDirPath().file_string() + 
        L"/Config/Games.xml");
    }

    std::vector<std::shared_ptr<GameMgr::MenuData>> GameMgr::GetAllData() const
    {
      return m_GameMenuList;
    }
  }
}
