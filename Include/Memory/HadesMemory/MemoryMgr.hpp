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
#include <HadesMemory/Detail/Error.hpp>
#include <HadesMemory/Detail/Process.hpp>

// C++ Standard Library
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <type_traits>

// Windows API
#include <Windows.h>

namespace HadesMem
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

    // MemoryMgr::Call return data
    class RemoteFunctionRet
    {
    public:
      RemoteFunctionRet(DWORD_PTR ReturnValue, DWORD64 ReturnValue64, 
        DWORD LastError);
      
      DWORD_PTR GetReturnValue() const;
      
      DWORD64 GetReturnValue64() const;
      
      DWORD GetLastError() const;
      
    private:
      DWORD_PTR m_ReturnValue;
      DWORD64 m_ReturnValue64;
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

    // Read memory (POD types)
    template <typename T>
    T Read(PVOID Address, typename std::enable_if<std::is_pod<T>::value, 
      T>::type* Dummy = 0) const;

    // Read memory (string types)
    template <typename T>
    T ReadString(PVOID Address, typename std::enable_if<std::is_same<T, std::
      basic_string<typename T::value_type>>::value, T>::type* Dummy = 0) 
      const;

    // Read memory (vector types)
    template <typename T>
    T ReadList(PVOID Address, std::size_t Size, typename std::enable_if<std::
      is_same<T, std::vector<typename T::value_type>>::value, T>::type* 
      Dummy = 0) const;
      
    // Write memory (POD types)
    template <typename T>
    void Write(PVOID Address, T const& Data, typename std::enable_if<std::
      is_pod<T>::value, T>::type* Dummy = 0) const;

    // Write memory (string types)
    template <typename T>
    void WriteString(PVOID Address, T const& Data, typename std::enable_if<
      std::is_same<T, std::basic_string<typename T::value_type>>::value, T>::
      type* Dummy = 0) const;

    // Write memory (vector types)
    template <typename T>
    void WriteList(PVOID Address, T const& Data, typename std::enable_if<std::
      is_same<T, std::vector<typename T::value_type>>::value, T>::type* 
      Dummy = 0) const;

    // Whether an address is currently readable
    bool CanRead(LPCVOID Address) const;

    // Whether an address is currently writable
    bool CanWrite(LPCVOID Address) const;

    // Whether an address is currently executable
    bool CanExecute(LPCVOID Address) const;

    // Whether an address is contained within a guard page
    bool IsGuard(LPCVOID Address) const;
    
    // Protect a memory region
    DWORD ProtectRegion(LPVOID Address, DWORD Protect) const;

    // Allocate memory
    PVOID Alloc(SIZE_T Size) const;

    // Free memory
    void Free(PVOID Address) const;

    // Get address of export in remote process (by name)
    FARPROC GetRemoteProcAddress(HMODULE RemoteMod, 
      std::wstring const& Module, std::string const& Function) const;

    // Get address of export in remote process (by ordinal)
    FARPROC GetRemoteProcAddress(HMODULE RemoteMod, 
      std::wstring const& Module, WORD Function) const;

    // Flush instruction cache
    void FlushCache(LPCVOID Address, SIZE_T Size) const;

    // Get process handle of target
    HANDLE GetProcessHandle() const;

    // Get process ID of target
    DWORD GetProcessId() const;
    
    // Get process path
    std::wstring GetProcessPath() const;
    
    // Is WoW64 process
    bool IsWoW64Process() const;
    
    // Equality operator
    bool operator==(MemoryMgr const& Rhs) const;
    
    // Inequality operator
    bool operator!=(MemoryMgr const& Rhs) const;

  private:
    // Read memory
    void ReadImpl(PVOID Address, PVOID Out, std::size_t OutSize) const;
      
    // Write memory
    void WriteImpl(PVOID Address, LPCVOID In, std::size_t InSize) const;
    
    // Target process
    Detail::Process m_Process;
  };
    
  // Create process
  MemoryMgr CreateProcess(std::wstring const& Path, 
    std::wstring const& Params, 
    std::wstring const& WorkingDir);
  
  // Gets the SeDebugPrivilege
  void GetSeDebugPrivilege();

  // RAII class for remote memory allocation and freeing
  class AllocAndFree
  {
  public:
    AllocAndFree(MemoryMgr const& MyMemoryMgr, SIZE_T Size);

    ~AllocAndFree();
    
    void Free() const;

    PVOID GetBase() const;
    
    SIZE_T GetSize() const;

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
    // Read data
    T Data;
    ReadImpl(Address, &Data, sizeof(Data));
    return Data;
  }

  // Read memory (string types)
  template <typename T>
  T MemoryMgr::ReadString(PVOID Address, typename std::enable_if<std::is_same<
    T, std::basic_string<typename T::value_type>>::value, T>::type* /*Dummy*/) 
    const
  {
    // Character type
    typedef typename T::value_type CharT;

    // Ensure chracter type is POD
    static_assert(std::is_pod<CharT>::value, "Character type of string must "
      "be POD.");
    
    // Create buffer to store results
    T Buffer;

    // Read until a null terminator is found
    CharT* AddressReal = static_cast<CharT*>(Address);
    for (CharT Current = this->Read<CharT>(AddressReal); Current != CharT(); 
      ++AddressReal, Current = this->Read<CharT>(AddressReal))
    {
      Buffer.push_back(Current);
    }
    
    // Return buffer
    return Buffer;
  }

  // Read memory (vector types)
  template <typename T>
  T MemoryMgr::ReadList(PVOID Address, std::size_t Size, typename 
    std::enable_if<std::is_same<T, std::vector<typename T::value_type>>::
    value, T>::type* /*Dummy*/) const
  {
    // Value type
    typedef typename T::value_type ValT;
    
    // Ensure value type is POD
    static_assert(std::is_pod<ValT>::value, "Value type of vector must be "
      "POD.");
    
    // Read data
    T Data(Size);
    this->ReadImpl(Address, Data.data(), sizeof(ValT) * Size);
    return Data;
  }

  // Write memory (POD types)
  template <typename T>
  void MemoryMgr::Write(PVOID Address, T const& Data, typename std::
    enable_if<std::is_pod<T>::value, T>::type* /*Dummy*/) const 
  {
    // Write memory
    WriteImpl(Address, &Data, sizeof(Data));
  }

  // Write memory (string types)
  template <typename T>
  void MemoryMgr::WriteString(PVOID Address, T const& Data, 
    typename std::enable_if<std::is_same<T, std::basic_string<typename T::
    value_type>>::value, T>::type* /*Dummy*/) const
  {
    // Character type
    typedef typename T::value_type CharT;

    // Ensure chracter type is POD
    static_assert(std::is_pod<CharT>::value, "Character type of string must "
      "be POD.");
    
    // Write memory
    std::size_t const RawSize = (Data.size() * sizeof(CharT)) + 1;
    WriteImpl(Address, Data.data(), RawSize);
  }

  // Write memory (vector types)
  template <typename T>
  void MemoryMgr::WriteList(PVOID Address, T const& Data, typename std::
    enable_if<std::is_same<T, std::vector<typename T::value_type>>::value, 
    T>::type* /*Dummy*/) const
  {
    // Value type
    typedef typename T::value_type ValT;
    
    // Ensure value type is POD
    static_assert(std::is_pod<ValT>::value, "Value type of vector must be "
      "POD.");
    
    // Write memory
    std::size_t const RawSize = Data.size() * sizeof(ValT);
    WriteImpl(Address, Data.data(), RawSize);
  }
}
