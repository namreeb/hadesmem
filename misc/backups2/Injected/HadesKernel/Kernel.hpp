/*
This file is part of HadesMem.
Copyright (C) 2011 Joshua Boyce (a.k.a. RaptorFactor).
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
#pragma warning(push, 1)
#pragma warning(disable: 4267)
#pragma warning(disable: 4503)
#include <boost/signals2.hpp>
#include <boost/filesystem.hpp>
#pragma warning(pop)

// Hades
#include "HadesRenderer/Renderer.hpp"
#include "HadesCommon/EnsureCleanup.hpp"

namespace Hades
{
  namespace Kernel
  {
    // Hades injected 'kernel'. Central manager of injected components.
    class Kernel
    {
    public:
      // Error type
      class Error : public virtual HadesError 
      { };
      
      // Load extension
      virtual void LoadExtension(boost::filesystem::path const& Path);
      
      // OnFrame notification function
      virtual void OnFrame(Hades::GUI::Renderer& pRenderer);
        
      // Subscribe to OnFrame event
      typedef boost::signals2::signal<void (Hades::GUI::Renderer& pRenderer)> 
        OnFrameSig;
      typedef OnFrameSig::slot_type OnFrameFn;
      virtual boost::signals2::connection RegisterOnFrame(OnFrameFn Fn);

    private:
      // Loaded extensions
      std::vector<Hades::Windows::EnsureFreeLibrary> m_Extensions;
        
      // OnFrame event subscribers
      OnFrameSig m_OnFrame;
    };
  }
}
