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

// TODO: Pattern generator.

// TODO: Standalone app/example for FindPattern. For dumping results,
// experimenting with patterns, automatically generating new patterns, etc.

// TODO: Allow custom regions to be specified (similar to module name).

namespace hadesmem
{

struct PatternFlags
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

namespace detail
{

class PatternDataByte
{
public:
  HADESMEM_DETAIL_CONSTEXPR explicit PatternDataByte(std::uint8_t data,
                                                     bool wildcard)
    HADESMEM_DETAIL_NOEXCEPT : data_(data),
                               wildcard_(wildcard)
  {
  }

  HADESMEM_DETAIL_CONSTEXPR std::uint8_t GetData() const
    HADESMEM_DETAIL_NOEXCEPT
  {
    return data_;
  }

  HADESMEM_DETAIL_CONSTEXPR bool IsWildcard() const HADESMEM_DETAIL_NOEXCEPT
  {
    return wildcard_;
  }

private:
  std::uint8_t data_;
  bool wildcard_;
};

inline std::vector<PatternDataByte> ConvertData(std::wstring const& data)
{
  HADESMEM_DETAIL_ASSERT(!data.empty());

  std::wstring const data_trimmed{
    data.substr(0, data.find_last_not_of(L" \n\r\t") + 1)};

  HADESMEM_DETAIL_ASSERT(!data_trimmed.empty());

  std::wistringstream data_str{data_trimmed};
  data_str.imbue(std::locale::classic());
  std::vector<PatternDataByte> data_real;
  do
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

      if (current > static_cast<std::uint8_t>(-1))
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                        << ErrorString("Invalid data."));
      }
    }

    data_real.emplace_back(static_cast<std::uint8_t>(current), is_wildcard);
  } while (!data_str.eof());

  return data_real;
}

template <typename NeedleIterator>
void* FindRaw(Process const& process,
              std::uint8_t* s_beg,
              std::uint8_t* s_end,
              NeedleIterator n_beg,
              NeedleIterator n_end)
{
  HADESMEM_DETAIL_ASSERT(s_beg < s_end);

  std::ptrdiff_t const mem_size = s_end - s_beg;
  std::vector<std::uint8_t> const haystack{ReadVector<std::uint8_t>(
    process, s_beg, static_cast<std::size_t>(mem_size))};

  auto const h_beg = std::begin(haystack);
  auto const h_end = std::end(haystack);
  auto const iter =
    std::search(h_beg,
                h_end,
                n_beg,
                n_end,
                [](std::uint8_t h_cur, detail::PatternDataByte const& n_cur)
  { return (n_cur.IsWildcard()) || (h_cur == n_cur.GetData()); });

  if (iter != h_end)
  {
    return s_beg + std::distance(h_beg, iter);
  }

  return nullptr;
}

struct ModuleRegionInfo
{
  std::uintptr_t base;
  using ScanRegion = std::pair<std::uint8_t*, std::uint8_t*>;
  std::vector<ScanRegion> code_regions;
  std::vector<ScanRegion> data_regions;
};

inline ModuleRegionInfo GetModuleInfo(Process const& process,
                                      std::wstring const& module)
{
  ModuleRegionInfo mod_info;

  if (module.empty())
  {
    Module const module_ex{process, nullptr};
    mod_info.base = reinterpret_cast<std::uintptr_t>(module_ex.GetHandle());
  }
  else
  {
    Module const module_ex{process, module};
    mod_info.base = reinterpret_cast<std::uintptr_t>(module_ex.GetHandle());
  }

  auto const base = reinterpret_cast<std::uint8_t*>(mod_info.base);
  PeFile const pe_file{process, base, hadesmem::PeFileType::Image, 0};
  DosHeader const dos_header{process, pe_file};
  NtHeaders const nt_headers{process, pe_file};
  SectionList const sections{process, pe_file};
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

    auto const section_beg = static_cast<std::uint8_t*>(
      RvaToVa(process, pe_file, s.GetVirtualAddress()));
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

    auto const section_end = section_beg + section_size;

    auto& regions =
      is_code_section ? mod_info.code_regions : mod_info.data_regions;
    regions.emplace_back(section_beg, section_end);
  }

  if (mod_info.code_regions.empty() && mod_info.data_regions.empty())
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      Error() << ErrorString("No valid sections to scan found."));
  }

  return mod_info;
}

template <typename NeedleIterator>
void* Find(Process const& process,
           ModuleRegionInfo::ScanRegion const& region,
           void* start,
           NeedleIterator n_beg,
           NeedleIterator n_end)
{
  std::uint8_t* s_beg = region.first;
  std::uint8_t* const s_end = region.second;

  // Support custom scan start address.
  if (start)
  {
    // Use specified starting address (plus one, so we don't
    // just find the same thing again) if we're in the target
    // region.
    if (start >= s_beg && start < s_end)
    {
      s_beg = static_cast<std::uint8_t*>(start) + 1;
      if (s_beg == s_end)
      {
        HADESMEM_DETAIL_THROW_EXCEPTION(
          Error() << ErrorString("Invalid start address."));
      }
    }
    // Skip if we're not in the target region.
    else
    {
      return nullptr;
    }
  }

  return FindRaw(process, s_beg, s_end, n_beg, n_end);
}

template <typename NeedleIterator>
void* Find(Process const& process,
           ModuleRegionInfo const& mod_info,
           NeedleIterator n_beg,
           NeedleIterator n_end,
           std::uint32_t flags,
           void* start)
{
  HADESMEM_DETAIL_ASSERT(n_beg != n_end);

  bool const scan_data_secs = !!(flags & PatternFlags::kScanData);
  auto const& scan_regions =
    scan_data_secs ? mod_info.data_regions : mod_info.code_regions;
  for (auto const& region : scan_regions)
  {
    if (void* const address = Find(process, region, start, n_beg, n_end))
    {
      return !!(flags & PatternFlags::kRelativeAddress)
               ? static_cast<std::uint8_t*>(address) - mod_info.base
               : address;
    }
  }

  if (!!(flags & PatternFlags::kThrowOnUnmatch))
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                    << ErrorString("Could not match pattern."));
  }

  return nullptr;
}
}

inline void* Find(Process const& process,
                  std::wstring const& module,
                  std::wstring const& data,
                  std::uint32_t flags,
                  std::uintptr_t start)
{
  HADESMEM_DETAIL_ASSERT(
    !(flags & ~(PatternFlags::kInvalidFlagMaxValue - 1UL)));

  auto const mod_info = detail::GetModuleInfo(process, module);
  auto const needle = detail::ConvertData(data);
  void* const start_abs =
    start ? reinterpret_cast<std::uint8_t*>(mod_info.base) + start : nullptr;
  return detail::Find(
    process, mod_info, std::begin(needle), std::end(needle), flags, start_abs);
}

class Pattern
{
public:
  HADESMEM_DETAIL_CONSTEXPR Pattern() HADESMEM_DETAIL_NOEXCEPT
  {
  }

  HADESMEM_DETAIL_CONSTEXPR explicit Pattern(void* address, std::uint32_t flags)
    HADESMEM_DETAIL_NOEXCEPT : address_{address},
                               flags_{flags}
  {
  }

  HADESMEM_DETAIL_CONSTEXPR void* GetAddress() const HADESMEM_DETAIL_NOEXCEPT
  {
    return address_;
  }

  HADESMEM_DETAIL_CONSTEXPR std::uint32_t GetFlags() const
    HADESMEM_DETAIL_NOEXCEPT
  {
    return flags_;
  }

private:
  void* address_{nullptr};
  std::uint32_t flags_{PatternFlags::kNone};
};

inline bool operator==(Pattern const& lhs, Pattern const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetAddress() == rhs.GetAddress() &&
         lhs.GetFlags() == rhs.GetFlags();
}

inline bool operator!=(Pattern const& lhs, Pattern const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return !(lhs == rhs);
}

inline bool operator<(Pattern const& lhs, Pattern const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetAddress() < rhs.GetAddress();
}

inline bool operator<=(Pattern const& lhs, Pattern const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetAddress() <= rhs.GetAddress();
}

inline bool operator>(Pattern const& lhs, Pattern const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetAddress() > rhs.GetAddress();
}

inline bool operator>=(Pattern const& lhs, Pattern const& rhs)
  HADESMEM_DETAIL_NOEXCEPT
{
  return lhs.GetAddress() >= rhs.GetAddress();
}

class PatternMap
{
public:
  using key_type = std::map<std::wstring, Pattern>::key_type;
  using mapped_type = std::map<std::wstring, Pattern>::mapped_type;
  using value_type = std::map<std::wstring, Pattern>::value_type;
  using iterator = std::map<std::wstring, Pattern>::iterator;
  using const_iterator = std::map<std::wstring, Pattern>::const_iterator;
  using size_type = std::map<std::wstring, Pattern>::size_type;

  friend class FindPattern;

  PatternMap() : map_()
  {
  }

  const_iterator find(std::wstring const& name) const
  {
    return map_.find(name);
  }

  Pattern const& at(std::wstring const& name) const
  {
    return map_.at(name);
  }

  size_type size() const
  {
    return map_.size();
  }

  const_iterator begin() const
  {
    return map_.cbegin();
  }

  const_iterator cbegin() const
  {
    return map_.cbegin();
  }

  const_iterator end() const
  {
    return map_.end();
  }

  const_iterator cend() const
  {
    return map_.cend();
  }

  friend inline bool operator==(PatternMap const& lhs, PatternMap const& rhs)
    HADESMEM_DETAIL_NOEXCEPT
  {
    return lhs.map_ == rhs.map_;
  }

  friend inline bool operator!=(PatternMap const& lhs, PatternMap const& rhs)
    HADESMEM_DETAIL_NOEXCEPT
  {
    return !(lhs == rhs);
  }

private:
  Pattern& operator[](std::wstring const& name)
  {
    return map_[name];
  }

  Pattern& operator[](std::wstring&& name)
  {
    return map_[std::move(name)];
  }

  std::map<std::wstring, Pattern> map_;
};

class ModuleMap
{
public:
  using key_type = std::map<std::wstring, PatternMap>::key_type;
  using mapped_type = std::map<std::wstring, PatternMap>::mapped_type;
  using value_type = std::map<std::wstring, PatternMap>::value_type;
  using iterator = std::map<std::wstring, PatternMap>::iterator;
  using const_iterator = std::map<std::wstring, PatternMap>::const_iterator;
  using size_type = std::map<std::wstring, PatternMap>::size_type;

  friend class FindPattern;

  ModuleMap() : map_()
  {
  }

  const_iterator find(std::wstring const& name) const
  {
    return map_.find(name);
  }

  PatternMap const& at(std::wstring const& name) const
  {
    return map_.at(name);
  }

  size_type size() const
  {
    return map_.size();
  }

  const_iterator begin() const
  {
    return map_.cbegin();
  }

  const_iterator cbegin() const
  {
    return map_.cbegin();
  }

  const_iterator end() const
  {
    return map_.end();
  }

  const_iterator cend() const
  {
    return map_.cend();
  }

  friend inline bool operator==(ModuleMap const& lhs, ModuleMap const& rhs)
    HADESMEM_DETAIL_NOEXCEPT
  {
    return lhs.map_ == rhs.map_;
  }

  friend inline bool operator!=(ModuleMap const& lhs, ModuleMap const& rhs)
    HADESMEM_DETAIL_NOEXCEPT
  {
    return !(lhs == rhs);
  }

private:
  PatternMap& operator[](std::wstring const& name)
  {
    return map_[name];
  }

  PatternMap& operator[](std::wstring&& name)
  {
    return map_[std::move(name)];
  }

  std::map<std::wstring, PatternMap> map_;
};

class FindPattern
{
public:
  explicit FindPattern(Process const& process,
                       std::wstring const& pattern_file,
                       bool in_memory_file)
    : process_{&process}, find_pattern_datas_{}
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
    : process_{other.process_},
      find_pattern_datas_{std::move(other.find_pattern_datas_)}
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

  ModuleMap const& GetModuleMap() const HADESMEM_DETAIL_NOEXCEPT
  {
    return find_pattern_datas_;
  }

  PatternMap const& GetPatternMap(std::wstring const& module) const
  {
    try
    {
      return find_pattern_datas_.at(detail::ToUpperOrdinal(module));
    }
    catch (std::out_of_range const&)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                      << ErrorString("Invalid module name."));
    }
  }

  void* Lookup(std::wstring const& module, std::wstring const& name) const
  {
    return LookupEx(module, name).GetAddress();
  }

  friend bool operator==(FindPattern const& lhs, FindPattern const& rhs)
  {
    return lhs.process_ == rhs.process_ &&
           lhs.find_pattern_datas_ == rhs.find_pattern_datas_;
  }

  friend bool operator!=(FindPattern const& lhs, FindPattern const& rhs)
  {
    return !(lhs == rhs);
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

  Pattern LookupEx(std::wstring const& module, std::wstring const& name) const
  {
    auto const& pattern_map = GetPatternMap(module);
    try
    {
      return pattern_map.at(name);
    }
    catch (std::out_of_range const&)
    {
      HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                      << ErrorString("Invalid pattern name."));
    }
  }

  struct PatternInfo
  {
    std::wstring name;
    std::wstring data;
    std::wstring start;
    std::wstring start_rva;
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

  inline void* Add(std::uintptr_t /*base*/,
                   void* address,
                   std::uint32_t /*flags*/,
                   std::uintptr_t offset) const
  {
    return static_cast<std::uint8_t*>(address) + offset;
  }

  inline void* Sub(std::uintptr_t /*base*/,
                   void* address,
                   std::uint32_t /*flags*/,
                   std::uintptr_t offset) const
  {
    return static_cast<std::uint8_t*>(address) - offset;
  }

  inline void*
    Lea(std::uintptr_t base, void* address, std::uint32_t flags) const
  {
    try
    {
      bool const is_relative_address =
        !!(flags & PatternFlags::kRelativeAddress);
      std::uintptr_t real_base = is_relative_address ? base : 0;
      auto const real_address = static_cast<std::uint8_t*>(address) + real_base;
      std::uint8_t* const result = Read<std::uint8_t*>(*process_, real_address);
      return is_relative_address ? result - base : result;
    }
    catch (std::exception const& /*e*/)
    {
      return nullptr;
    }
  }

  inline void* Rel(std::uintptr_t base,
                   void* address,
                   std::uint32_t flags,
                   std::uintptr_t size,
                   std::uintptr_t offset) const
  {
    try
    {
      bool const is_relative_address =
        !!(flags & PatternFlags::kRelativeAddress);
      std::uintptr_t const real_base = is_relative_address ? base : 0;
      auto const real_address = static_cast<std::uint8_t*>(address) + real_base;
      auto const result = Read<std::uint8_t*>(*process_, real_address) +
                          reinterpret_cast<std::uintptr_t>(real_address) +
                          size - offset;
      return is_relative_address ? result - base : result;
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
    std::uint32_t flags = PatternFlags::kNone;
    for (auto const& flag : node.children(L"Flag"))
    {
      auto const flag_name = GetAttributeValue(flag, L"Name");

      if (flag_name == L"None")
      {
        flags |= PatternFlags::kNone;
      }
      else if (flag_name == L"ThrowOnUnmatch")
      {
        flags |= PatternFlags::kThrowOnUnmatch;
      }
      else if (flag_name == L"RelativeAddress")
      {
        flags |= PatternFlags::kRelativeAddress;
      }
      else if (flag_name == L"ScanData")
      {
        flags |= PatternFlags::kScanData;
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

        auto const pattern_start_rva =
          GetOptionalAttributeValue(pattern, L"StartRVA");

        std::uint32_t const pattern_flags = ReadFlags(pattern);

        PatternInfo pattern_info{pattern_name,      pattern_data, pattern_start,
                                 pattern_start_rva, pattern_flags};

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

          auto const manipulator_operand1 = manipulator.attribute(L"Operand1");
          bool const has_operand1 = !!manipulator_operand1;
          std::uintptr_t const operand1 =
            has_operand1 ? detail::HexStrToPtr(manipulator_operand1.value())
                         : 0U;

          auto const manipulator_operand2 = manipulator.attribute(L"Operand2");
          bool const has_operand2 = !!manipulator_operand2;
          std::uintptr_t const operand2 =
            has_operand2 ? detail::HexStrToPtr(manipulator_operand2.value())
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

  inline void* ApplyManipulators(void* address,
                                 std::uint32_t flags,
                                 std::uintptr_t base,
                                 std::vector<ManipInfo> const& manip_list) const
  {
    for (auto const& m : manip_list)
    {
      switch (m.type)
      {
      case ManipInfo::Manipulator::kAdd:
        if (!m.has_operand1 || m.has_operand2)
        {
          HADESMEM_DETAIL_THROW_EXCEPTION(
            Error() << ErrorString("Invalid manipulator operands for 'Add'."));
        }

        address = Add(base, address, flags, m.operand1);

        break;

      case ManipInfo::Manipulator::kSub:
        if (!m.has_operand1 || m.has_operand2)
        {
          HADESMEM_DETAIL_THROW_EXCEPTION(
            Error() << ErrorString("Invalid manipulator operands for 'Sub'."));
        }

        address = Sub(base, address, flags, m.operand1);

        break;

      case ManipInfo::Manipulator::kRel:
        if (!m.has_operand1 || !m.has_operand2)
        {
          HADESMEM_DETAIL_THROW_EXCEPTION(
            Error() << ErrorString("Invalid manipulator operands for 'Rel'."));
        }

        address = Rel(base, address, flags, m.operand1, m.operand2);

        break;

      case ManipInfo::Manipulator::kLea:
        if (m.has_operand1 || m.has_operand2)
        {
          HADESMEM_DETAIL_THROW_EXCEPTION(
            Error() << ErrorString("Invalid manipulator operands for 'Lea'."));
        }

        address = Lea(base, address, flags);

        break;

      default:
        HADESMEM_DETAIL_THROW_EXCEPTION(Error()
                                        << ErrorString("Unknown manipulator."));

        break;
      }
    }

    return address;
  }

  inline std::uintptr_t GetStartRva(std::wstring const& module,
                                    std::uintptr_t base,
                                    std::wstring const& start) const
  {
    std::uintptr_t start_rva = 0U;
    if (!start.empty())
    {
      Pattern const start_pattern = LookupEx(module, start);
      start_rva = reinterpret_cast<std::uintptr_t>(start_pattern.GetAddress());
      if (!(start_pattern.GetFlags() & PatternFlags::kRelativeAddress))
      {
        start_rva -= base;
      }
    }

    return start_rva;
  }

  inline void LoadPatternFileImpl(pugi::xml_document const& doc)
  {
    auto const patterns_info_full_list = ReadPatternsFromXml(doc);
    for (auto const& patterns_info_full_pair : patterns_info_full_list)
    {
      HADESMEM_DETAIL_ASSERT(
        find_pattern_datas_.find(patterns_info_full_pair.first) ==
        std::end(find_pattern_datas_));

      auto const mod_info =
        detail::GetModuleInfo(*process_, patterns_info_full_pair.first);
      auto const& module = patterns_info_full_pair.first;
      auto const& patterns_info_full = patterns_info_full_pair.second;
      auto const& pattern_infos = patterns_info_full.patterns;
      for (auto const& p : pattern_infos)
      {
        std::uint32_t const flags = patterns_info_full.flags | p.pattern.flags;
        void* address = nullptr;
        if (!p.pattern.start_rva.empty())
        {
          std::uintptr_t const start_rva =
            detail::HexStrToPtr(p.pattern.start_rva);
          address = ::hadesmem::Find(
            *process_, module, p.pattern.data, flags, start_rva);
        }
        else
        {
          std::uintptr_t const start_rva =
            GetStartRva(module, mod_info.base, p.pattern.start);
          address = ::hadesmem::Find(
            *process_, module, p.pattern.data, flags, start_rva);
        }

        if (address)
        {
          address =
            ApplyManipulators(address, flags, mod_info.base, p.manipulators);
        }

        find_pattern_datas_[patterns_info_full_pair.first][p.pattern.name] =
          Pattern{address, flags};
      }
    }
  }

  Process const* process_;
  ModuleMap find_pattern_datas_;
};
}
