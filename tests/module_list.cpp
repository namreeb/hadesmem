// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/module_list.hpp>

#include <utility>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/concept_check.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/process.hpp>

void TestModuleList()
{
    BOOST_CONCEPT_ASSERT((boost::InputIterator<hadesmem::ModuleList::iterator>));
    BOOST_CONCEPT_ASSERT((boost::InputIterator<hadesmem::ModuleList::
        const_iterator>));

    hadesmem::Process const process(::GetCurrentProcessId());

    hadesmem::ModuleList const module_list_1(process);
    hadesmem::ModuleList module_list_2(module_list_1);
    hadesmem::ModuleList module_list_3(std::move(module_list_2));
    module_list_2 = std::move(module_list_3);
    BOOST_TEST(std::begin(module_list_2) != std::end(module_list_2));

    auto iter = std::begin(module_list_1);
    hadesmem::Module const this_mod(process, nullptr);
    BOOST_TEST(iter != std::end(module_list_1));
    BOOST_TEST_EQ(*iter, this_mod);
    hadesmem::Module const ntdll_mod(process, L"NtDll.DlL");
    BOOST_TEST(++iter != std::end(module_list_1));
    BOOST_TEST_EQ(*iter, ntdll_mod);
}

void TestModuleListAlgorithm()
{
    hadesmem::Process const process(::GetCurrentProcessId());

    hadesmem::ModuleList const module_list_1(process);

    for (auto const& module : module_list_1)
    {
        BOOST_TEST_NE(module.GetHandle(), static_cast<void*>(nullptr));
        BOOST_TEST_NE(module.GetSize(), 0U);
        BOOST_TEST(!module.GetName().empty());
        BOOST_TEST(!module.GetPath().empty());
    }

    auto const user32_iter = std::find_if(
        std::begin(module_list_1),
        std::end(module_list_1),
        [](hadesmem::Module const& module)
    {
        return module.GetHandle() == ::GetModuleHandle(L"user32.dll");
    });
    BOOST_TEST(user32_iter != std::end(module_list_1));
}

int main()
{
    TestModuleList();
    TestModuleListAlgorithm();
    return boost::report_errors();
}
