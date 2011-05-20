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

// Hades
#include <HadesMemory/Fwd.hpp>
#include <HadesMemory/Error.hpp>
#include <HadesMemory/MemoryMgr.hpp>
#include <HadesMemory/PeLib/PeFile.hpp>

// C++ Standard Library
#include <string>
#include <iterator>

// Boost
#include <boost/iterator.hpp>
#include <boost/optional.hpp>

// Windows
#include <Windows.h>

namespace Hades
{
  namespace Memory
  {
    // Import directory wrapper
    class ImportDir
    {
    public:
      // ImportDir error type
      class Error : public virtual HadesMemError
      { };

      // Constructor
      explicit ImportDir(PeFile const& MyPeFile, 
        PIMAGE_IMPORT_DESCRIPTOR pImpDesc = nullptr);

      // Whether import directory is valid
      bool IsValid() const;

      // Ensure import directory is valid
      void EnsureValid() const;

      // Get import directory base
      PBYTE GetBase() const;

      // Advance to next descriptor
      void Advance() const;

      // Get characteristics
      DWORD GetCharacteristics() const;

      // Set characteristics
      void SetCharacteristics(DWORD Characteristics) const;

      // Get time and date stamp
      DWORD GetTimeDateStamp() const;

      // Set time and date stamp
      void SetTimeDateStamp(DWORD TimeDateStamp) const;

      // Get forwarder chain
      DWORD GetForwarderChain() const;

      // Set forwarder chain
      void SetForwarderChain(DWORD ForwarderChain) const;

      // Get name (raw)
      DWORD GetNameRaw() const;

      // Set name (raw)
      void SetNameRaw(DWORD Name) const;

      // Get name
      std::string GetName() const;
        
      // Todo: SetName function

      // Get first thunk
      DWORD GetFirstThunk() const;

      // Set first thunk
      void SetFirstThunk(DWORD FirstThunk) const;

    private:
      PeFile m_PeFile;

      MemoryMgr m_Memory;

      mutable PIMAGE_IMPORT_DESCRIPTOR m_pImpDesc;
    };
    
    // ImportDir enumeration class
    class ImportDirList
    {
    public:
      // ImportDir list error class
      class Error : public virtual HadesMemError
      { };
        
      // ImportDir iterator
      class ImportDirIter : public boost::iterator_facade<ImportDirIter, 
        ImportDir, boost::forward_traversal_tag>
      {
      public:
        // ImportDir iterator error class
        class Error : public virtual HadesMemError
        { };

        // Constructor
        ImportDirIter() 
          : m_pParent(nullptr), 
          m_PeFile(), 
          m_ImportDir(), 
          m_Num(static_cast<DWORD>(-1))
        { }
        
        // Constructor
        ImportDirIter(ImportDirList& Parent) 
          : m_pParent(&Parent), 
          m_PeFile(Parent.m_PeFile), 
          m_ImportDir(*m_PeFile), 
          m_Num(0)
        {
          if (!m_ImportDir->IsValid() || !m_ImportDir->GetCharacteristics())
          {
            m_pParent = nullptr;
            m_PeFile = boost::optional<PeFile>();
            m_ImportDir = boost::optional<ImportDir>();
            m_Num = static_cast<DWORD>(-1);
          }
        }
        
        // Copy constructor
        ImportDirIter(ImportDirIter const& Rhs) 
          : m_pParent(Rhs.m_pParent), 
          m_PeFile(Rhs.m_PeFile), 
          m_ImportDir(Rhs.m_ImportDir), 
          m_Num(Rhs.m_Num)
        { }
        
        // Assignment operator
        ImportDirIter& operator=(ImportDirIter const& Rhs) 
        {
          m_pParent = Rhs.m_pParent;
          m_PeFile = Rhs.m_PeFile;
          m_ImportDir = Rhs.m_ImportDir;
          m_Num = Rhs.m_Num;
          return *this;
        }

      private:
        // Give Boost.Iterator access to internals
        friend class boost::iterator_core_access;

        // Increment iterator
        void increment() 
        {
          ++m_Num;
          auto pImpDesc = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(
            m_ImportDir->GetBase());
          m_ImportDir = ImportDir(*m_PeFile, ++pImpDesc);
          if (!m_ImportDir->GetCharacteristics())
          {
            m_pParent = nullptr;
            m_ImportDir = boost::optional<ImportDir>();
            m_PeFile = boost::optional<PeFile>();
            m_Num = static_cast<DWORD>(-1);
          }
        }
        
        // Check iterator for equality
        bool equal(ImportDirIter const& Rhs) const
        {
          return this->m_pParent == Rhs.m_pParent && this->m_Num == Rhs.m_Num;
        }
    
        // Dereference iterator
        ImportDir& dereference() const 
        {
          return *m_ImportDir;
        }

        // Parent
        class ImportDirList* m_pParent;
        // PE file
        boost::optional<PeFile> m_PeFile;
        // ImportDir object
        mutable boost::optional<ImportDir> m_ImportDir;
        // Import dir num
        DWORD m_Num;
      };
      
      // ImportDir list iterator types
      typedef ImportDirIter iterator;
      
      // Constructor
      ImportDirList(PeFile const& MyPeFile)
        : m_PeFile(MyPeFile)
      { }
      
      // Get start of importdir list
      iterator begin()
      {
        return iterator(*this);
      }
      
      // Get end of importdir list
      iterator end()
      {
        return iterator();
      }
      
    private:
      // Give iterator access to internals
      friend class ImportDirIter;
      
      // PE file
      PeFile m_PeFile;
    };

    // Import thunk wrapper
    class ImportThunk
    {
    public:
      // ImportThunk error type
      class Error : public virtual HadesMemError
      { };

      // Constructor
      ImportThunk(PeFile const& MyPeFile, PVOID pThunk);

      // Whether thunk is valid
      bool IsValid() const;

      // Ensure thunk is valid
      void EnsureValid() const;

      // Advance to next thunk
      void Advance() const;

      // Get address of data
      DWORD_PTR GetAddressOfData() const;

      // Set address of data
      void SetAddressOfData(DWORD_PTR AddressOfData) const;

      // Get ordinal (raw)
      DWORD_PTR GetOrdinalRaw() const;

      // Set ordinal (raw)
      void SetOrdinalRaw(DWORD_PTR OrdinalRaw) const;
      
      // Whether import is by ordinal
      bool ByOrdinal() const;

      // Get ordinal
      WORD GetOrdinal() const;
        
      // Todo: SetOrdinal function

      // Get function
      DWORD_PTR GetFunction() const;

      // Set function
      void SetFunction(DWORD_PTR Function) const;

      // Get hint
      WORD GetHint() const;

      // Set hint
      void SetHint(WORD Hint) const;

      // Get name
      std::string GetName() const;
        
      // Todo: SetName function
      
      // Get base
      PVOID GetBase() const;

    private:
      PeFile m_PeFile;

      MemoryMgr m_Memory;

      mutable PIMAGE_THUNK_DATA m_pThunk;

      mutable PBYTE m_pBase;
    };
    
    // ImportThunk enumeration class
    class ImportThunkList
    {
    public:
      // ImportThunk list error class
      class Error : public virtual HadesMemError
      { };
        
      // ImportThunk iterator
      class ImportThunkIter : public boost::iterator_facade<ImportThunkIter, 
        ImportThunk, boost::forward_traversal_tag>
      {
      public:
        // ImportThunk iterator error class
        class Error : public virtual HadesMemError
        { };

        // Constructor
        ImportThunkIter() 
          : m_pParent(nullptr), 
          m_PeFile(), 
          m_pThunk(nullptr), 
          m_ImportThunk()
        { }
        
        // Constructor
        ImportThunkIter(ImportThunkList& Parent, DWORD FirstThunk) 
          : m_pParent(&Parent), 
          m_PeFile(Parent.m_PeFile), 
          m_pThunk(nullptr), 
          m_ImportThunk()
        {
          m_pThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(m_PeFile->RvaToVa(
            FirstThunk));
          m_ImportThunk = ImportThunk(*m_PeFile, m_pThunk);
          if (!m_ImportThunk->IsValid())
          {
            m_pParent = nullptr;
            m_PeFile = boost::optional<PeFile>();
            m_pThunk = nullptr;
            m_ImportThunk = boost::optional<ImportThunk>();
          }
        }
        
        // Copy constructor
        ImportThunkIter(ImportThunkIter const& Rhs) 
          : m_pParent(Rhs.m_pParent), 
          m_PeFile(Rhs.m_PeFile), 
          m_pThunk(Rhs.m_pThunk), 
          m_ImportThunk(Rhs.m_ImportThunk)
        { }
        
        // Assignment operator
        ImportThunkIter& operator=(ImportThunkIter const& Rhs) 
        {
          m_pParent = Rhs.m_pParent;
          m_PeFile = Rhs.m_PeFile;
          m_pThunk = Rhs.m_pThunk;
          m_ImportThunk = Rhs.m_ImportThunk;
          return *this;
        }

      private:
        // Give Boost.Iterator access to internals
        friend class boost::iterator_core_access;

        // Increment iterator
        void increment() 
        {
          m_ImportThunk = ImportThunk(*m_PeFile, ++m_pThunk);
          if (!m_ImportThunk->IsValid())
          {
            m_pParent = nullptr;
            m_PeFile = boost::optional<PeFile>();
            m_pThunk = nullptr;
            m_ImportThunk = boost::optional<ImportThunk>();
          }
        }
        
        // Check iterator for equality
        bool equal(ImportThunkIter const& Rhs) const
        {
          return this->m_pParent == Rhs.m_pParent && 
            this->m_pThunk == Rhs.m_pThunk;
        }
    
        // Dereference iterator
        ImportThunk& dereference() const 
        {
          return *m_ImportThunk;
        }

        // Parent
        class ImportThunkList* m_pParent;
        // PE file
        boost::optional<PeFile> m_PeFile;
        // Current thunk pointer
        PIMAGE_THUNK_DATA m_pThunk;
        // ImportThunk object
        mutable boost::optional<ImportThunk> m_ImportThunk;
      };
      
      // ImportThunk list iterator types
      typedef ImportThunkIter iterator;
      
      // Constructor
      ImportThunkList(PeFile const& MyPeFile, DWORD FirstThunk)
        : m_PeFile(MyPeFile), 
        m_FirstThunk(FirstThunk)
      { }
      
      // Get start of importthunk list
      iterator begin()
      {
        return iterator(*this, m_FirstThunk);
      }
      
      // Get end of importthunk list
      iterator end()
      {
        return iterator();
      }
      
    private:
      // Give iterator access to internals
      friend class ImportThunkIter;
      
      // PE file
      PeFile m_PeFile;
      // First thunk
      DWORD m_FirstThunk;
    };
  }
}
