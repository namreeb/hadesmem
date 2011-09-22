// Copyright Joshua Boyce 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

// Hades
#include <HadesMemory/Detail/Error.hpp>
#include <HadesMemory/Detail/Config.hpp>
#include <HadesMemory/Detail/StringBuffer.hpp>

// C++ Standard Library
#include <string>

// Boost
#include <boost/lexical_cast.hpp>

namespace boost
{
  // Boost.LexicalCast specialization to allow conversions from wide to narrow 
  // strings.
  template<> 
  inline std::string lexical_cast<std::string, std::wstring>(
    std::wstring const& Source)
  {
    // Empty string optimization
    if (Source.empty())
    {
      return std::string();
    }

    // Caluclate output buffer size
    int const OutSize = WideCharToMultiByte(CP_ACP, 0, Source.c_str(), -1, 
      nullptr, 0, nullptr, nullptr);
    if (!OutSize)
    {
      DWORD const LastError = GetLastError();
      BOOST_THROW_EXCEPTION(HadesMem::HadesMemError() << 
        HadesMem::ErrorFunction("lexical_cast<std::string, std::wstring>") << 
        HadesMem::ErrorString("Could not get size of output buffer.") << 
        HadesMem::ErrorCodeWinLast(LastError));
    }
    
    // Convert wide string to narrow string
    std::string Dest;
    int const Result = WideCharToMultiByte(CP_ACP, 0, Source.c_str(), -1, 
      HadesMem::Detail::MakeStringBuffer(Dest, OutSize), OutSize, nullptr, 
      nullptr);
    if (!Result)
    {
      DWORD const LastError = GetLastError();
      BOOST_THROW_EXCEPTION(HadesMem::HadesMemError() << 
        HadesMem::ErrorFunction("lexical_cast<std::string, std::wstring>") << 
        HadesMem::ErrorString("Could not convert string.") << 
        HadesMem::ErrorCodeWinLast(LastError));
    }
    
    // Return new string
    return Dest;
  }
  
  // Boost.LexicalCast specialization to allow conversions from narrow to wide 
  // strings.
  template<> 
  inline std::wstring lexical_cast<std::wstring, std::string>(
    std::string const& Source)
  {
    // Empty string optimization
    if (Source.empty())
    {
      return std::wstring();
    }

    // Caluclate output buffer size
    int const OutSize = MultiByteToWideChar(CP_ACP, 0, Source.c_str(), -1, 
      nullptr, 0);
    if (!OutSize)
    {
      DWORD const LastError = GetLastError();
      BOOST_THROW_EXCEPTION(HadesMem::HadesMemError() << 
        HadesMem::ErrorFunction("lexical_cast<std::wstring, std::string>") << 
        HadesMem::ErrorString("Could not get size of output buffer.") << 
        HadesMem::ErrorCodeWinLast(LastError));
    }
    
    // Convert narrow string to wide string
    std::wstring Dest;
    int const Result = MultiByteToWideChar(CP_ACP, 0, Source.c_str(), -1, 
      HadesMem::Detail::MakeStringBuffer(Dest, OutSize), OutSize);
    if (!Result)
    {
      DWORD const LastError = GetLastError();
      BOOST_THROW_EXCEPTION(HadesMem::HadesMemError() << 
        HadesMem::ErrorFunction("lexical_cast<std::wstring, std::string>") << 
        HadesMem::ErrorString("Could not convert string.") << 
        HadesMem::ErrorCodeWinLast(LastError));
    }
    
    // Return new string
    return Dest;
  }
  
  // Turn attempts to convert between the same string types into a nullsub.
  template<> 
  inline std::wstring lexical_cast<std::wstring, std::wstring>(
    std::wstring const& Source)
  {
    return Source;
  }
  
  // Turn attempts to convert between the same string types into a nullsub.
  template<> 
  inline std::string lexical_cast<std::string, std::string>(
    std::string const& Source)
  {
    return Source;
  }
}
