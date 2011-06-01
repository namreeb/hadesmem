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
#include <memory>

// Boost
#pragma warning(push, 1)
#include <boost/optional.hpp>
#include <boost/iterator/iterator_facade.hpp>
#pragma warning(pop)

// Windows API
#include <Windows.h>

// Hades
#include "PeFile.hpp"
#include "Section.hpp"
#include "NtHeaders.hpp"

namespace Hades
{
  namespace Memory
  {
    // Section iterator
    class SectionIter : public boost::iterator_facade<SectionIter, 
      boost::optional<Section>, boost::incrementable_traversal_tag>
    {
    public:
      // Constructor
      explicit SectionIter(PeFile const& MyPeFile) 
        : m_PeFile(MyPeFile), 
        m_CurrentNum(0), 
        m_Current()
      {
        NtHeaders const MyNtHeaders(m_PeFile);
        WORD NumberOfSections = MyNtHeaders.GetNumberOfSections();
        if (NumberOfSections)
        {
          m_Current = Section(m_PeFile, m_CurrentNum);
        }
      }

    private:
      // Allow Boost.Iterator access to internals
      friend class boost::iterator_core_access;

      // For Boost.Iterator
      void increment() 
      {
        NtHeaders const MyNtHeaders(m_PeFile);
        WORD const NumberOfSections = MyNtHeaders.GetNumberOfSections();

        if (++m_CurrentNum < NumberOfSections)
        {
          m_Current = Section(m_PeFile, m_CurrentNum);
        }
        else
        {
          m_Current = boost::optional<Section>();
        }
      }

      // For Boost.Iterator
      boost::optional<Section>& dereference() const
      {
        return m_Current;
      }

      // Memory instance
      PeFile m_PeFile;

      // Current section number
      WORD m_CurrentNum;

      // Current section
      mutable boost::optional<Section> m_Current;
    };
  }
}
