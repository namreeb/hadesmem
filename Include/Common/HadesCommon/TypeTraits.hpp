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
#include <type_traits>

namespace Hades
{
  namespace Util
  {
    template <typename... Ts>
    struct all_pod;
    
    template <typename Head, typename... Tail>
    struct all_pod<Head, Tail...>
    {
      static const bool value = std::is_pod<Head>::value && 
        all_pod<Tail...>::value;
    };
    
    template <typename T>
    struct all_pod<T>
    {
      static const bool value = std::is_pod<T>::value;
    };
    
    template <typename... Ts>
    struct all_memsize_or_less;
    
    template <typename Head, typename... Tail>
    struct all_memsize_or_less<Head, Tail...>
    {
      static const bool value = (sizeof(Head) <= sizeof(void*)) && 
        all_memsize_or_less<Tail...>::value;
    };
    
    template <typename T>
    struct all_memsize_or_less<T>
    {
      static const bool value = sizeof(T) <= sizeof(void*);
    };
  }
}
