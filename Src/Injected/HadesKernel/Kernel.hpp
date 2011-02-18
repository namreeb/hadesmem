/*
This file is part of HadesMem.
Copyright (C) 2010 Joshua Boyce (aka RaptorFactor, Cypherjb, Cypher, Chazwazza).
<http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

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

// Boost
#ifdef _MSC_VER
#pragma warning(push, 1)
#pragma warning(disable: 4267)
#endif // #ifdef _MSC_VER
#include <boost/thread.hpp>
#include <boost/signals2.hpp>
#include <boost/filesystem.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif // #ifdef _MSC_VER

// Hades
#include "HadesRenderer/Renderer.hpp"
#include "HadesCommon/EnsureCleanup.hpp"

namespace Hades
{
  namespace Kernel
  {
    class Kernel
    {
    public:
      class Error : public virtual HadesError 
      { };
      
      virtual void LoadExtension(boost::filesystem::path const& Path);
      
      virtual void OnFrame(Hades::GUI::Renderer& pRenderer);
        
      typedef boost::signals2::signal<void (Hades::GUI::Renderer& pRenderer)> 
        OnFrameSig;
      typedef OnFrameSig::slot_type OnFrameFn;
      virtual boost::signals2::connection RegisterOnFrame(OnFrameFn Fn);

    private:
      std::vector<Hades::Windows::EnsureFreeLibrary> m_Extensions;
      OnFrameSig m_OnFrame;
    };
  }
}
