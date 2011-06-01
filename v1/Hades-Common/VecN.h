/*
This file is part of HadesMem.
Copyright © 2010 Cypherjb (aka Chazwazza, aka Cypher). 
<http://www.cypherjb.com/> <cypher.jb@gmail.com>

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
#include <array>
#include <cmath>
#include <vector>
#include <numeric>
#include <cassert>
#include <iterator>
#include <stdexcept>
#include <algorithm>
#include <functional>

namespace Hades
{
  namespace Math
  {
    // N-Dimensional Vector
    template <class ElemT, unsigned int Num>
    class VecN
    {
    public:
      // ElemT used to store elements
      typedef std::tr1::array<ElemT, Num> DataT;    

      // Element placeholders. For convenience only.
      enum Elems
      {
        XElem,
        YElem,
        ZElem
      };

      // Default constructor. 
      // All dimensions default initialized.
      VecN() 
        : m_Data() 
      {
        std::fill(m_Data.begin(), m_Data.end(), static_cast<ElemT>(0)); 
      }

      // Vec1 constructor
      VecN(ElemT X) 
        : m_Data()
      {
        static_assert(Num == 1, "Wrong constructor used for VecN");
        m_Data[XElem] = X;
      }

      // Vec2 constructor
      VecN(ElemT X, ElemT Y) 
        : m_Data()
      {
        static_assert(Num == 2, "Wrong constructor used for VecN");
        m_Data[XElem] = X; m_Data[YElem] = Y;
      }

      // Vec3 constructor
      VecN(ElemT X, ElemT Y, ElemT Z) 
        : m_Data()
      {
        static_assert(Num == 3, "Wrong constructor used for VecN");
        m_Data[XElem] = X; m_Data[YElem] = Y; m_Data[ZElem] = Z;
      }

      // Vec4 constructor
      VecN(ElemT X, ElemT Y, ElemT Z, ElemT N) 
        : m_Data()
      {
        static_assert(Num == 4, "Wrong constructor used for VecN");
        m_Data[XElem] = X; m_Data[YElem] = Y; m_Data[ZElem] = Z; m_Data[3] = N;
      }

      // Get reference to vector element
      // Implemented by calling const version of function and const_cast'ing 
      // the returned reference. Safe in this scenario. For more information 
      // see [Myers].
      ElemT& operator[] (unsigned int Index)
      {
        assert(Index < Num && "Out of range element access in VecN");
        return const_cast<ElemT&>(static_cast<const VecN<ElemT, Num>& >(*this).
          operator[](Index));
      }

      // Get const reference to vector element
      const ElemT& operator[] (unsigned int Index) const
      {
        assert(Index < Num && "Out of range element access in VecN");
        return m_Data[Index]; 
      }

      // Get negated vector
      VecN<ElemT, Num> operator- () const
      {
        VecN<ElemT, Num> Result;
        std::transform(m_Data.begin(), m_Data.end(), Result.m_Data.begin(), 
          std::negate<ElemT>());
        return Result;
      }

      // Vector subtraction
      VecN<ElemT, Num> operator- (const VecN<ElemT, Num>& Rhs) const
      {
        VecN<ElemT, Num> Result(*this);
        Result -= Rhs;
        return Result;
      }

      // Vector subtraction
      VecN<ElemT, Num> operator-= (const VecN<ElemT, Num>& Rhs) 
      {
        std::transform(m_Data.begin(), m_Data.end(), Rhs.m_Data.begin(), 
          m_Data.begin(), std::minus<ElemT>());
        return *this;
      }

      // Vector addition
      VecN<ElemT, Num> operator+ (const VecN<ElemT, Num>& Rhs) const
      {
        VecN<ElemT, Num> Result(*this);
        Result += Rhs;
        return Result;
      }

      // Vector addition
      VecN<ElemT, Num> operator+= (const VecN<ElemT, Num>& Rhs) 
      {
        std::transform(m_Data.begin(), m_Data.end(), Rhs.m_Data.begin(), 
          m_Data.begin(), std::plus<ElemT>());
        return *this;
      }

      // Vector multiplication (scales elements)
      VecN<ElemT, Num> operator* (ElemT Rhs) const
      {
        VecN<ElemT, Num> Result(*this);
        Result *= Rhs;
        return Result;
      }

      // Vector multiplication (scales elements)
      // Mixed mode version
      friend inline VecN<ElemT, Num> operator* (ElemT Lhs, 
        const VecN<ElemT, Num>& Rhs)
      {
        return Rhs * Lhs;
      }

      // Vector multiplication (scalar/dot product)
      ElemT operator* (const VecN<ElemT, Num>& Rhs) const
      {
        DataT Result(Num);
        std::transform(m_Data.begin(), m_Data.end(), Rhs.m_Data.begin(), 
          Result.begin(), std::multiplies<ElemT>());
        return std::accumulate(Result.begin(), Result.end(), 
          static_cast<ElemT>(0));
      }

      // Vector multiplication (scales elements)
      VecN<ElemT, Num> operator*= (ElemT Rhs) 
      {
        std::transform(m_Data.begin(), m_Data.end(), m_Data.begin(), 
          std::bind(std::multiplies<ElemT>(), _1, Rhs));
        return *this;
      }

      // Vector division (scalar/dot division)
      ElemT operator/ (const VecN<ElemT, Num>& Rhs) const
      {
        DataT Result(Num);
        std::transform(m_Data.begin(), m_Data.end(), Rhs.m_Data.begin(), 
          Result.begin(), std::divides<ElemT>());
        return std::accumulate(Result.begin(), Result.end(), 
          static_cast<ElemT>(0));
      }

      // Vector division (scalar/dot division)
      VecN<ElemT, Num> operator/= (const VecN<ElemT, Num>& Rhs) 
      {
        std::transform(m_Data.begin(), m_Data.end(), Rhs.m_Data.begin(), 
          m_Data.begin(), std::divides<ElemT>());
        return *this;
      }

      // Equality test
      bool operator == (const VecN<ElemT, Num>& Rhs) const
      {
        return std::equal(m_Data.begin(), m_Data.end(), Rhs.m_Data.begin());
      }

      // Inequality check
      bool operator != (const VecN<ElemT, Num>& Rhs) const
      {
        return !std::equal(m_Data.begin(), m_Data.end(), Rhs.m_Data.begin());
      }

      // Output stream overload
      // Prints in format:
      // E1 E2 ... EN
      friend inline std::ostream& operator << (std::ostream& Lhs, 
        const VecN<ElemT, Num>& Rhs)
      {
        std::copy(Rhs.m_Data.begin(), Rhs.m_Data.end(), 
          std::ostream_iterator<ElemT>(Lhs, " "));
        return Lhs;
      }

      // Output stream overload
      // Prints in format:
      // E1 E2 ... EN
      friend inline std::wostream& operator << (std::wostream& Lhs, 
        const VecN<ElemT, Num>& Rhs)
      {
        std::copy(Rhs.m_Data.begin(), Rhs.m_Data.end(), 
          std::ostream_iterator<ElemT, wchar_t>(Lhs, L" "));
        return Lhs;
      }

      // Gets the squared length of a vector
      friend inline ElemT LengthSquared(const VecN<ElemT, Num>& v)
      {
        VecN<ElemT, Num> Temp;
        std::transform(v.m_Data.begin(), v.m_Data.end(), v.m_Data.begin(), 
          Temp.m_Data.begin(), std::multiplies<ElemT>());
        return std::accumulate(Temp.m_Data.begin(), Temp.m_Data.end(), 
          static_cast<ElemT>(0));
      }

      // Gets the length of a vector
      friend inline ElemT Length(const VecN<ElemT, Num>& v)
      {
        return static_cast<ElemT>(sqrt(static_cast<long double>(
          LengthSquared(v))));
      }

      // Normalizes a vector
      friend inline ElemT Normalize(VecN<ElemT, Num>& v)
      {
        ElemT Len = Length(v);
        if (Len != static_cast<ElemT>(0))
        {
          std::transform(v.m_Data.begin(), v.m_Data.end(), v.m_Data.begin(), 
            std::bind(std::divides<ElemT>(), _1, Len));
        }
        return Len;
      }

      // Tests if a vector is normalized
      friend inline bool IsNormalized(const VecN<ElemT, Num>& v, 
        ElemT Eps = static_cast<ElemT>(1.0001f))
      {
        return static_cast<ElemT>(fabs(static_cast<long double>(
          LengthSquared(v) - static_cast<ElemT>(1)))) 
          <= static_cast<ElemT>(Eps);
      }

      // Reflection
      friend inline VecN<ElemT, Num> Reflect(const VecN<ElemT, Num>& v, 
        const VecN<ElemT, Num>& Normal)
      {
        return v - (static_cast<ElemT>(2.0) * ((v * Normal) * Normal));
      }

      // Distance
      friend inline ElemT Distance(const VecN<ElemT, Num>& v1, 
        const VecN<ElemT, Num>& v2)
      {
        return Length(v1 - v2);
      }

    private:
      // Vector elements
      DataT m_Data;
    };

    // Cross product
    template <typename ElemT>
    inline VecN<ElemT, 3> Cross(const VecN<ElemT, 3>& v1, 
      const VecN<ElemT, 3>& v2)
    {
      typedef VecN<ElemT, 3> VecT;
      return VecT(
        (v1[VecT::YElem] * v2[VecT::ZElem]) - 
        (v1[VecT::ZElem] * v2[VecT::YElem]), 
        (v1[VecT::ZElem] * v2[VecT::XElem]) - 
        (v1[VecT::XElem] * v2[VecT::ZElem]), 
        (v1[VecT::XElem] * v2[VecT::YElem]) - 
        (v1[VecT::YElem] * v2[VecT::XElem])
        );
    }

    // Ease of use typedefs
    typedef VecN<int, 2> Vec2i;
    typedef VecN<float, 2> Vec2f;
    typedef VecN<double, 2> Vec2d;
    typedef VecN<int, 3> Vec3i;
    typedef VecN<float, 3> Vec3f;
    typedef VecN<double, 3> Vec3d;

    // Assert assumptions
    static_assert(sizeof(Vec2f) == (sizeof(float) * 2), 
      "Size of Vec2f is incorrect!");
    static_assert(sizeof(Vec2d) == (sizeof(double) * 2), 
      "Size of Vec2d is incorrect!");
    static_assert(sizeof(Vec2i) == (sizeof(int) * 2), 
      "Size of Vec2i is incorrect!");

    static_assert(sizeof(Vec3f) == (sizeof(float) * 3), 
      "Size of Vec3f is incorrect!");
    static_assert(sizeof(Vec3d) == (sizeof(double) * 3), 
      "Size of Vec3d is incorrect!");
    static_assert(sizeof(Vec3i) == (sizeof(int) * 3), 
      "Size of Vec3i is incorrect!");
  }
}
