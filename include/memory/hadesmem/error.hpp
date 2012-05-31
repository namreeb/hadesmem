#pragma once

#include <exception>

#if defined(HADESMEM_MSVC)
#pragma warning(push, 1)
#endif // #if defined(HADESMEM_MSVC)
#include <boost/exception/all.hpp>
#if defined(HADESMEM_MSVC)
#pragma warning(pop)
#endif // #if defined(HADESMEM_MSVC)

namespace hadesmem
{

class HadesMemError : public virtual std::exception, 
  public virtual boost::exception
{ };

typedef boost::error_info<struct TagErrorFunc, std::string> ErrorFunction;
typedef boost::error_info<struct TagErrorString, std::string> ErrorString;
typedef boost::error_info<struct TagErrorCodeWinRet, DWORD_PTR> 
  ErrorCodeWinRet;
typedef boost::error_info<struct TagErrorCodeWinLast, DWORD> ErrorCodeWinLast;
typedef boost::error_info<struct TagErrorCodeWinOther, DWORD_PTR> 
  ErrorCodeWinOther;
typedef boost::error_info<struct TagErrorCodeOther, DWORD_PTR> ErrorCodeOther;

}
