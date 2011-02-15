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

// Hades
#include "Kernel.hpp"
#include "HadesCommon/Filesystem.hpp"

namespace Hades
{
  namespace Kernel
  {
    void Kernel::LoadExtension(boost::filesystem::path const& Path)
    {
      boost::filesystem::path PathReal(Path);
        
      if (Path.is_relative())
      {
        PathReal = Hades::Windows::GetSelfDirPath() / Path;
      }
      
      Windows::EnsureFreeLibrary ExtMod(LoadLibrary(PathReal.c_str()));
      if (!ExtMod)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Kernel::EnsureFreeLibrary") << 
          ErrorString("Could not load extension.") << 
          ErrorCode(LastError));
      }
      
#if defined(_M_AMD64) 
      FARPROC pInitialize = GetProcAddress(ExtMod, "Initialize");
#elif defined(_M_IX86) 
      FARPROC pInitialize = GetProcAddress(ExtMod, "_Initialize@8");
#else 
#error "[HadesMem] Unsupported architecture."
#endif
      if (!pInitialize)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Kernel::EnsureFreeLibrary") << 
          ErrorString("Could not initialize extension.") << 
          ErrorCode(LastError));
      }
      
      typedef DWORD (__stdcall* tInitialize)(HMODULE Module, Kernel* pKernel);
      auto pInitializeReal = reinterpret_cast<tInitialize>(pInitialize);
      pInitializeReal(ExtMod, this);
      
      m_Extensions.push_back(std::move(ExtMod));
    }
    
    void Kernel::OnFrame(Hades::GUI::Renderer& pRenderer)
    {
      pRenderer.DrawText(L"Test", 0, 0);
    }
  }
}
