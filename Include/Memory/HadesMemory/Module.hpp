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

// C++ Standard Library
#include <string>

// Boost
#include <boost/iterator.hpp>
#include <boost/optional.hpp>
#include <boost/filesystem.hpp>

// Windows API
#include <Windows.h>
#include <TlHelp32.h>

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

      // Find module by handle
      Module(MemoryMgr const& MyMemory, HMODULE Handle);

      // Find module by name
      Module(MemoryMgr const& MyMemory, std::wstring const& ModuleName);

      // Create module
      Module(MemoryMgr const& MyMemory, MODULEENTRY32 const& ModuleEntry);

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
      class ModuleIter : public boost::iterator_facade<ModuleIter, Module, 
        boost::forward_traversal_tag>
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
        ModuleIter(ModuleIter const& Rhs) 
          : m_pParent(Rhs.m_pParent), 
          m_Number(Rhs.m_Number), 
          m_Current(Rhs.m_Current)
        { }
        
        // Assignment operator
        ModuleIter& operator=(ModuleIter const& Rhs) 
        {
          m_pParent = Rhs.m_pParent;
          m_Number = Rhs.m_Number;
          m_Current = Rhs.m_Current;
          return *this;
        }
      
      private:
        // Give Boost.Iterator access to internals
        friend class boost::iterator_core_access;

        // Increment iterator
        void increment() 
        {
          boost::optional<Module&> Temp = m_pParent->GetByNum(++m_Number);
          m_Current = Temp ? *Temp : boost::optional<Module>();
          if (!Temp)
          {
            m_pParent = nullptr;
            m_Number = static_cast<DWORD>(-1);
          }
        }
        
        // Check iterator for equality
        bool equal(ModuleIter const& Rhs) const
        {
          return this->m_pParent == Rhs.m_pParent && 
            this->m_Number == Rhs.m_Number;
        }
    
        // Dereference iterator
        Module& dereference() const 
        {
          return *m_Current;
        }

        // Parent list instance
        class ModuleList* m_pParent;
        // Module number
        DWORD m_Number;
        // Current module instance
        mutable boost::optional<Module> m_Current;
      };
      
      // Module list iterator types
      typedef ModuleIter iterator;
      
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
            m_Snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 
              m_Memory.GetProcessID());
            if (m_Snap == INVALID_HANDLE_VALUE)
            {
              DWORD const LastError = GetLastError();
              BOOST_THROW_EXCEPTION(Error() << 
                ErrorFunction("ModuleList::GetByNum") << 
                ErrorString("Could not get module snapshot.") << 
                ErrorCodeWinLast(LastError));
            }
    
            MODULEENTRY32 MyModuleEntry;
            ZeroMemory(&MyModuleEntry, sizeof(MyModuleEntry));
            MyModuleEntry.dwSize = sizeof(MyModuleEntry);
            if (!Module32First(m_Snap, &MyModuleEntry))
            {
              DWORD const LastError = GetLastError();
              BOOST_THROW_EXCEPTION(Error() << 
                ErrorFunction("ModuleList::GetByNum") << 
                ErrorString("Could not get module info.") << 
                ErrorCodeWinLast(LastError));
            }
    
            m_Cache.push_back(Module(m_Memory, MyModuleEntry));
          }
          else
          {
            MODULEENTRY32 MyModuleEntry;
            ZeroMemory(&MyModuleEntry, sizeof(MyModuleEntry));
            MyModuleEntry.dwSize = sizeof(MyModuleEntry);
            if (!Module32Next(m_Snap, &MyModuleEntry))
            {
              if (GetLastError() != ERROR_NO_MORE_FILES)
              {
                DWORD const LastError = GetLastError();
                BOOST_THROW_EXCEPTION(Error() << 
                  ErrorFunction("ModuleList::GetByNum") << 
                  ErrorString("Error enumerating module list.") << 
                  ErrorCodeWinLast(LastError));
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
  }
}
