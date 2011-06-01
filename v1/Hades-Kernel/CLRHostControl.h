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

// Windows API
#include <windows.h>
#include <atlbase.h>
#include <mscoree.h>

// C++ Standard Library
#include <map>

// Hades
#import "Hades-AD/HadesAD.tlb"

// Hades namespace
namespace Hades
{
  namespace Kernel
  {
    // CLR host control class. Manages Hades-AD.
    class HadesHostControl : public IHostControl
    {
    public:	   
      // IHostControl
      HRESULT STDMETHODCALLTYPE GetHostManager(REFIID riid, void **ppObject);
      HRESULT STDMETHODCALLTYPE SetAppDomainManager(DWORD dwAppDomainID, 
        IUnknown *pUnkAppDomainManager);
      HRESULT STDMETHODCALLTYPE GetDomainNeutralAssemblies(
        ICLRAssemblyReferenceList **ppReferenceList);

      // IUnknown
      virtual HRESULT STDMETHODCALLTYPE QueryInterface(const IID &iid, 
        void **ppv);
      virtual ULONG STDMETHODCALLTYPE AddRef();
      virtual ULONG STDMETHODCALLTYPE Release();

      // Constructor
      HadesHostControl();

      // Destructor
      virtual ~HadesHostControl();

      // Get default domain domain manager
      HadesAD::IHadesVM *GetDomainManagerForDefaultDomain();

      // Typedef for map to hold all domain managers
      typedef std::map<int, HadesAD::IHadesVM*> DomainMap;

      // Get all domain managers
      DomainMap& GetAllDomainManagers();

    private:
      // Reference count
      long m_RefCount;
      // Default domain domain manager
      HadesAD::IHadesVM* m_pDefaultDomainDomainManager;
      // All domains
      DomainMap m_Domains;
    };
  }
}
