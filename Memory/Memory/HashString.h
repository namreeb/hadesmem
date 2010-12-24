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

// IMPORTANT NOTE:
// This is a modified version of work by 'Tor Brede Vekterli' who has licensed 
// the original code under the Boost Software license[1]. As such, all 
// compile-time string hashing code is licensed under the Boost Software 
// License.
// [1] http://www.boost.org/LICENSE_1_0.txt
// The GPL notice above is present to cover any other code which is unrelated 
// to the compile-time string hashing.

// Boost
#pragma warning(push, 1)
#include <boost/mpl/string.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/size_t.hpp>
#pragma warning(pop)

namespace Hades
{
  namespace Memory
  {
    namespace Detail
    {
      // Disable 'integral constant overflow' warning.
#pragma warning(push)
#pragma warning(disable: 4307)

      // Perform hashing
      template <typename Seed, typename Value>
      struct HashCombine
      {
        typedef boost::mpl::size_t<Seed::value ^ (Value::value + 0x9e3779b9 + 
          (Seed::value << 6) + (Seed::value >> 2))> type;
      };

#pragma warning(pop)

      // Hash any sequence of integral wrapper types
      template <typename Sequence>
      struct HashSequence
        : boost::mpl::fold<
        Sequence, 
        boost::mpl::size_t<0>, 
        HashCombine<boost::mpl::_1, boost::mpl::_2>
        >::type
      { };
    }

    // Hash WITHOUT terminating null
    template <typename String>
    struct HashString 
      : Detail::HashSequence<String>
    { };

    // Hash WITH terminating null
    template <typename String>
    struct HashCString 
      : Detail::HashCombine<
      Detail::HashSequence<String>, 
      boost::mpl::size_t<0>
      >::type
    { };
  }
}
