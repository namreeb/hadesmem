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
#include "Scripting.h"

namespace Hades
{
  namespace Kernel
  {
    // Destructor
    LuaState::~LuaState()
    {
      // Close LUA
      lua_close(m_State);
    }

    // Implicitly act as a lua_State pointer
    LuaState::operator lua_State*() const
    {
      // Return underlying lua state
      return m_State;
    }

    // Implicitly act as a lua_State pointer
    LuaState::operator lua_State*()
    {
      // Return underlying lua state
      return m_State;
    }

    // Constructor
    LuaState::LuaState() 
      : m_State(lua_open()) // Open LUA
    { }

    // Constructor
    LuaMgr::LuaMgr() 
      : m_State()
    {
      // Open LuaBind with Lua state
      luabind::open(m_State);
    }

    // Get LUA state
    const LuaState& LuaMgr::GetState() const
    {
      return m_State;
    }

    // Run a LUA script on disk
    void LuaMgr::RunFile(std::string const& Path) const
    {
      // Load and execute file
      int Status = luaL_dofile(m_State, Path.c_str());
      // Clean up if an error occurred
      if (Status != 0) 
      {
        lua_gc(m_State, LUA_GCCOLLECT, 0);
      }
      // Report any errors
      ReportError(Status);
    }

    // Run a LUA script from a string
    std::vector<std::string> LuaMgr::RunString(std::string const& Script) const
    {
      // Load string
      int Status = luaL_loadstring(m_State, Script.c_str());

      // Clean up if an error occurred
      if (Status != 0) 
      {
        lua_gc(m_State, LUA_GCCOLLECT, 0);
      }

      // Report any errors
      ReportError(Status);

      // Execute string
      Status = lua_pcall(m_State, 0, LUA_MULTRET, 0);

      // Clean up if an error occurred
      if (Status != 0) 
      {
        lua_gc(m_State, LUA_GCCOLLECT, 0);
      }

      // Report any errors
      ReportError(Status);

      // Get number of results
      int NumResults = lua_gettop(m_State);

      // Debug output
      char const* TopResult = lua_tostring(m_State, lua_gettop(m_State));
      std::wcout << "LuaMgr::RunString: NumResults = " << NumResults << 
        ", TopResult = " << (TopResult ? TopResult : "<Unknown>") << "." << 
        std::endl;

      // Result list
      std::vector<std::string> Results;
      Results.reserve(NumResults);

      // Loop over all results
      for (int i = 1; i <= NumResults; ++i)
      {
        // Convert result to string
        char const* Result(lua_tostring(m_State, i));

        // Boolean 'false' returns a null pointer and must be converted
        // manually.
        if (!Result  || !Result[0])
          Result = "nil";

        // Add result to list
        Results.push_back(Result);
      }

      // Pop results
      lua_pop(m_State, NumResults);

      // Clear stack
      lua_settop(m_State, 0);

      // Return result list
      return Results;
    }

    // Reports an error to the console
    void LuaMgr::ReportError(int Status) const
    {
      // Check if an error occurred
      if (Status && !lua_isnil(m_State, -1)) 
      {
        // Get error message as string
        const char* Message = lua_tostring(m_State, -1);
        // If a conversion to string is not possible set that as the message
        if (Message == NULL) 
        {
          Message = "Error object is not a string";
        }
        // Pop error message off stack
        lua_pop(m_State, 1);
        // Throw exception for error
        BOOST_THROW_EXCEPTION(LuaError() << 
          ErrorFunction("LuaMgr::ReportError") << 
          ErrorString(Message));
      }
    }
  }
}
