// Copyright Joshua Boyce 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

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
      
    // Copy constructor
    MemoryMgr(MemoryMgr const& Other);
    
    // Copy assignment operator
    MemoryMgr& operator=(MemoryMgr const& Other);
    
    // Move constructor
    MemoryMgr(MemoryMgr&& Other);
    
    // Move assignment operator
    MemoryMgr& operator=(MemoryMgr&& Other);
    
    // Destructor
    ~MemoryMgr();

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
    T Read(PVOID Address) const;

    // Read memory (string types)
    template <typename T>
    T ReadString(PVOID Address) const;

    // Read memory (vector types)
    template <typename T>
    T ReadList(PVOID Address, std::size_t Size) const;
      
    // Write memory (POD types)
    template <typename T>
    void Write(PVOID Address, T const& Data) const;

    // Write memory (string types)
    template <typename T>
    void WriteString(PVOID Address, T const& Data) const;

    // Write memory (vector types)
    template <typename T>
    void WriteList(PVOID Address, T const& Data) const;

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
    std::wstring const& CommandLine, 
    std::wstring const& WorkingDir);
    
  // Create process
  MemoryMgr CreateProcess(std::wstring const& Path, 
    std::vector<std::wstring> const& Args, 
    std::wstring const& WorkingDir);
  
  // Gets the SeDebugPrivilege
  void GetSeDebugPrivilege();

  // RAII class for remote memory allocation and freeing
  class AllocAndFree
  {
  public:
    AllocAndFree(MemoryMgr const& MyMemoryMgr, SIZE_T Size);
    
    AllocAndFree(AllocAndFree&& Other);
    
    AllocAndFree& operator=(AllocAndFree&& Other);

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
  T MemoryMgr::Read(PVOID Address) const
  {
    // Ensure T is POD
    static_assert(std::is_pod<T>::value, "MemoryMgr::Read: T must be POD.");
    
    // Read data
    T Data;
    ReadImpl(Address, &Data, sizeof(Data));
    return Data;
  }

  // Read memory (string types)
  template <typename T>
  T MemoryMgr::ReadString(PVOID Address) const
  {
    typedef typename T::value_type CharT;
    typedef typename T::traits_type TraitsT;
    typedef typename T::allocator_type AllocT;
    
    // Ensure type is a string
    static_assert(std::is_same<T, std::basic_string<CharT, TraitsT, 
      AllocT>>::value, "MemoryMgr::ReadString: T must be of type "
      "std::basic_string.");

    // Ensure chracter type is POD
    static_assert(std::is_pod<CharT>::value, "MemoryMgr::ReadString: "
      "Character type of string must be POD.");
    
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
  T MemoryMgr::ReadList(PVOID Address, std::size_t Size) const
  {
    typedef typename T::value_type ValueT;
    typedef typename T::allocator_type AllocT;
    
    // Ensure type is a string
    static_assert(std::is_same<T, std::vector<ValueT, AllocT>>::value, 
      "MemoryMgr::ReadList: T must be of type std::vector.");

    // Ensure value type is POD
    static_assert(std::is_pod<ValueT>::value, "MemoryMgr::ReadList: Value "
      "type of vector must be POD.");
    
    // Read data
    T Data(Size);
    this->ReadImpl(Address, Data.data(), sizeof(ValueT) * Size);
    return Data;
  }

  // Write memory (POD types)
  template <typename T>
  void MemoryMgr::Write(PVOID Address, T const& Data) const 
  {
    // Ensure T is POD
    static_assert(std::is_pod<T>::value, "MemoryMgr::Write: T must be POD.");
    
    // Write memory
    WriteImpl(Address, &Data, sizeof(Data));
  }

  // Write memory (string types)
  template <typename T>
  void MemoryMgr::WriteString(PVOID Address, T const& Data) const
  {
    typedef typename T::value_type CharT;
    typedef typename T::traits_type TraitsT;
    typedef typename T::allocator_type AllocT;
    
    // Ensure type is a string
    static_assert(std::is_same<T, std::basic_string<CharT, TraitsT, 
      AllocT>>::value, "MemoryMgr::WriteString: T must be of type "
      "std::basic_string.");

    // Ensure chracter type is POD
    static_assert(std::is_pod<CharT>::value, "MemoryMgr::WriteString: "
      "Character type of string must be POD.");
    
    // Write memory
    std::size_t const RawSize = (Data.size() * sizeof(CharT)) + 1;
    WriteImpl(Address, Data.data(), RawSize);
  }

  // Write memory (vector types)
  template <typename T>
  void MemoryMgr::WriteList(PVOID Address, T const& Data) const
  {
    typedef typename T::value_type ValueT;
    typedef typename T::allocator_type AllocT;
    
    // Ensure type is a string
    static_assert(std::is_same<T, std::vector<ValueT, AllocT>>::value, 
      "MemoryMgr::WriteList: T must be of type std::vector.");

    // Ensure value type is POD
    static_assert(std::is_pod<ValueT>::value, "MemoryMgr::WriteList: Value "
      "type of vector must be POD.");
    
    // Write memory
    std::size_t const RawSize = Data.size() * sizeof(ValueT);
    WriteImpl(Address, Data.data(), RawSize);
  }
}
