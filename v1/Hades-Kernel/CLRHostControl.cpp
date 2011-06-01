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

// Hades
#include "CLRHostControl.h"

namespace Hades
{
  namespace Kernel
  {
    HadesHostControl::HadesHostControl() 
      : m_RefCount(1), 
      m_pDefaultDomainDomainManager(0), 
      m_Domains()
    { }

    HadesHostControl::~HadesHostControl() 
    { }

    HRESULT STDMETHODCALLTYPE HadesHostControl::QueryInterface(const IID& iid, 
      void** ppv)
    {
      if (iid == IID_IUnknown || iid == IID_IHostControl)
      {
        *ppv = static_cast<IHostControl*>(this);
        static_cast<IUnknown*>(*ppv)->AddRef();
        return S_OK;
      }
      else
      {
        *ppv = NULL;
        return E_NOINTERFACE;
      }
    }

    ULONG STDMETHODCALLTYPE HadesHostControl::AddRef()
    {
      return InterlockedIncrement(&m_RefCount);
    }

    ULONG STDMETHODCALLTYPE HadesHostControl::Release() 
    {
      if (InterlockedDecrement(&m_RefCount) == 0)
      {
        delete this;
        return 0;
      }

      return m_RefCount;
    }

    HRESULT STDMETHODCALLTYPE HadesHostControl::GetHostManager(REFIID /*riid*/, 
      void **ppObject)
    {
      *ppObject = NULL;
      return E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE HadesHostControl::SetAppDomainManager(
      DWORD dwAppDomainID,
      IUnknown *pUnkAppDomainManager)

    {
      // Each time a domain gets created in the process, this method is
      // called. We keep a mapping of domainIDs to pointers to application
      // domain managers for each domain in the process. This map is handy for
      // enumerating the domains and so on.
      HadesAD::IHadesVM *pDomainManager = NULL;
      HRESULT hr = pUnkAppDomainManager->QueryInterface(
        __uuidof(HadesAD::IHadesVM), 
        reinterpret_cast<PVOID*>(&pDomainManager));

      m_Domains[dwAppDomainID] = pDomainManager;

      // Save the pointer to the default domain for convenience. We
      // initialize m_pDefaultDomainDomainManager to NULL in the
      // class's constructor. The first time this method is called
      // is for the default application domain.
      if (!m_pDefaultDomainDomainManager)
      {
        m_pDefaultDomainDomainManager = pDomainManager;
      }

      return hr;
    }

    HadesAD::IHadesVM* HadesHostControl::GetDomainManagerForDefaultDomain()
    {
      // AddRef the pointer before returning it.
      if (m_pDefaultDomainDomainManager) 
      {
        m_pDefaultDomainDomainManager->AddRef();
      }

      return m_pDefaultDomainDomainManager;
    }

    HadesHostControl::DomainMap& HadesHostControl::GetAllDomainManagers()
    {
      return m_Domains;
    }
  }
}
