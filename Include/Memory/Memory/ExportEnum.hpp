/*
This file is part of HadesMem.
Copyright © 2010 RaptorFactor (aka Cypherjb, Cypher, Chazwazza). 
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
#include <memory>

// Boost
#ifdef _MSC_VER
#pragma warning(push, 1)
#endif // #ifdef _MSC_VER
#include <boost/optional.hpp>
#include <boost/iterator/iterator_facade.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif // #ifdef _MSC_VER

// Windows API
#include <Windows.h>

// Hades
#include "ExportDir.hpp"

namespace Hades
{
  namespace Memory
  {
    // Section iterator
    class ExportIter : public boost::iterator_facade<ExportIter, 
      boost::optional<Export>, boost::incrementable_traversal_tag>
    {
    public:
      // Constructor
      explicit ExportIter(PeFile const& MyPeFile)
        : m_PeFile(MyPeFile), 
        m_Current()
      {
        ExportDir const MyExportDir(m_PeFile);
        DWORD const NumberOfFunctions = MyExportDir.GetNumberOfFunctions();
        if (NumberOfFunctions)
        {
          m_Current = Export(m_PeFile, MyExportDir.GetOrdinalBase());
        }
      }

    private:
      // Allow Boost.Iterator access to internals
      friend class boost::iterator_core_access;

      // For Boost.Iterator
      void increment() 
      {
        ExportDir const MyExportDir(m_PeFile);
        DWORD const NumberOfFunctions = MyExportDir.GetNumberOfFunctions();
        DWORD const NextOrdinal = m_Current->GetOrdinal() + 1;
        if (NextOrdinal - MyExportDir.GetOrdinalBase() < NumberOfFunctions)
        {
          m_Current = Export(m_PeFile, NextOrdinal);
        }
        else
        {
          m_Current = boost::optional<Export>();
        }
      }

      // For Boost.Iterator
      boost::optional<Export>& dereference() const
      {
        return m_Current;
      }

      // Memory instance
      PeFile m_PeFile;

      // Current section
      mutable boost::optional<Export> m_Current;
    };
  }
}
