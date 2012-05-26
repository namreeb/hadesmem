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

// Boost
#include <boost/thread.hpp>

// Hades
#include "Kernel.hpp"
#include "HadesCommon/Filesystem.hpp"

namespace Hades
{
  namespace Kernel
  {
    // Load extension
    void Kernel::LoadExtension(boost::filesystem::path const& Path)
    {
      // If extension path is relative assume that it's relative to the 
      // directory that the kernel is located in
      boost::filesystem::path PathReal(Path.is_relative() ? 
        Hades::Windows::GetSelfDirPath() / Path : Path);
      
      // Load extension
      Windows::EnsureFreeLibrary ExtMod(LoadLibrary(PathReal.c_str()));
      if (!ExtMod)
      {
        std::error_code const LastError = GetLastErrorCode();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Kernel::EnsureFreeLibrary") << 
          ErrorString("Could not load extension.") << 
          ErrorCode(LastError));
      }
      
      // Call extension initialization routine
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
      typedef DWORD (__stdcall* tInitialize)(HMODULE Module, Kernel& MyKernel);
      auto pInitializeReal = reinterpret_cast<tInitialize>(pInitialize);
      pInitializeReal(ExtMod, *this);
      
      // Add loaded extension to list
      m_Extensions.push_back(std::move(ExtMod));
    }
    
    // OnFrame notification function
    void Kernel::OnFrame(Hades::GUI::Renderer& pRenderer)
    {
      // Function is not thread-safe
      static boost::mutex MyMutex;
      boost::lock_guard<boost::mutex> MyLock(MyMutex);
        
      // Draw watermark
      pRenderer.DrawText(L"Hades", 5, 5);
      
      // Notify subscribers
      m_OnFrame(pRenderer);
    }
    
    // Subscribe to OnFrame event
    boost::signals2::connection Kernel::RegisterOnFrame(OnFrameFn Fn)
    {
      // Subscribe to OnFrame event
      return m_OnFrame.connect(Fn);
    }
  }
}
