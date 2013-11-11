// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <algorithm>
#include <cstdint>
#include <fstream>
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
#include <hadesmem/detail/str_conv.hpp>
#include <hadesmem/error.hpp>
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

// TODO: Add stream overloads.

// TODO: Rewrite to remove cyclic dependencies.

// TODO: Pattern generator.

// TODO: Multi-pass support (e.g. search for pattern, apply for manipulators, 
// use as starting point for second search).

// TODO: Arbitrary region support.

// TODO: Standalone app/example for FindPattern. For dumping results, 
// experimenting with patterns, automatically generating new patterns, etc.

namespace hadesmem
{
    namespace detail
    {

        struct PatternInfo
        {
            std::wstring name;
            std::wstring data;
        };

        struct ManipInfo
        {
            struct Manipulator
            {
                enum
                {
                    kAdd,
                    kSub,
                    kRel,
                    kLea
                };
            };

            std::int32_t type;
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

    }

    struct FindPatternFlags
    {
        enum
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
        Pattern(
            FindPattern& finder, 
            std::wstring const& data, 
            std::uint32_t flags);

        Pattern(FindPattern& finder, 
            std::wstring const& data,
            std::wstring const& name, 
            std::uint32_t flags);

        Pattern(Pattern const&) = default;

        Pattern& operator=(Pattern const&) = default;

#if !defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

        Pattern(Pattern&&) = default;

        Pattern& operator=(Pattern&&) = default;

#else // #if !defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

        Pattern(Pattern&& other) HADESMEM_DETAIL_NOEXCEPT
            : finder_(other.finder_),
            name_(std::move(other.name_)),
            address_(other.address_),
            flags_(other.flags_)
        {
            other.finder_ = nullptr;
            other.address_ = nullptr;
            other.flags_ = FindPatternFlags::kNone;
        }

        Pattern& operator=(Pattern&& other) HADESMEM_DETAIL_NOEXCEPT
        {
            finder_ = other.finder_;
            other.finder_ = nullptr;

            name_ = std::move(other.name_);

            address_ = other.address_;
            other.address_ = nullptr;

            flags_ = other.flags_;
            other.flags_ = FindPatternFlags::kNone;

            return *this;
        }

#endif // #if !defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

        void Save();

        // TODO: More consistent naming.
        void Update(PBYTE address)
        {
            address_ = address;
        }

        PBYTE GetAddress() const HADESMEM_DETAIL_NOEXCEPT
        {
            return address_;
        }

        std::uint32_t GetFlags() const HADESMEM_DETAIL_NOEXCEPT
        {
            return flags_;
        }

        DWORD_PTR GetBase() const HADESMEM_DETAIL_NOEXCEPT;

        Process const* GetProcess() const HADESMEM_DETAIL_NOEXCEPT;

    private:
        FindPattern* finder_;
        std::wstring name_;
        PBYTE address_;
        std::uint32_t flags_;
    };

    namespace pattern_manipulators
    {

        template <typename D>
        class Manipulator
        {
        public:
            void Manipulate(Pattern& pattern) const
            {
                return static_cast<D const*>(this)->Manipulate(pattern);
            }
        };

        template <typename D>
        inline Pattern& operator<<(Pattern& pattern,
            Manipulator<D> const& manipulator)
        {
            manipulator.Manipulate(pattern);
            return pattern;
        }

        class Save : public Manipulator<Save>
        {
        public:
            void Manipulate(Pattern& pattern) const;
        };

        class Add : public Manipulator<Add>
        {
        public:
            explicit Add(DWORD_PTR offset) HADESMEM_DETAIL_NOEXCEPT
                : offset_(offset)
            { }

            void Manipulate(Pattern& pattern) const;

        private:
            DWORD_PTR offset_;
        };

        class Sub : public Manipulator<Sub>
        {
        public:
            explicit Sub(DWORD_PTR offset) HADESMEM_DETAIL_NOEXCEPT
                : offset_(offset)
            { }

            void Manipulate(Pattern& pattern) const;

        private:
            DWORD_PTR offset_;
        };

        class Lea : public Manipulator<Lea>
        {
        public:
            void Manipulate(Pattern& pattern) const;
        };

        class Rel : public Manipulator<Rel>
        {
        public:
            explicit Rel(DWORD_PTR size, DWORD_PTR offset) 
                HADESMEM_DETAIL_NOEXCEPT
                : size_(size),
                offset_(offset)
            { }

            void Manipulate(Pattern& pattern) const;

        private:
            DWORD_PTR size_;
            DWORD_PTR offset_;
        };

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

                PBYTE const section_end = section_beg + s.GetSizeOfRawData();

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
            HADESMEM_DETAIL_ASSERT(!(flags &
                ~(FindPatternFlags::kInvalidFlagMaxValue - 1UL)));

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

            bool const scan_data_secs = !!(flags & 
                FindPatternFlags::kScanData);

            PVOID address = Find(data_real, scan_data_secs);

            if (!address && !!(flags & FindPatternFlags::kThrowOnUnmatch))
            {
                HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                    ErrorString("Could not match pattern."));
            }

            if (address && !!(flags & FindPatternFlags::kRelativeAddress))
            {
                address = static_cast<PBYTE>(address)-base_;
            }

            return address;
        }

        PVOID Find(
            std::wstring const& data, 
            std::wstring const& name, 
            std::uint32_t flags)
        {
            PVOID const address = Find(data, flags);

            if (!name.empty())
            {
                addresses_[name] = address;
            }

            return address;
        }

        std::map<std::wstring, PVOID> GetAddresses() const
        {
            return addresses_;
        }

        PVOID operator[](std::wstring const& name) const
        {
            auto const iter = addresses_.find(name);
            return (iter != addresses_.end()) ? iter->second : nullptr;
        }

        PVOID Lookup(std::wstring const& name) const
        {
            auto const iter = addresses_.find(name);
            if (iter == std::end(addresses_))
            {
                HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                    ErrorString("Could not find target pattern."));
            }

            return iter->second;
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
        PVOID Find(std::vector<std::pair<BYTE, bool>> const& data,
            bool scan_data_secs) const
        {
            HADESMEM_DETAIL_ASSERT(!data.empty());

            std::vector<std::pair<PBYTE, PBYTE>> const& scan_regions =
                scan_data_secs ? data_regions_ : code_regions_;
            for (auto const& region : scan_regions)
            {
                PBYTE const s_beg = region.first;
                PBYTE const s_end = region.second;
                HADESMEM_DETAIL_ASSERT(s_end > s_beg);

                std::ptrdiff_t const mem_size = s_end - s_beg;
                HADESMEM_DETAIL_ASSERT(s_beg <= s_end);
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

            std::uint32_t flags = FindPatternFlags::kNone;
            for (auto const& flag : find_pattern_node.children(L"Flag"))
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

            std::vector<detail::PatternInfoFull> pattern_infos;

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

                detail::PatternInfo pattern_info{
                    pattern_name,
                    pattern_data };

                std::vector<detail::ManipInfo> pattern_manips;

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

                    std::int32_t type = 0;
                    if (manipulator_name == L"Add")
                    {
                        type = detail::ManipInfo::Manipulator::kAdd;
                    }
                    else if (manipulator_name == L"Sub")
                    {
                        type = detail::ManipInfo::Manipulator::kSub;
                    }
                    else if (manipulator_name == L"Rel")
                    {
                        type = detail::ManipInfo::Manipulator::kRel;
                    }
                    else if (manipulator_name == L"Lea")
                    {
                        type = detail::ManipInfo::Manipulator::kLea;
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

                    pattern_manips.emplace_back(detail::ManipInfo{
                        type,
                        has_operand1,
                        operand1,
                        has_operand2,
                        operand2 });
                }

                pattern_infos.emplace_back(detail::PatternInfoFull{
                    pattern_info,
                    pattern_manips });
            }


            for (auto const& p : pattern_infos)
            {
                auto const& pat_info = p.pattern;
                Pattern pattern(*this, pat_info.data, pat_info.name, flags);

                auto const& manip_list = p.manipulators;
                for (auto const& m : manip_list)
                {
                    switch (m.type)
                    {
                    case detail::ManipInfo::Manipulator::kAdd:
                        if (!m.has_operand1 || m.has_operand2)
                        {
                            HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                                ErrorString("Invalid manipulator operands "
                                "for 'Add'."));
                        }

                        pattern << pattern_manipulators::Add(m.operand1);

                        break;

                    case detail::ManipInfo::Manipulator::kSub:
                        if (!m.has_operand1 || m.has_operand2)
                        {
                            HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                                ErrorString("Invalid manipulator operands "
                                "for 'Sub'."));
                        }

                        pattern << pattern_manipulators::Sub(m.operand1);

                        break;

                    case detail::ManipInfo::Manipulator::kRel:
                        if (!m.has_operand1 || !m.has_operand2)
                        {
                            HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                                ErrorString("Invalid manipulator operands "
                                "for 'Rel'."));
                        }

                        pattern << pattern_manipulators::Rel(m.operand1, m.operand2);

                        break;

                    case detail::ManipInfo::Manipulator::kLea:
                        if (m.has_operand1 || m.has_operand2)
                        {
                            HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                                ErrorString("Invalid manipulator operands "
                                "for 'Lea'."));
                        }

                        pattern << pattern_manipulators::Lea();

                        break;

                    default:
                        HADESMEM_DETAIL_THROW_EXCEPTION(Error() <<
                            ErrorString("Unknown manipulator."));
                    }
                }

                pattern.Save();
            }
        }

        Process const* process_;
        DWORD_PTR base_;
        std::vector<std::pair<PBYTE, PBYTE>> code_regions_;
        std::vector<std::pair<PBYTE, PBYTE>> data_regions_;
        std::map<std::wstring, PVOID> addresses_;
    };

    inline Pattern::Pattern(FindPattern& finder,
        std::wstring const& data,
        std::uint32_t flags)
        : finder_(&finder),
        name_(),
        address_(static_cast<PBYTE>(finder.Find(data, flags))),
        flags_(flags)
    { }

    inline Pattern::Pattern(FindPattern& finder,
        std::wstring const& data,
        std::wstring const& name,
        std::uint32_t flags)
        : finder_(&finder),
        name_(name),
        address_(static_cast<PBYTE>(finder.Find(data, flags))),
        flags_(flags)
    { }

    inline void Pattern::Save()
    {
        if (name_.empty())
        {
            return;
        }

        // TODO: This feels like a hack. Investigate and fix this. (And if 
        // appropriate, remove friendship requirement.)
        finder_->addresses_[name_] = address_;
    }

    inline DWORD_PTR Pattern::GetBase() const HADESMEM_DETAIL_NOEXCEPT
    {
        // TODO: This feels like a hack. Investigate and fix this. (And if 
        // appropriate, remove friendship requirement.)
        return finder_->base_;
    }

    // TODO: This feels like a hack. Investigate and fix this.
    inline Process const* Pattern::GetProcess() const HADESMEM_DETAIL_NOEXCEPT
    {
        return finder_->process_;
    }

    namespace pattern_manipulators
    {

        inline void Save::Manipulate(Pattern& pattern) const
        {
            pattern.Save();
        }

        inline void Add::Manipulate(Pattern& pattern) const
        {
            PBYTE const address = pattern.GetAddress();
            if (!address)
            {
                return;
            }

            pattern.Update(address + offset_);
        }

        inline void Sub::Manipulate(Pattern& pattern) const
        {
            PBYTE const address = pattern.GetAddress();
            if (!address)
            {
                return;
            }

            pattern.Update(address - offset_);
        }

        inline void Lea::Manipulate(Pattern& pattern) const
        {
            PBYTE address = pattern.GetAddress();
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

            pattern.Update(address);
        }

        inline void Rel::Manipulate(Pattern& pattern) const
        {
            PBYTE address = pattern.GetAddress();
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
                    size_ - offset_;
            }
            catch (std::exception const& /*e*/)
            {
                address = nullptr;
            }

            pattern.Update(address);
        }

    }

}
