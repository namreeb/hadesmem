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
#include <boost/filesystem.hpp>

// Windows API
#include <Windows.h>
#include <TlHelp32.h>

// Hades
#include "Fwd.hpp"
#include "Error.hpp"
#include "MemoryMgr.hpp"

namespace Hades
{
  namespace Memory
  {
    // Module managing class
    class Module
    {
    public:
      // Module exception type
      class Error : public virtual HadesMemError
      { };

      // Create module
      Module(MemoryMgr const& MyMemory, MODULEENTRY32 const& ModuleEntry);

      // Find module by handle
      Module(MemoryMgr const& MyMemory, HMODULE Handle);

      // Find module by name
      Module(MemoryMgr const& MyMemory, std::wstring const& ModuleName);

      // Get module base
      HMODULE GetBase() const;
      // Get module size
      DWORD GetSize() const;

      // Get module name
      std::wstring GetName() const;
      // Get module path
      boost::filesystem::path GetPath() const;

    private:
      // Memory instance
      MemoryMgr m_Memory;

      // Module base address
      HMODULE m_Base;
      // Module size
      DWORD m_Size;
      // Module name
      std::wstring m_Name;
      // Module path
      boost::filesystem::path m_Path;
    };
    
    // Module enumeration class
    class ModuleList
    {
    public:
      // ModuleList exception type
      class Error : public virtual HadesMemError
      { };

      // Module iterator
      template <typename ModuleT>
      class ModuleIter : public std::iterator<std::input_iterator_tag, 
        ModuleT>
      {
      public:
        // Module iterator error class
        class Error : public virtual HadesMemError
        { };

        // Constructor
        ModuleIter() : 
          m_pParent(nullptr), 
          m_Number(static_cast<DWORD>(-1)), 
          m_Current()
        { }
        
        // Constructor
        ModuleIter(ModuleList& Parent) 
          : m_pParent(&Parent), 
          m_Number(0), 
          m_Current()
        {
          boost::optional<Module&> Temp = m_pParent->GetByNum(m_Number);
          if (Temp)
          {
            m_Current = *Temp;
          }
          else
          {
            m_pParent = nullptr;
            m_Number = static_cast<DWORD>(-1);
          }
        }
        
        // Copy constructor
        template <typename OtherT>
        ModuleIter(ModuleIter<OtherT> const& Rhs) 
          : m_pParent(Rhs.m_pParent), 
          m_Number(Rhs.m_Number), 
          m_Current(Rhs.m_Current)
        { }
        
        // Assignment operator
        template <typename OtherT>
        ModuleIter& operator=(ModuleIter<OtherT> const& Rhs) 
        {
          m_pParent = Rhs.m_pParent;
          m_Number = Rhs.m_Number;
          m_Current = Rhs.m_Current;
        }
        
        // Prefix increment
        ModuleIter& operator++()
        {
          boost::optional<Module&> Temp = m_pParent->GetByNum(++m_Number);
          m_Current = Temp ? *Temp : boost::optional<Module>();
          if (!Temp)
          {
            m_pParent = nullptr;
            m_Number = static_cast<DWORD>(-1);
          }
          return *this;
        }
        
        // Postfix increment
        ModuleIter operator++(int)
        {
          ModuleIter Temp(*this);
          ++*this;
          return Temp;
        }
        
        // Dereference operator
        reference operator*()
        {
          return *m_Current;
        }
        
        // Dereference operator
        pointer operator->()
        {
          return &*m_Current;
        }
        
        // Equality operator
        template<typename T>
        friend bool operator==(const ModuleIter<T>& Rhs, 
          const ModuleIter<T>& Lhs);
        
        // Inequality operator
        template<typename T>
        friend bool operator!=(const ModuleIter<T>& Rhs, 
          const ModuleIter<T>& Lhs);

      private:
        // Parent list instance
        class ModuleList* m_pParent;
        // Module number
        DWORD m_Number;
        // Current module instance
        boost::optional<Module> m_Current;
      };
      
      // Module list iterator types
      typedef ModuleIter<const Module> const_iterator;
      typedef ModuleIter<Module> iterator;
      
      // Constructor
      ModuleList(MemoryMgr const& MyMemory)
        : m_Memory(MyMemory), 
        m_Snap(), 
        m_Cache()
      { }
      
      // Get start of module list
      iterator begin()
      {
        return iterator(*this);
      }
      
      // Get end of module list
      iterator end()
      {
        return iterator();
      }
      
      // Get module from cache by number
      boost::optional<Module&> GetByNum(DWORD Num)
      {
        while (Num >= m_Cache.size())
        {
          if (m_Cache.empty())
          {
            m_Snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | 
              TH32CS_SNAPMODULE32, m_Memory.GetProcessID());
            if (m_Snap == INVALID_HANDLE_VALUE)
            {
              std::error_code const LastError = GetLastErrorCode();
              BOOST_THROW_EXCEPTION(Error() << 
                ErrorFunction("ModuleList::GetByNum") << 
                ErrorString("Could not get module snapshot.") << 
                ErrorCode(LastError));
            }
    
            MODULEENTRY32 MyModuleEntry = { sizeof(MyModuleEntry) };
            if (!Module32First(m_Snap, &MyModuleEntry))
            {
              std::error_code const LastError = GetLastErrorCode();
              BOOST_THROW_EXCEPTION(Error() << 
                ErrorFunction("ModuleList::GetByNum") << 
                ErrorString("Could not get module info.") << 
                ErrorCode(LastError));
            }
    
            m_Cache.push_back(Module(m_Memory, MyModuleEntry));
          }
          else
          {
            MODULEENTRY32 MyModuleEntry = { sizeof(MyModuleEntry) };
            if (!Module32Next(m_Snap, &MyModuleEntry))
            {
              if (GetLastError() != ERROR_NO_MORE_FILES)
              {
                std::error_code const LastError = GetLastErrorCode();
                BOOST_THROW_EXCEPTION(Error() << 
                  ErrorFunction("ModuleList::GetByNum") << 
                  ErrorString("Error enumerating module list.") << 
                  ErrorCode(LastError));
              }
              
              return boost::optional<Module&>();
            }
            else
            {
              m_Cache.push_back(Module(m_Memory, MyModuleEntry));
            }
          }
        }
        
        return m_Cache[Num];
      }
      
    private:
      // Memory instance
      MemoryMgr m_Memory;
      // Snapshot handle
      Windows::EnsureCloseSnap m_Snap;
      // Module cache
      std::vector<Module> m_Cache;
    };
    
    // Equality operator
    template<typename T>
    inline bool operator==(ModuleList::ModuleIter<T> const& Lhs, 
      ModuleList::ModuleIter<T> const& Rhs)
    {
      return (Lhs.m_pParent == Rhs.m_pParent && Lhs.m_Number == Rhs.m_Number);
    }
        
    // Inequality operator
    template<typename T>    
    inline bool operator!=(ModuleList::ModuleIter<T> const& Lhs, 
      ModuleList::ModuleIter<T> const& Rhs)
    {
      return !(Lhs == Rhs);
    }
  }
}
