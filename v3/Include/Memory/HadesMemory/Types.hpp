/*
This file is part of HadesMem.
Copyright (C) 2010 Joshua Boyce (aka RaptorFactor, Cypherjb, Cypher, Chazwazza).
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
#include <cstdint>

namespace Hades
{
  namespace Memory
  {
    namespace Types
    {
      // Declare fixed-size types
      typedef std::int8_t     Int8;
      typedef std::uint8_t    UInt8;
      typedef std::int16_t    Int16;
      typedef std::uint16_t   UInt16;
      typedef std::int32_t    Int32;
      typedef std::uint32_t   UInt32;
      typedef std::int64_t    Int64;
      typedef std::uint64_t   UInt64;
      typedef float           Float;
      typedef double          Double;
      typedef char            CharA;
      typedef wchar_t         CharW;
      typedef std::string     StringA;
      typedef std::wstring    StringW;
      typedef void*           Pointer;

      // Ensure data type are correct
      static_assert(sizeof(Int8) == 1, "Size of Int8 is wrong.");
      static_assert(sizeof(UInt8) == 1, "Size of UInt8 is wrong.");
      static_assert(sizeof(Int16) == 2, "Size of Int16 is wrong.");
      static_assert(sizeof(UInt16) == 2, "Size of UInt16 is wrong.");
      static_assert(sizeof(Int32) == 4, "Size of Int32 is wrong.");
      static_assert(sizeof(UInt32) == 4, "Size of UInt32 is wrong.");
      static_assert(sizeof(Int64) == 8, "Size of Int64 is wrong.");
      static_assert(sizeof(UInt64) == 8, "Size of UInt64 is wrong.");
      static_assert(sizeof(Float) == 4, "Size of Float is wrong.");
      static_assert(sizeof(Double) == 8, "Size of Double is wrong.");
    }
  }
}
