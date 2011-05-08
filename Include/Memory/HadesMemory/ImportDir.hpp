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
#include <string>
#include <iterator>

// Boost
#include <boost/optional.hpp>

// Windows
#include <Windows.h>

// Hades
#include "Fwd.hpp"
#include "Error.hpp"
#include "PeFile.hpp"
#include "MemoryMgr.hpp"

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
      template <typename ImportDirT>
      class ImportDirIter : public std::iterator<std::input_iterator_tag, 
        ImportDirT>
      {
      public:
        // ImportDir iterator error class
        class Error : public virtual HadesMemError
        { };

        // Constructor
        ImportDirIter() 
          : m_PeFile(), 
          m_ImportDir(), 
          m_Num(static_cast<DWORD>(-1))
        { }
        
        // Constructor
        ImportDirIter(PeFile const& MyPeFile) 
          : m_PeFile(MyPeFile), 
          m_ImportDir(*m_PeFile), 
          m_Num(0)
        {
          if (!m_ImportDir->IsValid() || !m_ImportDir->GetCharacteristics())
          {
            m_PeFile = boost::optional<PeFile>();
            m_ImportDir = boost::optional<ImportDir>();
            m_Num = static_cast<DWORD>(-1);
          }
        }
        
        // Copy constructor
        template <typename OtherT>
        ImportDirIter(ImportDirIter<OtherT> const& Rhs) 
          : m_PeFile(Rhs.m_PeFile), 
          m_ImportDir(Rhs.m_ImportDir), 
          m_Num(Rhs.m_Num)
        { }
        
        // Assignment operator
        template <typename OtherT>
        ImportDirIter& operator=(ImportDirIter<OtherT> const& Rhs) 
        {
          m_PeFile = Rhs.m_PeFile;
          m_ImportDir = Rhs.m_ImportDir;
          m_Num = Rhs.m_Num;
        }
        
        // Prefix increment
        ImportDirIter& operator++()
        {
          ++m_Num;
          auto pImpDesc = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(
            m_ImportDir->GetBase());
          m_ImportDir = ImportDir(*m_PeFile, ++pImpDesc);
          if (!m_ImportDir->GetCharacteristics())
          {
            m_ImportDir = boost::optional<ImportDir>();
            m_PeFile = boost::optional<PeFile>();
            m_Num = static_cast<DWORD>(-1);
          }
          
          return *this;
        }
        
        // Postfix increment
        ImportDirIter operator++(int)
        {
          ImportDirIter Temp(*this);
          ++*this;
          return Temp;
        }
        
        // Dereference operator
        reference operator*()
        {
          return *m_ImportDir;
        }
        
        // Dereference operator
        pointer operator->()
        {
          return &*m_ImportDir;
        }
        
        // Equality operator
        template<typename T>
        friend bool operator==(const ImportDirIter<T>& Rhs, 
          const ImportDirIter<T>& Lhs);
        
        // Inequality operator
        template<typename T>
        friend bool operator!=(const ImportDirIter<T>& Rhs, 
          const ImportDirIter<T>& Lhs);

      private:
        // PE file
        boost::optional<PeFile> m_PeFile;
        // ImportDir object
        boost::optional<ImportDir> m_ImportDir;
        // Import dir num
        DWORD m_Num;
      };
      
      // ImportDir list iterator types
      typedef ImportDirIter<const ImportDir> const_iterator;
      typedef ImportDirIter<ImportDir> iterator;
      
      // Constructor
      ImportDirList(PeFile const& MyPeFile)
        : m_PeFile(MyPeFile)
      { }
      
      // Get start of importdir list
      iterator begin()
      {
        return iterator(m_PeFile);
      }
      
      // Get end of importdir list
      iterator end()
      {
        return iterator();
      }
      
    private:
      // PE file
      PeFile m_PeFile;
    };
    
    // Equality operator
    template<typename T>
    inline bool operator==(ImportDirList::ImportDirIter<T> const& Lhs, 
      ImportDirList::ImportDirIter<T> const& Rhs)
    {
      return (Lhs.m_Num == Rhs.m_Num);
    }
        
    // Inequality operator
    template<typename T>    
    inline bool operator!=(ImportDirList::ImportDirIter<T> const& Lhs, 
      ImportDirList::ImportDirIter<T> const& Rhs)
    {
      return !(Lhs == Rhs);
    }

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
      template <typename ImportThunkT>
      class ImportThunkIter : public std::iterator<std::input_iterator_tag, 
        ImportThunkT>
      {
      public:
        // ImportThunk iterator error class
        class Error : public virtual HadesMemError
        { };

        // Constructor
        ImportThunkIter() 
          : m_PeFile(), 
          m_pThunk(nullptr), 
          m_ImportThunk()
        { }
        
        // Constructor
        ImportThunkIter(PeFile const& MyPeFile, DWORD FirstThunk) 
          : m_PeFile(MyPeFile), 
          m_pThunk(nullptr), 
          m_ImportThunk()
        {
          m_pThunk = reinterpret_cast<PIMAGE_THUNK_DATA>(m_PeFile->RvaToVa(
            FirstThunk));
          m_ImportThunk = ImportThunk(*m_PeFile, m_pThunk);
          if (!m_ImportThunk->IsValid())
          {
            m_PeFile = boost::optional<PeFile>();
            m_pThunk = nullptr;
            m_ImportThunk = boost::optional<ImportThunk>();
          }
        }
        
        // Copy constructor
        template <typename OtherT>
        ImportThunkIter(ImportThunkIter<OtherT> const& Rhs) 
          : m_PeFile(Rhs.m_PeFile), 
          m_pThunk(Rhs.m_pThunk), 
          m_ImportThunk(Rhs.m_ImportThunk)
        { }
        
        // Assignment operator
        template <typename OtherT>
        ImportThunkIter& operator=(ImportThunkIter<OtherT> const& Rhs) 
        {
          m_PeFile = Rhs.m_PeFile;
          m_pThunk = Rhs.m_pThunk;
          m_ImportThunk = Rhs.m_ImportThunk;
        }
        
        // Prefix increment
        ImportThunkIter& operator++()
        {
          m_ImportThunk = ImportThunk(*m_PeFile, ++m_pThunk);
          if (!m_ImportThunk->IsValid())
          {
            m_PeFile = boost::optional<PeFile>();
            m_pThunk = nullptr;
            m_ImportThunk = boost::optional<ImportThunk>();
          }
          
          return *this;
        }
        
        // Postfix increment
        ImportThunkIter operator++(int)
        {
          ImportThunkIter Temp(*this);
          ++*this;
          return Temp;
        }
        
        // Dereference operator
        reference operator*()
        {
          return *m_ImportThunk;
        }
        
        // Dereference operator
        pointer operator->()
        {
          return &*m_ImportThunk;
        }
        
        // Equality operator
        template<typename T>
        friend bool operator==(const ImportThunkIter<T>& Rhs, 
          const ImportThunkIter<T>& Lhs);
        
        // Inequality operator
        template<typename T>
        friend bool operator!=(const ImportThunkIter<T>& Rhs, 
          const ImportThunkIter<T>& Lhs);

      private:
        // PE file
        boost::optional<PeFile> m_PeFile;
        // Current thunk pointer
        PIMAGE_THUNK_DATA m_pThunk;
        // ImportThunk object
        boost::optional<ImportThunk> m_ImportThunk;
      };
      
      // ImportThunk list iterator types
      typedef ImportThunkIter<const ImportThunk> const_iterator;
      typedef ImportThunkIter<ImportThunk> iterator;
      
      // Constructor
      ImportThunkList(PeFile const& MyPeFile, DWORD FirstThunk)
        : m_PeFile(MyPeFile), 
        m_FirstThunk(FirstThunk)
      { }
      
      // Get start of importthunk list
      iterator begin()
      {
        return iterator(m_PeFile, m_FirstThunk);
      }
      
      // Get end of importthunk list
      iterator end()
      {
        return iterator();
      }
      
    private:
      // PE file
      PeFile m_PeFile;
      // First thunk
      DWORD m_FirstThunk;
    };
    
    // Equality operator
    template<typename T>
    inline bool operator==(ImportThunkList::ImportThunkIter<T> const& Lhs, 
      ImportThunkList::ImportThunkIter<T> const& Rhs)
    {
      return (Lhs.m_pThunk == Rhs.m_pThunk);
    }
        
    // Inequality operator
    template<typename T>    
    inline bool operator!=(ImportThunkList::ImportThunkIter<T> const& Lhs, 
      ImportThunkList::ImportThunkIter<T> const& Rhs)
    {
      return !(Lhs == Rhs);
    }
  }
}
