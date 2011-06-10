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
#include <HadesMemory/Error.hpp>
#include <HadesMemory/Process.hpp>
#include <HadesCommon/TypeTraits.hpp>

// C++ Standard Library
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <type_traits>

// Boost
#include <boost/config.hpp>
#include <boost/filesystem.hpp>

// Windows API
#include <Windows.h>

namespace Hades
{
  namespace Memory
  {
    // Memory managing class
    class MemoryMgr
    {
    public:
      // Memory exception type
      class Error : public virtual HadesMemError 
      { };

      // Open process from process ID
      explicit MemoryMgr(DWORD ProcID);

      // Open process from process name
      explicit MemoryMgr(std::wstring const& ProcName);

      // Open process from window name and class
      MemoryMgr(std::wstring const& WindowName, std::wstring const& ClassName);

      // MemoryMgr::Call return data
      class RemoteFunctionRet
      {
      public:
        RemoteFunctionRet() 
          : m_ReturnValue(0), 
          m_LastError(0)
        { }
        
        RemoteFunctionRet(DWORD_PTR ReturnValue, DWORD LastError) 
          : m_ReturnValue(ReturnValue), 
          m_LastError(LastError)
        { }
        
        DWORD_PTR GetReturnValue() const
        {
          return m_ReturnValue;
        }
        
        DWORD GetLastError() const
        {
          return m_LastError;
        }
        
      private:
        DWORD_PTR m_ReturnValue;
        DWORD m_LastError;
      };
    
      // Calling conventions
      enum CallConv
      {
        CallConv_Default, 
        CallConv_CDECL, 
        CallConv_STDCALL, 
        CallConv_THISCALL, 
        CallConv_FASTCALL, 
        CallConv_X64
      };

      // Call remote function
      RemoteFunctionRet Call(LPCVOID Address, CallConv MyCallConv, 
        std::vector<PVOID> const& Args) const;

#if !defined(BOOST_NO_INITIALIZER_LISTS) && !defined(BOOST_NO_VARIADIC_TEMPLATES)
      // Call remote function
      template <typename... T>
      RemoteFunctionRet Call(LPCVOID Address, CallConv MyCallConv, 
        T const&... Args) const
      {
        static_assert(Util::all_memsize_or_less<T...>::value, 
          "Size of types must be <= memsize.");
        static_assert(Util::all_pod<T...>::value, "All types must be POD.");
        std::vector<PVOID> NewArgs { reinterpret_cast<PVOID>(Args)... };
        return Call(Address, MyCallConv, NewArgs);
      }
#endif

      // Read memory (POD types)
      template <typename T>
      T Read(PVOID Address, typename std::enable_if<std::is_pod<T>::value, 
        T>::type* Dummy = 0) const;

      // Read memory (string types)
      template <typename T>
      T Read(PVOID Address, typename std::enable_if<std::is_same<T, std::
        basic_string<typename T::value_type>>::value, T>::type* Dummy = 0) 
        const;

      // Read memory (vector types)
      template <typename T>
      T Read(PVOID Address, std::size_t Size, typename std::enable_if<std::
        is_same<T, std::vector<typename T::value_type>>::value, T>::type* 
        Dummy = 0) const;

      // Write memory (POD types)
      template <typename T>
      void Write(PVOID Address, T const& Data, typename std::enable_if<std::
        is_pod<T>::value, T>::type* Dummy = 0) const;

      // Write memory (string types)
      template <typename T>
      void Write(PVOID Address, T const& Data, typename std::enable_if<std::
        is_same<T, std::basic_string<typename T::value_type>>::value, T>::
        type* Dummy = 0) const;

      // Write memory (vector types)
      template <typename T>
      void Write(PVOID Address, T const& Data, typename std::enable_if<std::
        is_same<T, std::vector<typename T::value_type>>::value, T>::type* 
        Dummy = 0) const;

      // Whether an address is currently readable
      bool CanRead(LPCVOID Address) const;

      // Whether an address is currently writable
      bool CanWrite(LPCVOID Address) const;

      // Whether an address is contained within a guard page
      bool IsGuard(LPCVOID Address) const;

      // Allocate memory
      PVOID Alloc(SIZE_T Size) const;

      // Free memory
      void Free(PVOID Address) const;

      // Get process ID of target
      DWORD GetProcessID() const;

      // Get process handle of target
      HANDLE GetProcessHandle() const;

      // Get address of export in remote process (by name)
      FARPROC GetRemoteProcAddress(HMODULE RemoteMod, 
        boost::filesystem::path const& Module, std::string const& Function) 
        const;

      // Get address of export in remote process (by ordinal)
      FARPROC GetRemoteProcAddress(HMODULE RemoteMod, 
        boost::filesystem::path const& Module, WORD Function) const;

      // Flush instruction cache
      void FlushCache(LPCVOID Address, SIZE_T Size) const;
      
      // Is WoW64 process
      bool IsWoW64() const;

    private:
      // Target process
      Process m_Process;
    };

    // RAII class for remote memory allocation and freeing
    class AllocAndFree
    {
    public:
      AllocAndFree(MemoryMgr const& MyMemoryMgr, SIZE_T Size) 
        : m_Memory(MyMemoryMgr), 
        m_Size(Size), 
        m_Address(MyMemoryMgr.Alloc(Size)) 
      { }
      
      void Free() const
      {
        if (m_Address)
        {
          m_Memory.Free(m_Address);
          m_Address = nullptr;
        }
      }

      ~AllocAndFree()
      {
        try
        {
          Free();
        }
        catch (std::exception const& e)
        {
          OutputDebugStringA(boost::diagnostic_information(e).c_str());
        }
        catch (...)
        {
          OutputDebugString(L"AllocAndFree::~AllocAndFree: Unknown error.");
        }
      }

      PVOID GetBase() const 
      {
        return m_Address;
      }
      
      SIZE_T GetSize() const
      {
        return m_Size;
      }

    protected:
      AllocAndFree(AllocAndFree const&);
      AllocAndFree& operator=(AllocAndFree const&);
      
    private:
      MemoryMgr m_Memory;
      SIZE_T m_Size;
      mutable PVOID m_Address;
    };

    // Read memory (POD types)
    template <typename T>
    T MemoryMgr::Read(PVOID Address, typename std::enable_if<std::is_pod<T>::
      value, T>::type* /*Dummy*/) const 
    {
      // Treat attempt to read from a guard page as an error
      if (IsGuard(Address))
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("MemoryMgr::Read") << 
          ErrorString("Attempt to read from guard page."));
      }
      
      // Whether we can read the given address
      bool const CanReadMem = CanRead(Address);

      // Set page protection for reading
      DWORD OldProtect = 0;
      if (!CanReadMem)
      {
        if (!VirtualProtectEx(m_Process.GetHandle(), Address, sizeof(T), 
          PAGE_EXECUTE_READWRITE, &OldProtect))
        {
          DWORD const LastError = GetLastError();
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("MemoryMgr::Read") << 
            ErrorString("Could not change process memory protection.") << 
            ErrorCodeWinLast(LastError));
        }
      }

      // Read data
      T Out = T();
      SIZE_T BytesRead = 0;
      if (!ReadProcessMemory(m_Process.GetHandle(), Address, &Out, sizeof(T), 
        &BytesRead) || BytesRead != sizeof(T))
      {
        if (!CanReadMem)
        {
          // Restore original page protections
          VirtualProtectEx(m_Process.GetHandle(), Address, sizeof(T), 
            OldProtect, &OldProtect);
        }

        DWORD const LastError = GetLastError();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("MemoryMgr::Read") << 
          ErrorString("Could not read process memory.") << 
          ErrorCodeWinLast(LastError));
      }

      // Restore original page protections
      if (!CanReadMem)
      {
        if (!VirtualProtectEx(m_Process.GetHandle(), Address, sizeof(T), 
          OldProtect, &OldProtect))
        {
          DWORD const LastError = GetLastError();
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("MemoryMgr::Read") << 
            ErrorString("Could not restore process memory protection.") << 
            ErrorCodeWinLast(LastError));
        }
      }

      return Out;
    }

    // Read memory (string types)
    template <typename T>
    T MemoryMgr::Read(PVOID Address, typename std::enable_if<std::is_same<T, 
      std::basic_string<typename T::value_type>>::value, T>::type* /*Dummy*/) 
      const
    {
      // Character type
      typedef typename T::value_type CharT;

      // Create buffer to store results
      std::basic_string<CharT> Buffer;

      // Loop until a null terminator is found
      for (CharT* AddressReal = static_cast<CharT*>(Address);; ++AddressReal)
      {
        // Read current character
        CharT const Current = Read<CharT>(AddressReal);

        // Return generated string on null terminator
        if (Current == 0)
        {
          return Buffer;
        }

        // Add character to buffer
        Buffer += Current;
      }
    }

    // Read memory (vector types)
    template <typename T>
    T MemoryMgr::Read(PVOID Address, std::size_t Size, typename 
      std::enable_if<std::is_same<T, std::vector<typename T::value_type>>::
      value, T>::type* /*Dummy*/) const
    {
      // Ensure type to be read is POD
      static_assert(std::is_pod<typename T::value_type>::value, 
        "MemoryMgr::Read: Value type of vector must be POD.");

      // Treat attempt to read from a guard page as an error
      if (IsGuard(Address))
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("MemoryMgr::Read") << 
          ErrorString("Attempt to read from guard page."));
      }

      // Calculate 'raw' size of data
      std::size_t RawSize = Size * sizeof(typename T::value_type);

      // Whether we can read the given address
      bool const CanReadMem = CanRead(Address);

      // Set page protection for reading
      DWORD OldProtect = 0;
      if (!CanReadMem)
      {
        if (!VirtualProtectEx(m_Process.GetHandle(), Address, RawSize, 
          PAGE_EXECUTE_READWRITE, &OldProtect))
        {
          DWORD const LastError = GetLastError();
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("MemoryMgr::Read") << 
            ErrorString("Could not change process memory protection.") << 
            ErrorCodeWinLast(LastError));
        }
      }

      // Read data
      T Buffer(Size);
      SIZE_T BytesRead = 0;
      if (!ReadProcessMemory(m_Process.GetHandle(), Address, Buffer.data(), 
        RawSize, &BytesRead) || BytesRead != RawSize)
      {
        if (!CanReadMem)
        {
          // Restore original page protections
          VirtualProtectEx(m_Process.GetHandle(), Address, RawSize, 
            OldProtect, &OldProtect);
        }

        DWORD const LastError = GetLastError();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("MemoryMgr::Read") << 
          ErrorString("Could not read process memory.") << 
          ErrorCodeWinLast(LastError));
      }

      // Restore original page protections
      if (!CanReadMem)
      {
        if (!VirtualProtectEx(m_Process.GetHandle(), Address, RawSize, 
          OldProtect, &OldProtect))
        {
          DWORD const LastError = GetLastError();
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("MemoryMgr::Read") << 
            ErrorString("Could not restore process memory protection.") << 
            ErrorCodeWinLast(LastError));
        }
      }

      // Return buffer
      return Buffer;
    }

    // Write memory (POD types)
    template <typename T>
    void MemoryMgr::Write(PVOID Address, T const& Data, typename std::
      enable_if<std::is_pod<T>::value, T>::type* /*Dummy*/) const 
    {
      // Treat attempt to write to a guard page as an error
      if (IsGuard(Address))
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("MemoryMgr::Write") << 
          ErrorString("Attempt to write to guard page."));
      }

      // Set page protections for writing
      DWORD OldProtect = 0;
      if (!VirtualProtectEx(m_Process.GetHandle(), Address, sizeof(T), 
        PAGE_EXECUTE_READWRITE, &OldProtect))
      {
        DWORD const LastError = GetLastError();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("MemoryMgr::Write") << 
          ErrorString("Could not change process memory protection.") << 
          ErrorCodeWinLast(LastError));
      }

      // Write data
      SIZE_T BytesWritten = 0;
      if (!WriteProcessMemory(m_Process.GetHandle(), Address, &Data, sizeof(T), 
        &BytesWritten) || BytesWritten != sizeof(T))
      {
        // Restore original page protections
        VirtualProtectEx(m_Process.GetHandle(), Address, sizeof(T), OldProtect, 
          &OldProtect);

        DWORD const LastError = GetLastError();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("MemoryMgr::Write") << 
          ErrorString("Could not write process memory.") << 
          ErrorCodeWinLast(LastError));
      }

      // Restore original page protections
      if (!VirtualProtectEx(m_Process.GetHandle(), Address, sizeof(T), 
        OldProtect, &OldProtect))
      {
        DWORD const LastError = GetLastError();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("MemoryMgr::Write") << 
          ErrorString("Could not restore process memory protection.") << 
          ErrorCodeWinLast(LastError));
      }
    }

    // Write memory (string types)
    template <typename T>
    void MemoryMgr::Write(PVOID Address, T const& Data, 
      typename std::enable_if<std::is_same<T, std::basic_string<typename T::
      value_type>>::value, T>::type* /*Dummy*/) const
    {
      // Character type
      typedef typename T::value_type CharT;

      // Convert string to vector
      std::vector<CharT> DataReal(Data.cbegin(), Data.cend());
      DataReal.push_back(0);

      // Write string to memory
      this->Write(Address, DataReal);
    }

    // Write memory (vector types)
    template <typename T>
    void MemoryMgr::Write(PVOID Address, T const& Data, typename std::
      enable_if<std::is_same<T, std::vector<typename T::value_type>>::value, 
      T>::type* /*Dummy*/) const
    {
      // Ensure type to be written is POD
      static_assert(std::is_pod<typename T::value_type>::value, 
        "MemoryMgr::Write: Value type of vector must be POD.");

      // Treat attempt to write to a guard page as an error
      if (IsGuard(Address))
      {
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("MemoryMgr::Write") << 
          ErrorString("Attempt to write to guard page."));
      }

      // Calculate 'raw' size of data
      std::size_t RawSize = Data.size() * sizeof(typename T::value_type);

      // Whether we can write to the given address
      bool const CanWriteMem = CanWrite(Address);

      // Set page protection for writing
      DWORD OldProtect = 0;
      if (!CanWriteMem)
      {
        if (!VirtualProtectEx(m_Process.GetHandle(), Address, RawSize, 
          PAGE_EXECUTE_READWRITE, &OldProtect))
        {
          DWORD const LastError = GetLastError();
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("MemoryMgr::Write") << 
            ErrorString("Could not change process memory protection.") << 
            ErrorCodeWinLast(LastError));
        }
      }

      // Read data
      SIZE_T BytesWritten = 0;
      if (!WriteProcessMemory(m_Process.GetHandle(), Address, Data.data(), 
        RawSize, &BytesWritten) || BytesWritten != RawSize)
      {
        if (!CanWriteMem)
        {
          // Restore original page protections
          VirtualProtectEx(m_Process.GetHandle(), Address, RawSize, 
            OldProtect, &OldProtect);
        }

        DWORD const LastError = GetLastError();
        BOOST_THROW_EXCEPTION(Error() << 
          ErrorFunction("MemoryMgr::Write") << 
          ErrorString("Could not read process memory.") << 
          ErrorCodeWinLast(LastError));
      }

      // Restore original page protections
      if (!CanWriteMem)
      {
        if (!VirtualProtectEx(m_Process.GetHandle(), Address, RawSize, 
          OldProtect, &OldProtect))
        {
          DWORD const LastError = GetLastError();
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("MemoryMgr::Write") << 
            ErrorString("Could not restore process memory protection.") << 
            ErrorCodeWinLast(LastError));
        }
      }
    }
  }
}
