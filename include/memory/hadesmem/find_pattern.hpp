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
#include <hadesmem/detail/to_upper_ordinal.hpp>
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

// TODO: Add support for RVA as start address, not just name.

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

class FindPattern
{
public:
  // TODO: Use an enum instead of a boolean.
  explicit FindPattern(Process const& process,
                       std::wstring const& pattern_file,
                       bool in_memory_file)
    : process_(&process), find_pattern_datas_()
  {
    if (in_memory_file)
    {
      LoadPatternFileMemory(pattern_file);
    }
    else
    {
      LoadPatternFile(pattern_file);
    }
  }

#if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

  FindPattern(FindPattern const&) = default;

  FindPattern& operator=(FindPattern const&) = default;

  FindPattern(FindPattern&& other)
    : process_(other.process_),
      find_pattern_datas_(std::move(other.find_pattern_datas_))
  {
    other.process_ = nullptr;
  }

  FindPattern& operator=(FindPattern&& other)
  {
    process_ = other.process_;
    other.process_ = nullptr;

    find_pattern_datas_ = std::move(other.find_pattern_datas_);

    return *this;
  }

#endif // #if defined(HADESMEM_DETAIL_NO_RVALUE_REFERENCES_V3)

  // TODO: Add a way to enumerate all patterns.

  PVOID Lookup(std::wstring const& module, std::wstring const& name) const
  {
    return LookupEx(module, name).address;
  }

  // TODO: Add a way to enumerate all modules/bases.

  friend bool operator==(FindPattern const& lhs, FindPattern const& rhs)
  {
    return lhs.process_ == rhs.process_ &&
           lhs.find_pattern_datas_ == rhs.find_pattern_datas_;
  }

  friend bool operator!=(FindPattern const& lhs, FindPattern const& rhs)
  {
    return !(lhs == rhs);
  }

  // TODO: Remove this once enumeration is supported.
  std::size_t ModuleCount() const
  {
    return find_pattern_datas_.size();
  }

  // TODO: Remove this once enumeration is supported.
  std::size_t PatternCount(std::wstring const& module) const
  {
    auto const iter = find_pattern_datas_.find(detail::ToUpperOrdinal(module));
    if (iter == std::end(find_pattern_datas_))
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                      << ErrorString("Invalid module name."));
    }

    return iter->second.size();
  }

private:
  inline void LoadPatternFile(std::wstring const& path)
  {
    pugi::xml_document doc;
    auto const load_result = doc.load_file(path.c_str());
    if (!load_result)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("Loading XML file failed.")
                << ErrorCodeOther(load_result.status)
                << ErrorStringOther(load_result.description()));
    }

    LoadPatternFileImpl(doc);
  }

  inline void LoadPatternFileMemory(std::wstring const& data)
  {
    pugi::xml_document doc;
    auto const load_result = doc.load(data.c_str());
    if (!load_result)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("Loading XML file failed.")
                << ErrorCodeOther(load_result.status)
                << ErrorStringOther(load_result.description()));
    }

    LoadPatternFileImpl(doc);
  }

  struct PatternData
  {
    void* address;
    std::uint32_t flags;

    bool operator==(PatternData const& rhs) const
    {
      return address == rhs.address && flags == rhs.flags;
    }

    bool operator!=(PatternData const& rhs) const
    {
      return !(*this == rhs);
    }
  };

  std::vector<std::pair<BYTE, bool>> ConvertData(std::wstring const& data) const
  {
    HADESMEM_DETAIL_ASSERT(!data.empty());

    std::wstring const data_trimmed =
      data.substr(0, data.find_last_not_of(L" \n\r\t") + 1);

    HADESMEM_DETAIL_ASSERT(!data_trimmed.empty());

    std::wistringstream data_str(data_trimmed);
    data_str.imbue(std::locale::classic());
    std::vector<std::pair<BYTE, bool>> data_real;
    for (;;)
    {
      std::wstring data_cur_str;
      if (!(data_str >> data_cur_str))
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                        << ErrorString("Data parsing failed."));
      }

      bool const is_wildcard = (data_cur_str == L"??");
      std::uint32_t current = 0U;
      if (!is_wildcard)
      {
        std::wistringstream conv(data_cur_str);
        conv.imbue(std::locale::classic());
        if (!(conv >> std::hex >> current))
        {
          HADESMEM_DETAIL_THROW_EXCEPTION(
            Error() << ErrorString("Data conversion failed."));
        }

        if (current > 0xFFU)
        {
          HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                          << ErrorString("Invalid data."));
        }
      }

      data_real.emplace_back(static_cast<BYTE>(current), !is_wildcard);

      if (data_str.eof())
      {
        break;
      }
    }

    return data_real;
  }

  PatternData LookupEx(std::wstring const& module,
                       std::wstring const& name) const
  {
    auto const addresses_iter =
      find_pattern_datas_.find(detail::ToUpperOrdinal(module));
    if (addresses_iter == std::end(find_pattern_datas_))
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("Could not find target module."));
    }

    auto const& addresses = addresses_iter->second;

    auto const iter = addresses.find(name);
    if (iter == std::end(addresses))
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("Could not find target pattern."));
    }

    return iter->second;
  }

  struct ModuleInfo
  {
    DWORD_PTR base;
    std::vector<std::pair<PBYTE, PBYTE>> code_regions;
    std::vector<std::pair<PBYTE, PBYTE>> data_regions;
  };

  PVOID Find(ModuleInfo const& mod_info,
             std::wstring const& module,
             std::wstring const& data,
             std::uint32_t flags,
             std::wstring const& start) const
  {
    HADESMEM_DETAIL_ASSERT(
      !(flags & ~(FindPatternFlags::kInvalidFlagMaxValue - 1UL)));

    auto const data_real = ConvertData(data);

    PVOID address = Find(mod_info, module, data_real, flags, start);

    if (!address && !!(flags & FindPatternFlags::kThrowOnUnmatch))
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("Could not match pattern."));
    }

    bool const is_relative = !!(flags & FindPatternFlags::kRelativeAddress);
    if (address && is_relative)
    {
      address = static_cast<PBYTE>(address) - mod_info.base;
    }

    return address;
  }

  ModuleInfo GetModuleInfo(std::wstring const& module) const
  {
    ModuleInfo mod_info;

    if (module.empty())
    {
      Module const module_ex(*process_, nullptr);
      mod_info.base = reinterpret_cast<DWORD_PTR>(module_ex.GetHandle());
    }
    else
    {
      Module const module_ex(*process_, module);
      mod_info.base = reinterpret_cast<DWORD_PTR>(module_ex.GetHandle());
    }

    PBYTE const base = reinterpret_cast<PBYTE>(mod_info.base);
    PeFile const pe_file(*process_, base, hadesmem::PeFileType::Image);
    DosHeader const dos_header(*process_, pe_file);
    NtHeaders const nt_headers(*process_, pe_file);

    SectionList const sections(*process_, pe_file);
    for (auto const& s : sections)
    {
      bool const is_code_section =
        !!(s.GetCharacteristics() & IMAGE_SCN_CNT_CODE);
      bool const is_data_section =
        !!(s.GetCharacteristics() & IMAGE_SCN_CNT_INITIALIZED_DATA);
      if (!is_code_section && !is_data_section)
      {
        continue;
      }

      PBYTE const section_beg =
        static_cast<PBYTE>(RvaToVa(*process_, pe_file, s.GetVirtualAddress()));
      if (section_beg == nullptr)
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(
          Error() << ErrorString("Could not get section base address."));
      }

      DWORD const section_size = s.GetSizeOfRawData();
      if (!section_size)
      {
        continue;
      }

      PBYTE const section_end = section_beg + section_size;

      std::vector<std::pair<PBYTE, PBYTE>>& region =
        is_code_section ? mod_info.code_regions : mod_info.data_regions;
      region.emplace_back(section_beg, section_end);
    }

    if (mod_info.code_regions.empty() && mod_info.data_regions.empty())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("No valid sections to scan found."));
    }

    return mod_info;
  }

  PVOID Find(ModuleInfo const& mod_info,
             std::wstring const& module,
             std::vector<std::pair<BYTE, bool>> const& data,
             std::uint32_t flags,
             std::wstring const& start) const
  {
    HADESMEM_DETAIL_ASSERT(!data.empty());

    PVOID start_addr = nullptr;
    if (!start.empty())
    {
      PatternData const start_pattern = LookupEx(module, start);
      bool const start_pattern_relative =
        !!(start_pattern.flags & FindPatternFlags::kRelativeAddress);
      start_addr = start_pattern_relative
                     ? static_cast<PBYTE>(start_pattern.address) + mod_info.base
                     : start_pattern.address;
    }

    bool const scan_data_secs = !!(flags & FindPatternFlags::kScanData);
    std::vector<std::pair<PBYTE, PBYTE>> const& scan_regions =
      scan_data_secs ? mod_info.data_regions : mod_info.code_regions;
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
          s_beg = static_cast<PBYTE>(start_addr) + 1;
          if (s_beg == s_end)
          {
            HADESMEM_DETAIL_THROW_EXCEPTION(
              Error() << ErrorString("Invalid start address."));
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
      std::vector<BYTE> const buffer(
        ReadVector<BYTE>(*process_, s_beg, static_cast<std::size_t>(mem_size)));

      auto const iter =
        std::search(std::begin(buffer),
                    std::end(buffer),
                    std::begin(data),
                    std::end(data),
                    [](BYTE h_cur, std::pair<BYTE, bool> const& n_cur)
      { return (!n_cur.second) || (h_cur == n_cur.first); });

      if (iter != std::end(buffer))
      {
        return (s_beg + std::distance(std::begin(buffer), iter));
      }
    }

    return nullptr;
  }

  struct PatternInfo
  {
    std::wstring name;
    std::wstring data;
    std::wstring start;
    std::uint32_t flags;
  };

  struct ManipInfo
  {
    enum class Manipulator
    {
      kAdd,
      kSub,
      kRel,
      kLea
    };

    Manipulator type;
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

  inline void* Add(DWORD_PTR /*base*/,
                   void* address,
                   std::uint32_t /*flags*/,
                   DWORD_PTR offset) const
  {
    return address ? static_cast<unsigned char*>(address) + offset : nullptr;
  }

  inline void* Sub(DWORD_PTR /*base*/,
                   void* address,
                   std::uint32_t /*flags*/,
                   DWORD_PTR offset) const
  {
    return address ? static_cast<unsigned char*>(address) - offset : nullptr;
  }

  inline void* Lea(DWORD_PTR base, void* address, std::uint32_t flags) const
  {
    if (!address)
    {
      return nullptr;
    }

    try
    {
      bool const is_relative_address =
        !!(flags & FindPatternFlags::kRelativeAddress);
      DWORD_PTR real_base = is_relative_address ? base : 0;
      return Read<void*>(*process_,
                         static_cast<unsigned char*>(address) + real_base);
    }
    catch (std::exception const& /*e*/)
    {
      return nullptr;
    }
  }

  inline void* Rel(DWORD_PTR base,
                   void* address,
                   std::uint32_t flags,
                   DWORD_PTR size,
                   DWORD_PTR offset) const
  {
    if (!address)
    {
      return nullptr;
    }

    try
    {
      bool const is_relative_address =
        !!(flags & FindPatternFlags::kRelativeAddress);
      DWORD_PTR const real_base = is_relative_address ? base : 0;
      return Read<PBYTE>(*process_,
                         static_cast<unsigned char*>(address) + real_base) +
             reinterpret_cast<DWORD_PTR>(static_cast<unsigned char*>(address) +
                                         real_base) +
             size - offset;
    }
    catch (std::exception const& /*e*/)
    {
      return nullptr;
    }
  }

  inline std::wstring GetAttributeValue(pugi::xml_node const& node,
                                        std::wstring const& name) const
  {
    auto const attr = node.attribute(name.c_str());
    if (!attr)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("Failed to find attribute for node."));
    }

    std::wstring const value = attr.value();
    if (value.empty())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("Failed to find value for attribute."));
    }

    return value;
  }

  inline std::wstring GetOptionalAttributeValue(pugi::xml_node const& node,
                                                std::wstring const& name) const
  {
    auto const attr = node.attribute(name.c_str());
    if (!attr)
    {
      return {};
    }

    std::wstring const value = attr.value();
    if (value.empty())
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("Failed to find value for attribute."));
    }

    return value;
  }

  inline std::uint32_t ReadFlags(pugi::xml_node const& node) const
  {
    std::uint32_t flags = FindPatternFlags::kNone;
    for (auto const& flag : node.children(L"Flag"))
    {
      auto const flag_name = GetAttributeValue(flag, L"Name");

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
        HADESMEM_DETAIL_THROW_EXCEPTION(
          Error() << ErrorString("Unknown 'Flag' value."));
      }
    }

    return flags;
  }

  inline std::map<std::wstring, FindPatternInfo>
    ReadPatternsFromXml(pugi::xml_document const& doc) const
  {
    auto const hadesmem_root = doc.child(L"HadesMem");
    if (!hadesmem_root)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(
        Error() << ErrorString("Failed to find 'HadesMem' root node."));
    }

    std::map<std::wstring, FindPatternInfo> pattern_infos_full;
    for (auto const& find_pattern_node : hadesmem_root.children(L"FindPattern"))
    {
      auto const module_name = detail::ToUpperOrdinal(
        GetOptionalAttributeValue(find_pattern_node, L"Module"));

      std::uint32_t const flags = ReadFlags(find_pattern_node);

      std::vector<PatternInfoFull> pattern_infos;

      for (auto const& pattern : find_pattern_node.children(L"Pattern"))
      {
        auto const pattern_name = GetAttributeValue(pattern, L"Name");

        auto const pattern_data = GetAttributeValue(pattern, L"Data");

        auto const pattern_start = GetOptionalAttributeValue(pattern, L"Start");

        std::uint32_t const pattern_flags = ReadFlags(pattern);

        PatternInfo pattern_info{pattern_name,  pattern_data,
                                 pattern_start, pattern_flags};

        std::vector<ManipInfo> pattern_manips;

        for (auto const& manipulator : pattern.children(L"Manipulator"))
        {
          auto const manipulator_name = GetAttributeValue(manipulator, L"Name");

          ManipInfo::Manipulator type = ManipInfo::Manipulator::kAdd;
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
            HADESMEM_DETAIL_THROW_EXCEPTION(
              Error() << ErrorString("Unknown value for 'Name' attribute for "
                                     "'Manipulator' node."));
          }

          auto const hex_str_to_uintptr = [](std::wstring const & s)
            ->std::uintptr_t
          {
            std::wstringstream str;
            str.imbue(std::locale::classic());
            std::uintptr_t result = 0;
            if (!(str << s) || !(str >> std::hex >> result))
            {
              HADESMEM_DETAIL_THROW_EXCEPTION(
                Error() << ErrorString(
                             "Failed to convert hex string to integer."));
            }
            return result;
          };

          auto const manipulator_operand1 = manipulator.attribute(L"Operand1");
          bool const has_operand1 = !!manipulator_operand1;
          std::uintptr_t const operand1 =
            has_operand1 ? hex_str_to_uintptr(manipulator_operand1.value())
                         : 0U;

          auto const manipulator_operand2 = manipulator.attribute(L"Operand2");
          bool const has_operand2 = !!manipulator_operand2;
          std::uintptr_t const operand2 =
            has_operand2 ? hex_str_to_uintptr(manipulator_operand2.value())
                         : 0U;

          pattern_manips.emplace_back(
            ManipInfo{type, has_operand1, operand1, has_operand2, operand2});
        }

        pattern_infos.emplace_back(
          PatternInfoFull{pattern_info, pattern_manips});
      }

      HADESMEM_DETAIL_ASSERT(pattern_infos_full.find(module_name) ==
                             std::end(pattern_infos_full));
      pattern_infos_full[module_name] = {flags, pattern_infos};
    }

    return pattern_infos_full;
  }

  inline void LoadPatternFileImpl(pugi::xml_document const& doc)
  {
    auto const patterns_info_full_list = ReadPatternsFromXml(doc);
    for (auto const& patterns_info_full_pair : patterns_info_full_list)
    {
      HADESMEM_DETAIL_ASSERT(
        find_pattern_datas_.find(patterns_info_full_pair.first) ==
        std::end(find_pattern_datas_));

      ModuleInfo const mod_info = GetModuleInfo(patterns_info_full_pair.first);
      auto const& patterns_info_full = patterns_info_full_pair.second;
      auto const& pattern_infos = patterns_info_full.patterns;
      for (auto const& p : pattern_infos)
      {
        auto const& pat_info = p.pattern;
        std::uint32_t const flags = patterns_info_full.flags | pat_info.flags;
        std::wstring const name = pat_info.name;
        DWORD_PTR const base = mod_info.base;
        void* address = Find(mod_info,
                             patterns_info_full_pair.first,
                             pat_info.data,
                             flags,
                             pat_info.start);

        auto const& manip_list = p.manipulators;
        for (auto const& m : manip_list)
        {
          switch (m.type)
          {
          case ManipInfo::Manipulator::kAdd:
            if (!m.has_operand1 || m.has_operand2)
            {
              HADESMEM_DETAIL_THROW_EXCEPTION(
                Error() << ErrorString(
                             "Invalid manipulator operands for 'Add'."));
            }

            address = Add(base, address, flags, m.operand1);

            break;

          case ManipInfo::Manipulator::kSub:
            if (!m.has_operand1 || m.has_operand2)
            {
              HADESMEM_DETAIL_THROW_EXCEPTION(
                Error() << ErrorString(
                             "Invalid manipulator operands for 'Sub'."));
            }

            address = Sub(base, address, flags, m.operand1);

            break;

          case ManipInfo::Manipulator::kRel:
            if (!m.has_operand1 || !m.has_operand2)
            {
              HADESMEM_DETAIL_THROW_EXCEPTION(
                Error() << ErrorString(
                             "Invalid manipulator operands for 'Rel'."));
            }

            address = Rel(base, address, flags, m.operand1, m.operand2);

            break;

          case ManipInfo::Manipulator::kLea:
            if (m.has_operand1 || m.has_operand2)
            {
              HADESMEM_DETAIL_THROW_EXCEPTION(
                Error() << ErrorString(
                             "Invalid manipulator operands for 'Lea'."));
            }

            address = Lea(base, address, flags);

            break;

          default:
            HADESMEM_DETAIL_THROW_EXCEPTION(
              Error() << ErrorString("Unknown manipulator."));
          }
        }

        find_pattern_datas_[patterns_info_full_pair.first][name] = {address,
                                                                    flags};
      }
    }
  }

  using PatternDataMap = std::map<std::wstring, PatternData>;

  Process const* process_;
  std::map<std::wstring, PatternDataMap> find_pattern_datas_;
};
}
