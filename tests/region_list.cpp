// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/region_list.hpp>

#include <utility>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/region.hpp>

void TestRegionList()
{
    using RegionListIterCat =
        std::iterator_traits<hadesmem::RegionList::iterator>::
        iterator_category;
    HADESMEM_DETAIL_STATIC_ASSERT(std::is_base_of<std::input_iterator_tag,
        RegionListIterCat>::value);
    using RegionListConstIterCat =
        std::iterator_traits<hadesmem::RegionList::const_iterator>::
        iterator_category;
    HADESMEM_DETAIL_STATIC_ASSERT(std::is_base_of<std::input_iterator_tag,
        RegionListConstIterCat>::value);

    hadesmem::Process const process(::GetCurrentProcessId());

    hadesmem::RegionList const region_list_1(process);
    hadesmem::RegionList region_list_2(region_list_1);
    hadesmem::RegionList region_list_3(std::move(region_list_2));
    region_list_2 = std::move(region_list_3);
    BOOST_TEST(std::begin(region_list_2) != std::end(region_list_2));

    auto iter = std::begin(region_list_1);
    hadesmem::Region const first_region(process, nullptr);
    BOOST_TEST(iter != std::end(region_list_1));
    BOOST_TEST_EQ(*iter, first_region);
    hadesmem::Region const second_region(process,
        static_cast<char const* const>(first_region.GetBase()) +
        first_region.GetSize());
    BOOST_TEST(++iter != std::end(region_list_1));
    BOOST_TEST_EQ(*iter, second_region);
    hadesmem::Region last(process, nullptr);
    do
    {
        hadesmem::Region current = *iter;
        BOOST_TEST(current > last);
        BOOST_TEST(current >= last);
        BOOST_TEST(last < current);
        BOOST_TEST(last <= current);
        last = current;
    } while (++iter != std::end(region_list_1));

    BOOST_TEST_THROWS(hadesmem::Region(process, static_cast<char const* const>(
        last.GetBase()) + last.GetSize()), hadesmem::Error);
}

void TestRegionListAlgorithm()
{
    hadesmem::Process const process(::GetCurrentProcessId());

    hadesmem::RegionList const region_list_1(process);

    for (auto const& region : region_list_1)
    {
        hadesmem::Region const other(process, region.GetBase());
        BOOST_TEST_EQ(region, other);

        if (region.GetState() != MEM_FREE)
        {
            BOOST_TEST_NE(region.GetBase(), static_cast<void*>(nullptr));
            BOOST_TEST_NE(region.GetAllocBase(), static_cast<void*>(nullptr));
            BOOST_TEST_NE(region.GetAllocProtect(), 0U);
            BOOST_TEST_NE(region.GetType(), 0U);
        }

        region.GetProtect();

        BOOST_TEST_NE(region.GetSize(), 0U);
        BOOST_TEST_NE(region.GetState(), 0U);
    }

    HMODULE const user32_mod = GetModuleHandle(L"user32.dll");
    auto user32_iter = std::find_if(
        std::begin(region_list_1),
        std::end(region_list_1),
        [user32_mod](hadesmem::Region const& region)
    {
        return region.GetBase() == user32_mod;
    });
    BOOST_TEST(user32_iter != std::end(region_list_1));
}

int main()
{
    TestRegionList();
    TestRegionListAlgorithm();
    return boost::report_errors();
}
