// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <cstddef>

#include <type_traits>

#include <windows.h>

namespace hadesmem
{

class Process;

bool CanRead(Process const& process, LPCVOID address);

bool CanWrite(Process const& process, LPCVOID address);

bool CanExecute(Process const& process, LPCVOID address);

bool IsGuard(Process const& process, LPCVOID address);

PVOID Alloc(Process const& process, SIZE_T size);

void Free(Process const& process, LPVOID address);

void FlushInstructionCache(Process const& process, LPCVOID address, SIZE_T size);

DWORD Protect(Process const& process, LPVOID address, DWORD protect);

template <typename T>
T Read(Process const& process, PVOID address)
{
  static_assert(std::is_pod<T>::value, "Read: T must be POD.");
  
  T data;
  detail::Read(process, address, &data, sizeof(data));
  return data;
}

template <typename T>
T ReadString(Process const& process, PVOID address)
{
  typedef typename T::value_type CharT;
  typedef typename T::traits_type TraitsT;
  typedef typename T::allocator_type AllocT;
  
  static_assert(std::is_same<T, std::basic_string<CharT, TraitsT, 
    AllocT>>::value, "ReadString: T must be of type std::basic_string.");

  static_assert(std::is_pod<CharT>::value, "ReadString: Character type of "
    "string must be POD.");
  
  T data;

  CharT* address_real = static_cast<CharT*>(address);
  for (CharT current = Read<CharT>(process, address_real); 
    current != CharT(); 
    ++address_real, current = Read<CharT>(process, address_real))
  {
    data.push_back(current);
  }
  
  return data;
}

template <typename T>
T ReadList(Process const& process, PVOID address, std::size_t size)
{
  typedef typename T::value_type ValueT;
  typedef typename T::allocator_type AllocT;
  
  static_assert(std::is_same<T, std::vector<ValueT, AllocT>>::value, 
    "ReadList: T must be of type std::vector.");
  
  static_assert(std::is_pod<ValueT>::value, "ReadList: Value type of vector "
    "must be POD.");
  
  T data(size);
  detail::Read(process, address, data.data(), sizeof(ValueT) * size);
  return data;
}

template <typename T>
void Write(Process const& process, PVOID address, T const& data)
{
  static_assert(std::is_pod<T>::value, "Write: T must be POD.");
  
  detail::Write(process, address, &data, sizeof(data));
}

template <typename T>
void WriteString(Process const& process, PVOID address, T const& data)
{
  typedef typename T::value_type CharT;
  typedef typename T::traits_type TraitsT;
  typedef typename T::allocator_type AllocT;
  
  static_assert(std::is_same<T, std::basic_string<CharT, TraitsT, 
    AllocT>>::value, "WriteString: T must be of type std::basic_string.");
  
  static_assert(std::is_pod<CharT>::value, "WriteString: Character type of "
    "string must be POD.");
  
  std::size_t const raw_size = (data.size() * sizeof(CharT)) + 1;
  detail::Write(process, address, data.data(), raw_size);
}

template <typename T>
void WriteList(Process const& process, PVOID address, T const& data)
{
  typedef typename T::value_type ValueT;
  typedef typename T::allocator_type AllocT;
  
  static_assert(std::is_same<T, std::vector<ValueT, AllocT>>::value, 
    "WriteList: T must be of type std::vector.");
  
  static_assert(std::is_pod<ValueT>::value, "WriteList: Value type of vector "
    "must be POD.");
  
  std::size_t const raw_size = data.size() * sizeof(ValueT);
  detail::Write(process, address, data.data(), raw_size);
}

namespace detail
{

MEMORY_BASIC_INFORMATION Query(Process const& process, LPCVOID address);

void Read(Process const& process, LPVOID address, LPVOID out, std::size_t out_size);

void Write(Process const& process, PVOID address, LPCVOID in, std::size_t in_size);

}

}
