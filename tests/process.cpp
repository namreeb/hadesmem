#include "hadesmem/process.hpp"

#define BOOST_TEST_MODULE process
#if defined(HADES_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif // #if defined(HADES_GCC)
#if defined(HADES_INTEL)
#pragma warning(push, 1)
#pragma warning(disable: 177 367 869 1879)
#endif // #if defined(HADES_INTEL)
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#if defined(HADES_GCC)
#pragma GCC diagnostic pop
#endif // #if defined(HADES_GCC)
#if defined(HADES_INTEL)
#pragma warning(pop)
#endif // #if defined(HADES_INTEL)

// Boost.Test causes the following warning under GCC:
// error: base class 'struct boost::unit_test::ut_detail::nil_t' has a 
// non-virtual destructor [-Werror=effc++]
#if defined(HADES_GCC)
#pragma GCC diagnostic ignored "-Weffc++"
#endif // #if defined(HADES_GCC)

BOOST_AUTO_TEST_CASE(this_process)
{
  hadesmem::Process const process(GetCurrentProcessId());
  BOOST_CHECK_EQUAL(process.GetId(), GetCurrentProcessId());
  std::string const path(hadesmem::GetPath(process));
  BOOST_CHECK(!path.empty());
  BOOST_CHECK(boost::filesystem::exists(path));
  BOOL is_wow64_real = FALSE;
  BOOST_CHECK(IsWow64Process(GetCurrentProcess(), &is_wow64_real));
  BOOST_CHECK_EQUAL(hadesmem::IsWoW64(process), is_wow64_real != FALSE); 
}
