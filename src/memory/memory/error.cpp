// Copyright (C) 2010-2012 Joshua Boyce.
// See the file COPYING for copying permission.

#include "hadesmem/error.hpp"

namespace hadesmem
{

Error::Error()
  : what_()
{
  what_ += "Exception type: ";
  what_ += typeid(*this).name();
}

char const* Error::what() const HADESMEM_NOEXCEPT
{
  return what_.c_str();
}

}
