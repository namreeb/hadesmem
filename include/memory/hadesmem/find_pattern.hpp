// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>
#include <locale>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <windows.h>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <pugixml.hpp>
#include <pugixml.cpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/detail/static_assert.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/module.hpp>
#include <hadesmem/module_list.hpp>
#include <hadesmem/pelib/dos_header.hpp>
#include <hadesmem/pelib/nt_headers.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/pelib/section.hpp>
#include <hadesmem/pelib/section_list.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/read.hpp>

// TODO: Review, refactor, rewrite, etc this entire module. Put TODOs where 
// appropriate, remove and add APIs, fix bugs, clean up code, etc. Use new 
// language features like noexcept, constexpr, etc.

// TODO: Rewrite to remove cyclic dependencies.

// TODO: Pattern generator.

// TODO: Arbitrary region support.

// TODO: Standalone app/example for FindPattern. For dumping results, 
// experimenting with patterns, automatically generating new patterns, etc.

// TODO: Allow module names and custom regions to be specified in pattern 
// file.

// TODO: Fix order of args etc (general API cleanup) due to adding start 
// address support.

// TODO: Either support all the attributes, manipulators, etc in both the XML 
// file and in source code, or don't support any (which would simplify things 
// vastly). Probably best to only support very simple patterns in source code 
// and require XML for anything complex?

// TODO: Check for duplicate names etc? Or support it to allow patterns 
// overwiting themself when using the Start attribute (any other scenarios?).

// TODO: Clean up XML related code and remove redundant code, add/remove 
// checking where appropriate, etc.

namespace hadesmem
{

    struct FindPatternFlags
    {
        enum : std::uint32_t
        {
            kNone = 0,
            kThrowOnUnmatch = 1 << 0,
            kRelativeAddress = 1 << 1,
            kScanData = 1 << 2,
            kInvalidFlagMaxValue = 1 << 3
        };
    };

    class FindPattern;

    class Pattern
    {
    public:
        Pattern(FindPattern& finder,
            std::wstring const& data,
            std::uint32_t flags)
            : Pattern(finder, data, flags, L"")
        { }

        Pattern(FindPattern& finder,
            std::wstring const& data,
            std::uint32_t flags,
            std::wstring const& start)
            : Pattern(finder, data, L"", flags, start)
        { }

        Pattern(FindPattern& finder,
            std::wstring const& data,
            std::wstring const& name,
            std::uint32_t flags)
            : Pattern(finder, data, name, flags, L"")
        { }

        Pattern(FindPattern& finder,
            std::wstring const& data,
            std::wstring const& name,
            std::uint32_t flags,
            std::wstring const& start);

        Pattern(Pattern const&) = default;

        Pattern& operator=(Pattern const&) = default;

#if !defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

        Pattern(Pattern&&) = default;

        Pattern& operator=(Pattern&&) = default;

#else // #if !defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

        Pattern(Pattern&& other) HADESMEM_DETAIL_NOEXCEPT
            : process_(other.process_),
            base_(other.base_),
            name_(std::move(other.name_)),
            address_(other.address_),
            flags_(other.flags_)
        {
            other.process_ = nullptr;
            other.base_ = 0;
            other.address_ = nullptr;
            other.flags_ = FindPatternFlags::kNone;
        }

        Pattern& operator=(Pattern&& other) HADESMEM_DETAIL_NOEXCEPT
        {
            process_ = other.process_;
            other.process_ = nullptr;

            base_ = other.base_;
            other.base_ = 0;

            name_ = std::move(other.name_);

            address_ = other.address_;
            other.address_ = nullptr;

            flags_ = other.flags_;
            other.flags_ = FindPatternFlags::kNone;

            return *this;
        }

#endif // #if !defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

        Process const* GetProcess() const HADESMEM_DETAIL_NOEXCEPT
        {
            return process_;
        }

        DWORD_PTR GetBase() const HADESMEM_DETAIL_NOEXCEPT
        {
            return base_;
        }

        std::wstring GetName() const HADESMEM_DETAIL_NOEXCEPT
        {
            return name_;
        }

        PVOID GetAddress() const HADESMEM_DETAIL_NOEXCEPT
        {
            return address_;
        }

        void SetAddress(PVOID address)
        {
            address_ = static_cast<PBYTE>(address);
        }

        std::uint32_t GetFlags() const HADESMEM_DETAIL_NOEXCEPT
        {
            return flags_;
        }

    private:
        Process const* process_;
        DWORD_PTR base_;
        std::wstring name_;
        PBYTE address_;
        std::uint32_t flags_;
    };

    struct PatternData
    {
        void* address;
        std::uint32_t flags;
    };

    inline bool operator==(PatternData const& lhs, PatternData const& rhs)
    {
        return lhs.address == rhs.address && lhs.flags == rhs.flags;
    }

    inline bool operator!=(PatternData const& lhs, PatternData const& rhs)
    {
        return !(lhs == rhs);
    }

    inline void Add(Pattern& pattern, DWORD_PTR offset)
    {
        PBYTE const address = static_cast<PBYTE>(pattern.GetAddress());
        if (!address)
        {
            return;
        }

        pattern.SetAddress(address + offset);
    }

    inline void Sub(Pattern& pattern, DWORD_PTR offset)
    {
        PBYTE const address = static_cast<PBYTE>(pattern.GetAddress());
        if (!address)
        {
            return;
        }

        pattern.SetAddress(address - offset);
    }

    inline void Lea(Pattern& pattern)
    {
        PBYTE address = static_cast<PBYTE>(pattern.GetAddress());
        if (!address)
        {
            return;
        }

        try
        {
            bool const is_relative_address =
                !!(pattern.GetFlags() &
                FindPatternFlags::kRelativeAddress);
            DWORD_PTR base = is_relative_address ? pattern.GetBase() : 0;
            address = Read<PBYTE>(*pattern.GetProcess(), address + base);
        }
        catch (std::exception const& /*e*/)
        {
            address = nullptr;
        }

        pattern.SetAddress(address);
    }

    inline void Rel(Pattern& pattern, DWORD_PTR size, DWORD_PTR offset)
    {
        PBYTE address = static_cast<PBYTE>(pattern.GetAddress());
        if (!address)
        {
            return;
        }

        try
        {
            bool const is_relative_address =
                !!(pattern.GetFlags() &
                FindPatternFlags::kRelativeAddress);
            DWORD_PTR const base =
                is_relative_address ? pattern.GetBase() : 0;
            address =
                Read<PBYTE>(*pattern.GetProcess(), address + base) +
                reinterpret_cast<DWORD_PTR>(address + base) +
                size - offset;
        }
        catch (std::exception const& /*e*/)
        {
            address = nullptr;
        }

        pattern.SetAddress(address);
    }

    class FindPattern
    {
    public:
        friend class Pattern;

        explicit FindPattern(Process const& process, HMODULE module)
            : process_(&process),
            base_(0),
            code_regions_(),
            data_regions_(),
            addresses_()
        {
            if (!module)
            {
                ModuleList const modules(process);
                auto const exe_mod = std::begin(modules);
                HADESMEM_DETAIL_ASSERT(exe_mod != std::end(modules));
                module = exe_mod->GetHandle();
            }

            PBYTE const base = reinterpret_cast<PBYTE>(module);
            base_ = reinterpret_cast<DWORD_PTR>(base);
            PeFile const pe_file(process, reinterpret_cast<PVOID>(base),
                hadesmem::PeFileType::Image);
            DosHeader const dos_header(process, pe_file);
            NtHeaders const nt_headers(process, pe_file);

            SectionList const sections(process, pe_file);
            for (auto const& s : sections)
            {
                bool const is_code_section =
                    !!(s.GetCharacteristics() & IMAGE_SCN_CNT_CODE);
                bool const is_data_section =
                    !!(s.GetCharacteristics() &
                    IMAGE_SCN_CNT_INITIALIZED_DATA);
                if (!is_code_section && !is_data_section)
                {
                    continue;
                }

                PBYTE const section_beg = static_cast<PBYTE>(RvaToVa(
                    process,
                    pe_file,
                    s.GetVirtualAddress()));
                if (section_beg == nullptr)
                {
                    HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                        ErrorString("Could not get section base address."));
                }

                DWORD const section_size = s.GetSizeOfRawData();
                if (!section_size)
                {
                    continue;
                }

                PBYTE const section_end = section_beg + section_size;

                std::vector<std::pair<PBYTE, PBYTE>>& region =
                    is_code_section ? code_regions_ : data_regions_;
                region.emplace_back(section_beg, section_end);
            }

            if (code_regions_.empty() && data_regions_.empty())
            {
                HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                    ErrorString("No valid sections to scan found."));
            }
        }

        FindPattern(FindPattern const&) = default;

        FindPattern& operator=(FindPattern const&) = default;

#if !defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

        FindPattern(FindPattern&&) = default;

        FindPattern& operator=(FindPattern&&) = default;

#else // #if !defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

        FindPattern(FindPattern&& other) HADESMEM_DETAIL_NOEXCEPT
            : process_(other.process_),
            base_(other.base_),
            code_regions_(std::move(other.code_regions_)),
            data_regions_(std::move(other.data_regions_)),
            addresses_(std::move(other.addresses_))
        {
            other.process_ = nullptr;
            other.base_ = 0;
        }

        FindPattern& operator=(FindPattern&& other) HADESMEM_DETAIL_NOEXCEPT
        {
            process_ = other.process_;
            other.process_ = nullptr;

            base_ = other.base_;
            other.base_ = 0;

            code_regions_ = std::move(other.code_regions_);

            data_regions_ = std::move(other.data_regions_);

            addresses_ = std::move(other.addresses_);

            return *this;
        }

#endif // #if !defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

        PVOID Find(std::wstring const& data, std::uint32_t flags) const
        {
            return Find(data, flags, L"");
        }

        PVOID Find(
            std::wstring const& data,
            std::wstring const& name,
            std::uint32_t flags)
        {
            return Find(data, name, flags, L"");
        }

        PVOID Find(
            std::wstring const& data,
            std::wstring const& name,
            std::uint32_t flags,
            std::wstring const& start)
        {
            PVOID const address = Find(data, flags, start);

            if (!name.empty())
            {
                addresses_[name] = PatternData{ address, flags };
            }

            return address;
        }

        PVOID Find(
            std::wstring const& data,
            std::uint32_t flags,
            std::wstring const& start) const
        {
            HADESMEM_DETAIL_ASSERT(!(flags &
                ~(FindPatternFlags::kInvalidFlagMaxValue - 1UL)));

            auto const data_real = ConvertData(data);

            PVOID address = Find(
                data_real,
                flags,
                start);

            if (!address && !!(flags & FindPatternFlags::kThrowOnUnmatch))
            {
                HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                    ErrorString("Could not match pattern."));
            }

            bool const is_relative = !!(flags &
                FindPatternFlags::kRelativeAddress);
            if (address && is_relative)
            {
                address = static_cast<PBYTE>(address)-base_;
            }

            return address;
        }

        std::map<std::wstring, PatternData> GetAddresses() const
        {
            return addresses_;
        }

        PVOID operator[](std::wstring const& name) const
        {
            auto const iter = addresses_.find(name);
            return (iter != addresses_.end()) ? iter->second.address : nullptr;
        }

        PVOID Lookup(std::wstring const& name) const
        {
            auto const iter = addresses_.find(name);
            if (iter == std::end(addresses_))
            {
                HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                    ErrorString("Could not find target pattern."));
            }

            return iter->second.address;
        }

        void Update(std::wstring const& name, PVOID address, std::uint32_t flags)
        {
            addresses_[name] = PatternData{ address, flags };
        }

        void LoadFile(std::wstring const& path)
        {
            pugi::xml_document doc;
            auto const load_result = doc.load_file(path.c_str());
            if (!load_result)
            {
                HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                    ErrorString("Loading XML file failed.") <<
                    ErrorCodeOther(load_result.status) <<
                    ErrorStringOther(load_result.description()));
            }

            LoadImpl(doc);
        }

        void LoadFileMemory(std::wstring const& data)
        {
            pugi::xml_document doc;
            auto const load_result = doc.load(data.c_str());
            if (!load_result)
            {
                HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                    ErrorString("Loading XML file failed.") <<
                    ErrorCodeOther(load_result.status) <<
                    ErrorStringOther(load_result.description()));
            }

            LoadImpl(doc);
        }

        friend bool operator==(FindPattern const& lhs, FindPattern const& rhs)
        {
            return lhs.process_ == rhs.process_ &&
                lhs.base_ == rhs.base_ &&
                lhs.addresses_ == rhs.addresses_;
        }

        friend bool operator!=(FindPattern const& lhs, FindPattern const& rhs)
        {
            return !(lhs == rhs);
        }

    private:
        struct PatternInfo
        {
            std::wstring name;
            std::wstring data;
            std::wstring start;
            std::uint32_t flags;
        };

        struct ManipInfo
        {
            struct Manipulator
            {
                enum : std::uint32_t
                {
                    kAdd,
                    kSub,
                    kRel,
                    kLea
                };
            };

            std::uint32_t type;
            bool has_operand1;
            std::uintptr_t operand1;
            bool has_operand2;
            std::uintptr_t operand2;
        };

        struct PatternInfoFull
        {
            PatternInfo pattern;
            std::vector<ManipInfo> manipulators;
        };

        struct FindPatternInfo
        {
            std::uint32_t flags;
            std::vector<PatternInfoFull> patterns;
        };

        std::vector<std::pair<BYTE, bool>> ConvertData(
            std::wstring const& data) const
        {
            HADESMEM_DETAIL_ASSERT(!data.empty());

            std::wstring const data_trimmed = data.substr(0,
                data.find_last_not_of(L" \n\r\t") + 1);

            HADESMEM_DETAIL_ASSERT(!data_trimmed.empty());

            std::wistringstream data_str(data_trimmed);
            data_str.imbue(std::locale::classic());
            std::vector<std::pair<BYTE, bool>> data_real;
            for (;;)
            {
                std::wstring data_cur_str;
                if (!(data_str >> data_cur_str))
                {
                    HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                        ErrorString("Data parsing failed."));
                }

                bool const is_wildcard = (data_cur_str == L"??");
                std::uint32_t current = 0U;
                if (!is_wildcard)
                {
                    std::wistringstream conv(data_cur_str);
                    conv.imbue(std::locale::classic());
                    if (!(conv >> std::hex >> current))
                    {
                        HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                            ErrorString("Data conversion failed."));
                    }

                    if (current > 0xFFU)
                    {
                        HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                            ErrorString("Invalid data."));
                    }
                }

                data_real.emplace_back(
                    static_cast<BYTE>(current),
                    !is_wildcard);

                if (data_str.eof())
                {
                    break;
                }
            }

            return data_real;
        }

        PatternData LookupEx(std::wstring const& name) const
        {
            auto const iter = addresses_.find(name);
            if (iter == std::end(addresses_))
            {
                HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                    ErrorString("Could not find target pattern."));
            }

            return iter->second;
        }

        PVOID Find(
            std::vector<std::pair<BYTE, bool>> const& data,
            std::uint32_t flags,
            std::wstring const& start) const
        {
            HADESMEM_DETAIL_ASSERT(!data.empty());

            PVOID start_addr = nullptr;
            if (!start.empty())
            {
                PatternData const start_pattern = LookupEx(start);
                bool const start_pattern_relative = !!(start_pattern.flags &
                    FindPatternFlags::kRelativeAddress);
                start_addr = start_pattern_relative
                    ? static_cast<PBYTE>(start_pattern.address) + base_
                    : start_pattern.address;
            }

            bool const scan_data_secs = !!(flags &
                FindPatternFlags::kScanData);
            std::vector<std::pair<PBYTE, PBYTE>> const& scan_regions =
                scan_data_secs ? data_regions_ : code_regions_;
            for (auto const& region : scan_regions)
            {
                PBYTE s_beg = region.first;
                PBYTE const s_end = region.second;

                // Support custom scan start address.
                if (start_addr)
                {
                    // Use specified starting address (plus one, so we don't 
                    // just find the same thing again) if we're in the target 
                    // region.
                    if (start_addr >= s_beg && start_addr < s_end)
                    {
                        s_beg = static_cast<PBYTE>(start_addr)+1;
                        if (s_beg == s_end)
                        {
                            HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                                ErrorString("Invalid start address."));
                        }
                    }
                    // Skip if we're not in the target region.
                    else
                    {
                        continue;
                    }
                }

                HADESMEM_DETAIL_ASSERT(s_beg < s_end);

                std::ptrdiff_t const mem_size = s_end - s_beg;
                std::vector<BYTE> const buffer(ReadVector<BYTE>(
                    *process_,
                    s_beg,
                    static_cast<std::size_t>(mem_size)));

                auto const iter = std::search(
                    std::begin(buffer),
                    std::end(buffer),
                    std::begin(data),
                    std::end(data),
                    [](BYTE h_cur, std::pair<BYTE, bool> const& n_cur)
                {
                    return (!n_cur.second) || (h_cur == n_cur.first);
                });

                if (iter != std::end(buffer))
                {
                    return (s_beg + std::distance(std::begin(buffer), iter));
                }
            }

            return nullptr;
        }

        void LoadImpl(pugi::xml_document const& doc)
        {
            auto const patterns_info_full = ReadPatternsFromXml(doc);
            auto const& pattern_infos = patterns_info_full.patterns;
            for (auto const& p : pattern_infos)
            {
                auto const& pat_info = p.pattern;
                std::uint32_t const flags = patterns_info_full.flags | pat_info.flags;
                std::wstring const name = pat_info.name;
                Pattern pattern(*this, pat_info.data, name, flags, pat_info.start);

                auto const& manip_list = p.manipulators;
                for (auto const& m : manip_list)
                {
                    switch (m.type)
                    {
                    case ManipInfo::Manipulator::kAdd:
                        if (!m.has_operand1 || m.has_operand2)
                        {
                            HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                                ErrorString("Invalid manipulator operands "
                                "for 'Add'."));
                        }

                        Add(pattern, m.operand1);

                        break;

                    case ManipInfo::Manipulator::kSub:
                        if (!m.has_operand1 || m.has_operand2)
                        {
                            HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                                ErrorString("Invalid manipulator operands "
                                "for 'Sub'."));
                        }

                        Sub(pattern, m.operand1);

                        break;

                    case ManipInfo::Manipulator::kRel:
                        if (!m.has_operand1 || !m.has_operand2)
                        {
                            HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                                ErrorString("Invalid manipulator operands "
                                "for 'Rel'."));
                        }

                        Rel(pattern, m.operand1, m.operand2);

                        break;

                    case ManipInfo::Manipulator::kLea:
                        if (m.has_operand1 || m.has_operand2)
                        {
                            HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                                ErrorString("Invalid manipulator operands "
                                "for 'Lea'."));
                        }

                        Lea(pattern);

                        break;

                    default:
                        HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                            ErrorString("Unknown manipulator."));
                    }
                }

                if (!name.empty())
                {
                    addresses_[name] = { pattern.GetAddress(), pattern.GetFlags() };
                }
            }
        }

        FindPatternInfo ReadPatternsFromXml(pugi::xml_document const& doc)
        {
            auto const hadesmem_root = doc.child(L"HadesMem");
            if (!hadesmem_root)
            {
                HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                    ErrorString("Failed to find 'HadesMem' root node."));
            }

            auto const find_pattern_node = hadesmem_root.child(
                L"FindPattern");
            if (!find_pattern_node)
            {
                HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                    ErrorString("Failed to find 'Patterns' node."));
            }

            auto const read_flags =
                [](pugi::xml_node const& node) -> std::uint32_t
            {
                std::uint32_t flags = FindPatternFlags::kNone;
                for (auto const& flag : node.children(L"Flag"))
                {
                    auto const flag_name_attr = flag.attribute(L"Name");
                    if (!flag_name_attr)
                    {
                        HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                            ErrorString("Failed to find 'Name' attribute for "
                            "'Flag' node."));
                    }
                    std::wstring const flag_name = flag_name_attr.value();
                    if (flag_name.empty())
                    {
                        HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                            ErrorString("Failed to find value for 'Name' "
                            "attribute for 'Flag' node."));
                    }

                    if (flag_name == L"None")
                    {
                        flags |= FindPatternFlags::kNone;
                    }
                    else if (flag_name == L"ThrowOnUnmatch")
                    {
                        flags |= FindPatternFlags::kThrowOnUnmatch;
                    }
                    else if (flag_name == L"RelativeAddress")
                    {
                        flags |= FindPatternFlags::kRelativeAddress;
                    }
                    else if (flag_name == L"ScanData")
                    {
                        flags |= FindPatternFlags::kScanData;
                    }
                    else
                    {
                        HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                            ErrorString("Unknown 'Flag' value."));
                    }
                }

                return flags;
            };

            std::uint32_t const flags = read_flags(find_pattern_node);

            std::vector<PatternInfoFull> pattern_infos;

            for (auto const& pattern : find_pattern_node.children(L"Pattern"))
            {
                auto const pattern_name_attr = pattern.attribute(L"Name");
                if (!pattern_name_attr)
                {
                    HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                        ErrorString("Failed to find 'Name' attribute for "
                        "'Pattern' node."));
                }
                std::wstring const pattern_name = pattern_name_attr.value();
                if (pattern_name.empty())
                {
                    HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                        ErrorString("Failed to find value for 'Name' "
                        "attribute for 'Pattern' node."));
                }

                auto const pattern_data_attr = pattern.attribute(L"Data");
                if (!pattern_data_attr)
                {
                    HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                        ErrorString("Failed to find 'Data' attribute "
                        "for 'Pattern' node."));
                }
                std::wstring const pattern_data = pattern_data_attr.value();
                if (pattern_data.empty())
                {
                    HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                        ErrorString("Failed to find value for 'Data' "
                        "attribute for 'Pattern' node."));
                }

                std::wstring pattern_start;
                auto const pattern_start_attr = pattern.attribute(L"Start");
                if (pattern_start_attr)
                {
                    pattern_start = pattern_start_attr.value();
                    if (pattern_start.empty())
                    {
                        HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                            ErrorString("Failed to find value for 'Start' "
                            "attribute for 'Pattern' node."));
                    }
                }

                std::uint32_t const pattern_flags = read_flags(pattern);

                PatternInfo pattern_info{
                    pattern_name,
                    pattern_data,
                    pattern_start,
                    pattern_flags };

                std::vector<ManipInfo> pattern_manips;

                for (auto const& manipulator : pattern.children(
                    L"Manipulator"))
                {
                    auto const manipulator_name_attr = manipulator.attribute(
                        L"Name");
                    if (!manipulator_name_attr)
                    {
                        HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                            ErrorString("Failed to find 'Name' attribute "
                            "for 'Manipulator' node."));
                    }
                    std::wstring const manipulator_name =
                        manipulator_name_attr.value();
                    if (manipulator_name.empty())
                    {
                        HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                            ErrorString("Failed to find value for 'Name' "
                            "attribute for 'Manipulator' node."));
                    }

                    std::uint32_t type = 0;
                    if (manipulator_name == L"Add")
                    {
                        type = ManipInfo::Manipulator::kAdd;
                    }
                    else if (manipulator_name == L"Sub")
                    {
                        type = ManipInfo::Manipulator::kSub;
                    }
                    else if (manipulator_name == L"Rel")
                    {
                        type = ManipInfo::Manipulator::kRel;
                    }
                    else if (manipulator_name == L"Lea")
                    {
                        type = ManipInfo::Manipulator::kLea;
                    }
                    else
                    {
                        HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                            ErrorString("Unknown value for 'Name' attribute "
                            "for 'Manipulator' node."));
                    }

                    auto const hex_str_to_uintptr =
                        [](std::wstring const& s) -> std::uintptr_t
                    {
                        std::wstringstream str;
                        str.imbue(std::locale::classic());
                        std::uintptr_t result = 0;
                        if (!(str << s) || !(str >> std::hex >> result))
                        {
                            HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                                ErrorString("Failed to convert hex string to "
                                "integer."));
                        }
                        return result;
                    };

                    auto const manipulator_operand1 = manipulator.attribute(
                        L"Operand1");
                    bool const has_operand1 = !!manipulator_operand1;
                    std::uintptr_t const operand1 = has_operand1
                        ? hex_str_to_uintptr(manipulator_operand1.value())
                        : 0U;

                    auto const manipulator_operand2 = manipulator.attribute(
                        L"Operand2");
                    bool const has_operand2 = !!manipulator_operand2;
                    std::uintptr_t const operand2 = has_operand2
                        ? hex_str_to_uintptr(manipulator_operand2.value())
                        : 0U;

                    pattern_manips.emplace_back(ManipInfo{
                        type,
                        has_operand1,
                        operand1,
                        has_operand2,
                        operand2 });
                }

                pattern_infos.emplace_back(PatternInfoFull{
                    pattern_info,
                    pattern_manips });
            }

            return{ flags, pattern_infos };
        }

        Process const* process_;
        DWORD_PTR base_;
        std::vector<std::pair<PBYTE, PBYTE>> code_regions_;
        std::vector<std::pair<PBYTE, PBYTE>> data_regions_;
        std::map<std::wstring, PatternData> addresses_;
    };

    inline void Save(FindPattern& find_pattern, Pattern const& pattern)
    {
        std::wstring const name{ pattern.GetName() };
        if (name.empty())
        {
            return;
        }

        find_pattern.Update(
            pattern.GetName(),
            pattern.GetAddress(),
            pattern.GetFlags());
    }

    inline Pattern::Pattern(FindPattern& finder,
        std::wstring const& data,
        std::wstring const& name,
        std::uint32_t flags,
        std::wstring const& start)
        : process_(finder.process_),
        base_(finder.base_),
        name_(name),
        address_(static_cast<PBYTE>(finder.Find(data, flags, start))),
        flags_(flags)
    { }

}
