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
#include <HadesCommon/Error.hpp>
#include <HadesCommon/Config.hpp>
#include <HadesCommon/StringBuffer.hpp>

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
    if (Source.empty())
    {
      return std::string();
    }

    int const OutSize = WideCharToMultiByte(CP_ACP, 0, Source.c_str(), -1, 
      nullptr, 0, nullptr, nullptr);
    if (!OutSize)
    {
      std::error_code const LastError = Hades::GetLastErrorCode();
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("lexical_cast<std::string, std::wstring>") << 
        Hades::ErrorString("Could not get size of output buffer.") << 
        Hades::ErrorCode(LastError));
    }
    
    std::string Dest;
    int const Result = WideCharToMultiByte(CP_ACP, 0, Source.c_str(), -1, 
      Hades::Util::MakeStringBuffer(Dest, OutSize), OutSize, nullptr, nullptr);
    if (!Result)
    {
      std::error_code const LastError = Hades::GetLastErrorCode();
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("lexical_cast<std::string, std::wstring>") << 
        Hades::ErrorString("Could not convert string.") << 
        Hades::ErrorCode(LastError));
    }
    
    return Dest;
  }
  
  // Boost.LexicalCast specialization to allow conversions from narrow to wide 
  // strings.
  template<> 
  inline std::wstring lexical_cast<std::wstring, std::string>(
    std::string const& Source)
  {
    if (Source.empty())
    {
      return std::wstring();
    }

    int const OutSize = MultiByteToWideChar(CP_ACP, 0, Source.c_str(), -1, 
      nullptr, 0);
    if (!OutSize)
    {
      std::error_code const LastError = Hades::GetLastErrorCode();
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("lexical_cast<std::wstring, std::string>") << 
        Hades::ErrorString("Could not get size of output buffer.") << 
        Hades::ErrorCode(LastError));
    }
    
    std::wstring Dest;
    int const Result = MultiByteToWideChar(CP_ACP, 0, Source.c_str(), -1, 
      Hades::Util::MakeStringBuffer(Dest, OutSize), OutSize);
    if (!Result)
    {
      std::error_code const LastError = Hades::GetLastErrorCode();
      BOOST_THROW_EXCEPTION(Hades::HadesError() << 
        Hades::ErrorFunction("lexical_cast<std::wstring, std::string>") << 
        Hades::ErrorString("Could not convert string.") << 
        Hades::ErrorCode(LastError));
    }
    
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
