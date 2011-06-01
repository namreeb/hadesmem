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

// Hades
#include "Hades-Common/Error.h"

#define HADES_SCRIPTING_TRYCATCH_BEGIN \
  try\
{

#define HADE_SCRIPTING_TRYCATCH_END \
}\
  catch (boost::exception const& e)\
{\
  throw std::exception(boost::diagnostic_information(e).c_str());\
}

namespace Hades
{
  namespace Kernel
  {
    class Kernel;
    class DotNetMgr;

    namespace Wrappers
    {
      class WriteLn
      {
      public:
        explicit WriteLn(Kernel* pKernel);

        void operator()(std::string const& Input) const;

      private:
        Kernel* m_pKernel;
      };

      class LoadExt
      {
      public:
        explicit LoadExt(Kernel* pKernel);

        void operator()(std::string const& LoadExt) const;

      private:
        Kernel* m_pKernel;
      };

      class DotNet
      {
      public:
        explicit DotNet(DotNetMgr* pDotNet);

        void operator() (std::string const& Assembly, 
          std::string const& Parameters, 
          std::string const& Domain) const;

      private:
        DotNetMgr* m_pDotNet;
      };

      class Exit
      {
      public:
        void operator()() const;
      };

      class SessionId
      {
      public:
        SessionId(Kernel* pKernel);

        void operator() (unsigned int MySessionId) const;

        unsigned int operator()() const;

      private:
        Kernel* m_pKernel;
      };

      class SessionName
      {
      public:
        SessionName(Kernel* pKernel);

        std::string operator()() const;

      private:
        Kernel* m_pKernel;
      };

      class EnableWatermark
      {
      public:
        EnableWatermark(Kernel* pKernel);

        void operator()() const;

      private:
        Kernel* m_pKernel;
      };

      class DisableWatermark
      {
      public:
        DisableWatermark(Kernel* pKernel);

        void operator()() const;

      private:
        Kernel* m_pKernel;
      };
    }
  }
}
