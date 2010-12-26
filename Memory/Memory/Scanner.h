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

// Windows API
#include <tchar.h>
#include <Windows.h>

// C++ Standard Library
#include <map>
#include <string>
#include <vector>

// Boost
#ifdef _MSC_VER
#pragma warning(push, 1)
#endif // #ifdef _MSC_VER
#include <boost/filesystem.hpp>
#include <boost/noncopyable.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif // #ifdef _MSC_VER

// Hades
#include "Region.h"
#include "MemoryMgr.h"

namespace Hades
{
  namespace Memory
  {
    // Memory searching class
    class Scanner
    {
    public:
      // Scanner exception type
      class Error : public virtual HadesMemError 
      { };

      // Constructor
      Scanner(MemoryMgr const& MyMemory, PVOID Start, PVOID End);

      // Search memory (POD types)
      template <typename T>
      PVOID Find(T const& Data, typename boost::enable_if<std::is_pod<T>>::
        type* Dummy = 0) const;

      // Search memory (string types)
      template <typename T>
      PVOID Find(T const& Data, typename boost::enable_if<std::is_same<T, std::
        basic_string<typename T::value_type>>>::type* Dummy = 0) const;

      // Search memory (vector types)
      template <typename T>
      PVOID Find(T const& Data, typename boost::enable_if<std::is_same<T, 
        std::vector<typename T::value_type>>>::type* Dummy1 = 0) const;

      // Search memory (POD types)
      template <typename T>
      std::vector<PVOID> FindAll(T const& data, typename boost::enable_if<std::
        is_pod<T>>::type* Dummy = 0) const;

      // Search memory (string types)
      template <typename T>
      std::vector<PVOID> FindAll(T const& Data, typename boost::enable_if<std::
        is_same<T, std::basic_string<typename T::value_type>>>::type* Dummy 
        = 0) const;

      // Search memory (vector types)
      template <typename T>
      std::vector<PVOID> FindAll(T const& Data, typename boost::enable_if<std::
        is_same<T, std::vector<typename T::value_type>>>::type* Dummy1 = 0) 
        const;

    private:
      // Memory manager instance
      MemoryMgr m_Memory;

      // Start and end addresses of search region
      PBYTE m_Start;
      PBYTE m_End;
    };

    // Search memory (POD types)
    template <typename T>
    PVOID Scanner::Find(T const& Data, typename boost::enable_if<std::is_pod<
      T>>:: type* /*Dummy*/) const
    {
      // Put data in container
      std::vector<T> Buffer;
      Buffer.push_back(Data);
      // Use vector specialization of FindAll
      return Find(Buffer);
    }

    // Search memory (string types)
    template <typename T>
    PVOID Scanner::Find(T const& Data, typename boost::enable_if<std::is_same<
      T, std::basic_string<typename T::value_type>>>::type* /*Dummy*/) const
    {
      // Convert string to character buffer
      std::vector<T::value_type> const MyBuffer(Data.cbegin(), Data.cend());
      // Use vector specialization of find
      return Find(MyBuffer);
    }

    // Search memory (vector types)
    template <typename T>
    PVOID Scanner::Find(T const& Data, typename boost::enable_if<std::is_same<
      T, std::vector<typename T::value_type>>>::type* /*Dummy1*/) const
    {
      static_assert(std::is_pod<T::value_type>::value, "Scanner::Find: Value "
        "type of vector must be POD.");

      if (Data.empty())
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Scanner::Find") << 
          ErrorString("Data container is empty."));
      }

      LPCBYTE pDataRaw = reinterpret_cast<LPCBYTE>(&Data[0]);
      std::size_t const DataRawSize = Data.size() * sizeof(T::value_type);

      std::vector<BYTE> DataRaw(pDataRaw, pDataRaw + DataRawSize);

      for (RegionListIter i(m_Memory); *i; ++i)
      {
        Hades::Memory::Region const& MyRegion = **i;

        if (static_cast<PBYTE>(MyRegion.GetBase()) + MyRegion.GetSize() < 
          m_Start)
        {
          continue;
        }

        if (MyRegion.GetBase() > m_End)
        {
          break;
        }

        std::vector<BYTE> Buffer;

        try
        {
          Buffer = m_Memory.Read<std::vector<BYTE>>(MyRegion.GetBase(), 
            MyRegion.GetSize());
        }
        catch (MemoryMgr::Error const& /*e*/)
        {
          continue;
        }

        auto Iter = std::search(Buffer.cbegin(), Buffer.cend(), 
          DataRaw.cbegin(), DataRaw.cend());
        if (Iter != Buffer.cend())
        {
          PVOID AddressReal = static_cast<PBYTE>(MyRegion.GetBase()) + 
            std::distance(Buffer.cbegin(), Iter);
          if (AddressReal >= m_Start && AddressReal <= m_End)
          {
            return AddressReal;
          }
        }
      }

      return nullptr;
    }

    // Search memory (POD types)
    template <typename T>
    std::vector<PVOID> Scanner::FindAll(T const& Data, typename boost::
      enable_if<std::is_pod<T>>::type* /*Dummy*/) const
    {
      // Put data in container
      std::vector<T> Buffer;
      Buffer.push_back(Data);
      // Use vector specialization of FindAll
      return FindAll(Buffer);
    }

    template <typename T>
    std::vector<PVOID> Scanner::FindAll(T const& Data, typename boost::
      enable_if<std::is_same<T, std::basic_string<typename T::value_type>>>::
      type* /*Dummy*/) const
    {
      // Convert string to character buffer
      std::vector<T::value_type> const MyBuffer(Data.cbegin(), Data.cend());
      // Use vector specialization of find all
      return FindAll(MyBuffer);
    }

    // Search memory (vector types)
    // Fixme: This function is extremely inefficient and full of potential 
    // bugs. Perform a thorough review and rewrite.
    // Fixme: Refactor Find and FindAll to factor out duplicated code.
    template <typename T>
    std::vector<PVOID> Scanner::FindAll(T const& Data, typename boost::
      enable_if<std::is_same<T, std::vector<typename T::value_type>>>::type* 
      /*Dummy1*/) const
    {
      static_assert(std::is_pod<T::value_type>::value, "Scanner::Find: Value "
        "type of vector must be POD.");

      if (Data.empty())
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("Scanner::Find") << 
          ErrorString("Data container is empty."));
      }

      LPCBYTE pDataRaw = reinterpret_cast<LPCBYTE>(&Data[0]);
      std::size_t const DataRawSize = Data.size() * sizeof(T::value_type);

      std::vector<BYTE> DataRaw(pDataRaw, pDataRaw + DataRawSize);

      std::vector<PVOID> Matches;

      for (RegionListIter i(m_Memory); *i; ++i)
      {
        Hades::Memory::Region const& MyRegion = **i;

        if (static_cast<PBYTE>(MyRegion.GetBase()) + MyRegion.GetSize() < 
          m_Start)
        {
          continue;
        }

        if (MyRegion.GetBase() > m_End)
        {
          break;
        }

        std::vector<BYTE> Buffer;

        try
        {
          Buffer = m_Memory.Read<std::vector<BYTE>>(MyRegion.GetBase(), 
            MyRegion.GetSize());
        }
        catch (MemoryMgr::Error const& /*e*/)
        {
          continue;
        }

        auto Iter = std::search(Buffer.cbegin(), Buffer.cend(), 
          DataRaw.cbegin(), DataRaw.cend());
        if (Iter != Buffer.cend())
        {
          PVOID AddressReal = static_cast<PBYTE>(MyRegion.GetBase()) + 
            std::distance(Buffer.cbegin(), Iter);
          if (AddressReal >= m_Start && AddressReal <= m_End)
          {
            Matches.push_back(AddressReal);
          }
        }
      }

      return Matches;
    }
  }
}
