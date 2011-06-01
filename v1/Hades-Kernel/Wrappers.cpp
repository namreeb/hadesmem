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
#include "Kernel.h"
#include "DotNet.h"
#include "Wrappers.h"
#include "Hades-D3D9/GuiMgr.h"
#include "Hades-Common/I18n.h"

namespace Hades
{
  namespace Kernel
  {
    namespace Wrappers
    {
      WriteLn::WriteLn(Kernel* pKernel)
        : m_pKernel(pKernel)
      { }

      void WriteLn::operator()(std::string const& Input) const
      {
        HADES_SCRIPTING_TRYCATCH_BEGIN
          m_pKernel->GetGuiMgr()->Print(Input);
        HADE_SCRIPTING_TRYCATCH_END
      }

      LoadExt::LoadExt(Kernel* pKernel)
        : m_pKernel(pKernel)
      { }

      void LoadExt::operator()(std::string const& LoadExt) const
      {
        HADES_SCRIPTING_TRYCATCH_BEGIN
          m_pKernel->LoadExtension(boost::lexical_cast<std::wstring>(LoadExt));
        HADE_SCRIPTING_TRYCATCH_END
      }

      DotNet::DotNet(DotNetMgr* pDotNet)
        : m_pDotNet(pDotNet)
      { }

      void DotNet::operator()(std::string const& Assembly, 
        std::string const& Parameters, std::string const& Domain) const
      {
        HADES_SCRIPTING_TRYCATCH_BEGIN
          m_pDotNet->LoadAssembly(
          boost::lexical_cast<std::wstring>(Assembly), 
          boost::lexical_cast<std::wstring>(Parameters), 
          boost::lexical_cast<std::wstring>(Domain));
        HADE_SCRIPTING_TRYCATCH_END
      }

      void Exit::operator()() const
      {
        ExitProcess(0);
      }

      SessionId::SessionId(Kernel* pKernel)
        : m_pKernel(pKernel)
      { }

      void SessionId::operator()(unsigned int MySessionId) const
      {
        m_pKernel->SetSessionId(MySessionId);
      }

      unsigned int SessionId::operator()() const
      {
        return m_pKernel->GetSessionId();
      }

      SessionName::SessionName(Kernel* pKernel)
        : m_pKernel(pKernel)
      { }

      std::string SessionName::operator()() const
      {
        return boost::lexical_cast<std::string>(m_pKernel->GetSessionName());
      }

      EnableWatermark::EnableWatermark(Kernel* pKernel) 
        : m_pKernel(pKernel)
      { }

      void EnableWatermark::operator()() const
      {
        m_pKernel->GetGuiMgr()->EnableWatermark();
      }

      DisableWatermark::DisableWatermark(Kernel* pKernel) 
        : m_pKernel(pKernel)
      { }

      void DisableWatermark::operator()() const
      {
        m_pKernel->GetGuiMgr()->DisableWatermark();
      }
    }
  }
}
