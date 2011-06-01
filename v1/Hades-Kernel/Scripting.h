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
#include <string>
#include <vector>

// Boost
#pragma warning(push, 1)
#include <boost/noncopyable.hpp>
#pragma warning(pop)

// Lua
extern "C"
{
#include "lua.h"
}

// LuaBind
#pragma warning(push, 1)
#include "LuaBind/luabind.hpp"
#include "luabind/tag_function.hpp"
#include "luabind/iterator_policy.hpp"
#pragma warning(pop)

// Hades
#include "Hades-Common/Error.h"

namespace Hades
{
  namespace Kernel
  {
    // Lua exception type
    class LuaError : public virtual HadesError 
    { };

    // LuaState wrapper class for RAII
    class LuaState : private boost::noncopyable
    {
    public:
      // Destructor
      ~LuaState();

      // Implicitly act as a lua_State pointer
      virtual operator lua_State*() const;

      // Implicitly act as a lua_State pointer
      virtual operator lua_State*();

    protected:
      // Constructor
      LuaState();

    private:
      // Only LuaMgr can create states
      friend class LuaMgr;

      // Underlying lua state
      lua_State* m_State;
    };

    // Lua managing class
    class LuaMgr : private boost::noncopyable
    {
    public:
      // Constructor
      LuaMgr();

      // Get LUA state
      virtual const LuaState& GetState() const;

      // Run a LUA script on disk
      virtual void RunFile(std::string const& Path) const;

      // Run a LUA script from a string
      virtual std::vector<std::string> RunString(std::string const& Script) const;

      // Reports an error to the console
      virtual void ReportError(int Status) const;

    private:
      // Lua state
      LuaState m_State;
    };
  }
}
