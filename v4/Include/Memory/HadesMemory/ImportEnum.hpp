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
#include <memory>

// Boost
#pragma warning(push, 1)
#include <boost/optional.hpp>
#include <boost/iterator/iterator_facade.hpp>
#pragma warning(pop)

// Windows
#include <Windows.h>

// Hades
#include "Fwd.hpp"
#include "Error.hpp"
#include "PeFile.hpp"
#include "ImportDir.hpp"

namespace Hades
{
  namespace Memory
  {
    // Section iterator
    class ImportDirIter : public boost::iterator_facade<ImportDirIter, 
      boost::optional<ImportDir>, boost::incrementable_traversal_tag>
    {
    public:
      // Constructor
      explicit ImportDirIter(PeFile const& MyPeFile) 
        : m_PeFile(MyPeFile), 
        m_Current(m_PeFile)
      {
        if (m_Current->IsValid())
        {
          if (!m_Current->GetCharacteristics())
          {
            m_Current = boost::optional<ImportDir>();
          }
        }
      }

    private:
      // Allow Boost.Iterator access to internals
      friend class boost::iterator_core_access;

      // For Boost.Iterator
      void increment() 
      {
        if (m_Current->IsValid())
        {
          auto pImpDesc = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(
            m_Current->GetBase());
          m_Current = ImportDir(m_PeFile, ++pImpDesc);
          if (!m_Current->GetCharacteristics())
          {
            m_Current = boost::optional<ImportDir>();
          }
        }
        else
        {
          m_Current = boost::optional<ImportDir>();
        }
      }

      // For Boost.Iterator
      boost::optional<ImportDir>& dereference() const
      {
        return m_Current;
      }

      // Memory instance
      PeFile m_PeFile;

      // Current thunk pointer
      PIMAGE_IMPORT_DESCRIPTOR m_pImpDesc;

      // Current import dir
      mutable boost::optional<ImportDir> m_Current;
    };

    // Section iterator
    class ImportThunkIter : public boost::iterator_facade<ImportThunkIter, 
      boost::optional<ImportThunk>, boost::incrementable_traversal_tag>
    {
    public:
      // Constructor
      explicit ImportThunkIter(PeFile const& MyPeFile, DWORD FirstThunk) 
        : m_PeFile(MyPeFile), 
        m_pThunk(reinterpret_cast<PIMAGE_THUNK_DATA>(m_PeFile.RvaToVa(
          FirstThunk))), 
        m_Current(ImportThunk(m_PeFile, m_pThunk))
      {
        if (!m_Current->IsValid())
        {
          m_Current = boost::optional<ImportThunk>();
        }
      }

    private:
      // Allow Boost.Iterator access to internals
      friend class boost::iterator_core_access;

      // For Boost.Iterator
      void increment() 
      {
        m_Current = ImportThunk(m_PeFile, ++m_pThunk);
        if (!m_Current->IsValid())
        {
          m_Current = boost::optional<ImportThunk>();
        }
      }

      // For Boost.Iterator
      boost::optional<ImportThunk>& dereference() const
      {
        return m_Current;
      }

      // Memory instance
      PeFile m_PeFile;

      // Current thunk pointer
      PIMAGE_THUNK_DATA m_pThunk;

      // Current import thunk
      mutable boost::optional<ImportThunk> m_Current;
    };
  }
}
