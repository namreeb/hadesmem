// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#include <hadesmem/read.hpp>

#include <array>
#include <cstring>
#include <string>
#include <vector>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/alloc.hpp>
#include <hadesmem/config.hpp>
#include <hadesmem/detail/winapi.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>

void TestReadPod()
{
    hadesmem::Process const process(::GetCurrentProcessId());

    struct TestPODType
    {
        std::int32_t a;
        char* b;
        wchar_t c;
        std::int64_t d;
    };

    TestPODType test_pod_type = { 1, 0, L'a', 1234567812345678 };
    auto new_test_pod_type = hadesmem::Read<TestPODType>(
        process,
        &test_pod_type);
    BOOST_TEST_EQ(std::memcmp(
        &test_pod_type, 
        &new_test_pod_type,
        sizeof(test_pod_type)), 0);

    auto const new_test_array = 
        hadesmem::Read<std::array<char, sizeof(TestPODType)>>(
        process,
        &test_pod_type);
    BOOST_TEST_EQ(std::memcmp(
        &test_pod_type, 
        &new_test_array[0],
        sizeof(test_pod_type)), 0);

    auto const new_test_array_2 =
        hadesmem::Read<char, sizeof(TestPODType)>(
        process,
        &test_pod_type);
    BOOST_TEST_EQ(std::memcmp(
        &test_pod_type, 
        &new_test_array_2[0],
        sizeof(test_pod_type)), 0);

    PVOID const noaccess_page = VirtualAlloc(
        nullptr, 
        sizeof(void*),
        MEM_RESERVE | MEM_COMMIT, 
        PAGE_NOACCESS);
    BOOST_TEST(noaccess_page != nullptr);
    hadesmem::Read<void*>(process, noaccess_page);

    PVOID const guard_page = VirtualAlloc(
        nullptr, 
        sizeof(void*),
        MEM_RESERVE | MEM_COMMIT, 
        PAGE_EXECUTE_READWRITE | PAGE_GUARD);
    BOOST_TEST(guard_page != nullptr);
    BOOST_TEST_THROWS(hadesmem::Read<void*>(process, guard_page),
        hadesmem::Error);

    PVOID const execute_page = VirtualAlloc(
        nullptr, 
        sizeof(void*),
        MEM_RESERVE | MEM_COMMIT, 
        PAGE_EXECUTE);
    BOOST_TEST(execute_page != nullptr);
    hadesmem::Read<void*>(process, execute_page);
}

void TestReadString()
{
    hadesmem::Process const process(::GetCurrentProcessId());

    hadesmem::Allocator const str_alloc(process, 0x1000);
    char* const str_mem = static_cast<char*>(str_alloc.GetBase());
    wchar_t* const str_mem_wide = static_cast<wchar_t*>(str_alloc.GetBase());

    std::string test_string = "Narrow test string.";
    std::copy(std::begin(test_string), std::end(test_string), str_mem);
    str_mem[test_string.size()] = '\0';
    auto const new_test_string = hadesmem::ReadString<char>(process, str_mem);
    BOOST_TEST_EQ(new_test_string, test_string);

    std::wstring wide_test_string = L"Wide test string.";
    std::copy(
        std::begin(wide_test_string), 
        std::end(wide_test_string),
        str_mem_wide);
    str_mem[wide_test_string.size()] = L'\0';
    auto const wide_new_test_string = hadesmem::ReadString<wchar_t>(
        process,
        str_mem_wide);
    BOOST_TEST(wide_new_test_string == wide_test_string);

    std::string test_string_2 = "Narrow test string.";
    std::copy(std::begin(test_string_2), std::end(test_string_2), str_mem);
    str_mem[test_string_2.size()] = '\0';
    auto const new_test_string_2 = hadesmem::ReadStringEx<char>(
        process, 
        str_mem, 1);
    BOOST_TEST_EQ(new_test_string_2, test_string_2);

    std::wstring wide_test_string_2 = L"Wide test string.";
    std::copy(
        std::begin(wide_test_string_2), 
        std::end(wide_test_string_2),
        str_mem_wide);
    str_mem[wide_test_string_2.size()] = L'\0';
    auto const wide_new_test_string_2 = hadesmem::ReadStringEx<wchar_t>(
        process, 
        str_mem_wide, 
        1);
    BOOST_TEST(wide_new_test_string_2 == wide_test_string_2);
}

void TestReadVector()
{
    hadesmem::Process const process(::GetCurrentProcessId());

    std::vector<int> int_list = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    std::vector<int> int_list_read = hadesmem::ReadVector<int>(
        process, 
        &int_list[0], 
        10);
    BOOST_TEST(int_list == int_list_read);

    std::vector<int> int_list_read_2;
    hadesmem::ReadVector<int>(
        process, 
        &int_list[0], 
        10,
        std::back_inserter(int_list_read_2));
    BOOST_TEST(int_list_read_2 == int_list_read);
}

void TestReadCrossRegion()
{
    SYSTEM_INFO const sys_info = hadesmem::detail::GetSystemInfo();
    DWORD const page_size = sys_info.dwPageSize;

    LPVOID const address = VirtualAlloc(
        nullptr, 
        page_size * 2, 
        MEM_RESERVE | MEM_COMMIT, 
        PAGE_NOACCESS);
    BOOST_TEST(address != 0);

    hadesmem::Process const process(::GetCurrentProcessId());
    std::vector<char> buf = hadesmem::ReadVector<char>(
        process, 
        address,
        page_size * 2);
    std::vector<char> zero_buf(page_size * 2);
    BOOST_TEST(buf == zero_buf);
}

int main()
{
    TestReadPod();
    TestReadString();
    TestReadVector();
    TestReadCrossRegion();
    return boost::report_errors();
}
