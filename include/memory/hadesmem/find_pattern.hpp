// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <map>
#include <limits>
#include <locale>
#include <string>
#include <vector>
#include <utility>
#include <iterator>
#include <algorithm>

#include <hadesmem/detail/warning_disable_prefix.hpp>
#include <boost/variant.hpp>
#include <boost/filesystem.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi_uint.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <hadesmem/detail/warning_disable_suffix.hpp>

#include <Windows.h>

#include <hadesmem/read.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/module_list.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/pelib/pe_file.hpp>
#include <hadesmem/pelib/section.hpp>
#include <hadesmem/pelib/dos_header.hpp>
#include <hadesmem/pelib/nt_headers.hpp>
#include <hadesmem/pelib/section_list.hpp>
#include <hadesmem/detail/static_assert.hpp>

// TODO: Review, refactor, rewrite, etc this entire module. Put TODOs where 
// appropriate, remove and add apis, fix bugs, clean up code, etc. Use new 
// lanaguage features like noexcept, constexpr, etc.

// TODO: Add stream overloads.

// TODO: Rewrite to not use virtual functions.

// TODO: Rewrite to remove cyclic dependencies.

// TODO: Pattern generator.

// TODO: Multi-pass’ support (e.g. search for pattern, apply for manipulators, use as starting point for second search).

// TODO: Arbitrary region support.

// Clang generates a warning for all inline classes with virtual methods, due 
// to the potential object file bloat it may cause.
// error: 'Manipulator' has no out-of-line virtual method definitions; its 
// vtable will be emitted in every translation unit
// TODO: Fix the code and remove this suppression.
#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wweak-vtables"
#endif // #if defined(HADESMEM_CLANG)

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
  
  int type;
  // TODO: Use a DWORD_PTR here.
  std::vector<unsigned long> operands;
};

struct PatternInfoFull
{
  PatternInfo pattern;
  std::vector<ManipInfo> manipulators;
};

}

}

BOOST_FUSION_ADAPT_STRUCT(hadesmem::detail::PatternInfo, 
  (std::wstring, name)
  (std::wstring, data))

BOOST_FUSION_ADAPT_STRUCT(hadesmem::detail::ManipInfo, 
  (int, type)
  (std::vector<unsigned long>, operands))

BOOST_FUSION_ADAPT_STRUCT(hadesmem::detail::PatternInfoFull, 
  (hadesmem::detail::PatternInfo, pattern)
  (std::vector<hadesmem::detail::ManipInfo>, manipulators))

namespace hadesmem
{

struct FindPatternFlags
{
  enum
  {
    kNone = 0, 
    kThrowOnUnmatch = 1 << 0, 
    kRelativeAddress = 1 << 1, 
    kScanData = 1 << 2, 
  };
};

class FindPattern;

class Pattern
{
public:
  Pattern(FindPattern& finder, std::wstring const& data, int flags);
  
  Pattern(FindPattern& finder, std::wstring const& data, 
    std::wstring const& name, int flags);
  
  Pattern(Pattern const& other);

  Pattern& operator=(Pattern const& other);
  
  Pattern(Pattern&& other) HADESMEM_NOEXCEPT;
  
  Pattern& operator=(Pattern&& other) HADESMEM_NOEXCEPT;
  
  ~Pattern() HADESMEM_NOEXCEPT;
  
  void Save();
  
  // TODO: More consistent naming.
  void Update(PBYTE address);
  
  PBYTE GetAddress() const HADESMEM_NOEXCEPT;
  
  int GetFlags() const HADESMEM_NOEXCEPT;
  
  DWORD_PTR GetBase() const HADESMEM_NOEXCEPT;

  Process const* GetProcess() const HADESMEM_NOEXCEPT;
    
private:
  FindPattern* finder_;
  std::wstring name_;
  PBYTE address_;
  int flags_;
};

namespace pattern_manipulators
{

class Manipulator
{
public:
  // TODO: Should this be pure virtual?
  virtual void Manipulate(Pattern& /*pattern*/) const;

  virtual ~Manipulator() HADESMEM_NOEXCEPT;

  friend Pattern& operator<< (Pattern& pattern, 
    Manipulator const& manipulator);
};

class Save : public Manipulator
{
public:
  virtual void Manipulate(Pattern& pattern) const;
};

class Add : public Manipulator
{
public:
  explicit Add(DWORD_PTR offset) HADESMEM_NOEXCEPT;
  
  virtual void Manipulate(Pattern& pattern) const;
  
private:
  DWORD_PTR offset_;
};

class Sub : public Manipulator
{
public:
  explicit Sub(DWORD_PTR offset) HADESMEM_NOEXCEPT;
  
  virtual void Manipulate(Pattern& pattern) const;
    
private:
  DWORD_PTR offset_;
};

class Lea : public Manipulator
{
public:
  virtual void Manipulate(Pattern& pattern) const;
};

class Rel : public Manipulator
{
public:
  Rel(DWORD_PTR size, DWORD_PTR offset) HADESMEM_NOEXCEPT;
  
  virtual void Manipulate(Pattern& pattern) const;
  
private:
  DWORD_PTR size_;
  DWORD_PTR offset_;
};

}

class FindPattern
{
public:
  friend class Pattern;
  
  FindPattern(Process const& process, HMODULE module)
    : process_(&process), 
    base_(0), 
    code_regions_(), 
    data_regions_(), 
    addresses_()
  {
    if (!module)
    {
      ModuleList modules(process);
      module = std::begin(modules)->GetHandle();
    }
    
    PBYTE const base = reinterpret_cast<PBYTE>(module);
    base_ = reinterpret_cast<DWORD_PTR>(base);
    PeFile const pe_file(process, reinterpret_cast<PVOID>(base), 
      hadesmem::PeFileType::Image);
    DosHeader const dos_header(process, pe_file);
    NtHeaders const nt_headers(process, pe_file);
    
    SectionList sections(process, pe_file);
    for (auto const& s : sections)
    {
      bool is_code_section = 
        ((s.GetCharacteristics() & IMAGE_SCN_CNT_CODE) == IMAGE_SCN_CNT_CODE);
      bool is_data_section = 
        ((s.GetCharacteristics() & IMAGE_SCN_CNT_INITIALIZED_DATA) == 
        IMAGE_SCN_CNT_INITIALIZED_DATA);
      if (is_code_section || is_data_section)
      {
        PBYTE const section_beg = static_cast<PBYTE>(RvaToVa(process, 
          pe_file, s.GetVirtualAddress()));
        if (section_beg == nullptr)
        {
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorString("Could not get section base address."));
        }
          
        PBYTE const section_end = section_beg + s.GetSizeOfRawData();
        
        if (is_code_section)
        {
          code_regions_.emplace_back(std::make_pair(section_beg, section_end));
        }
        else
        {
          data_regions_.emplace_back(std::make_pair(section_beg, section_end));
        }
      }
    }
    
    if (code_regions_.empty() && data_regions_.empty())
    {
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorString("No valid sections to scan found."));
    }
  }
  
  FindPattern(FindPattern const& other)
    : process_(other.process_), 
    base_(other.base_), 
    code_regions_(other.code_regions_), 
    data_regions_(other.data_regions_), 
    addresses_(other.addresses_)
  { }
  
  FindPattern& operator=(FindPattern const& other)
  {
    process_ = other.process_;
    base_ = other.base_;
    code_regions_ = other.code_regions_;
    data_regions_ = other.data_regions_;
    addresses_ = other.addresses_;
    return *this;
  }
    
  FindPattern(FindPattern&& other) HADESMEM_NOEXCEPT
    : process_(other.process_), 
    base_(other.base_), 
    code_regions_(std::move(other.code_regions_)), 
    data_regions_(std::move(other.data_regions_)), 
    addresses_(std::move(other.addresses_))
  {
    other.process_ = nullptr;
    other.base_ = 0;
  }
    
  FindPattern& operator=(FindPattern&& other) HADESMEM_NOEXCEPT
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
    
  ~FindPattern() HADESMEM_NOEXCEPT
  { }
  
  PVOID Find(std::wstring const& data, int flags) const
  {
    typedef std::wstring::const_iterator DataIter;
    typedef boost::spirit::qi::standard::space_type SkipWsT;
    
    boost::spirit::qi::rule<DataIter, unsigned int(), SkipWsT> data_rule;
    data_rule %= (boost::spirit::qi::hex | 
      boost::spirit::qi::lit(L"??")[boost::spirit::qi::_val = 
      static_cast<unsigned int>(-1)]);
    
    boost::spirit::qi::rule<DataIter, std::vector<unsigned int>(), SkipWsT> 
      data_list_rule = +(data_rule);
    
    std::vector<unsigned int> data_parsed;
    auto data_beg = std::begin(data);
    auto data_end = std::end(data);
    bool const converted = boost::spirit::qi::phrase_parse(
      data_beg, data_end, 
      data_list_rule, 
      boost::spirit::qi::space, 
      data_parsed);
    if (!converted || data_beg != data_end)
    {
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorString("Data parsing failed."));
    }
    
    std::vector<std::pair<BYTE, bool>> data_real;
    std::transform(std::begin(data_parsed), std::end(data_parsed), 
      std::back_inserter(data_real), 
      [] (unsigned int current) -> std::pair<BYTE, bool>
      {
        bool const is_wildcard = (current == static_cast<unsigned int>(-1));
        
        BYTE current_byte = 0;
        if (!is_wildcard)
        {
          try
          {
            current_byte = boost::numeric_cast<BYTE>(current);
          }
          catch (std::exception const& /*e*/)
          {
            BOOST_THROW_EXCEPTION(Error() << 
              ErrorString("Data conversion failed (numeric)."));
          }
        }
        
        return std::make_pair(current_byte, !is_wildcard);
      });
    
    bool const scan_data_secs = ((flags & FindPatternFlags::kScanData) == 
      FindPatternFlags::kScanData);
    
    PVOID address = Find(data_real, scan_data_secs);
    
    if (!address && ((flags & FindPatternFlags::kThrowOnUnmatch) == 
      FindPatternFlags::kThrowOnUnmatch))
    {
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorString("Could not match pattern."));
    }
    
    if (address && ((flags & FindPatternFlags::kRelativeAddress) == 
      FindPatternFlags::kRelativeAddress))
    {
      address = static_cast<PBYTE>(address) - base_;
    }
    
    return address;
  }
  
  PVOID Find(std::wstring const& data, std::wstring const& name, int flags)
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
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorString("Could not find target pattern."));
    }
    return iter->second;
  }
      
  void LoadFile(std::wstring const& path)
  {
    boost::filesystem::wifstream pattern_file(path);
    if (!pattern_file)
    {
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorString("Could not open pattern file."));
    }
    
    std::istreambuf_iterator<wchar_t> const pat_file_beg(pattern_file);
    std::istreambuf_iterator<wchar_t> const pat_file_end;
    std::vector<wchar_t> pat_file_buf(pat_file_beg, pat_file_end);
    pat_file_buf.push_back(L'\0');
    
    LoadFileMemory(pat_file_buf.data());
  }
    
  void LoadFileMemory(std::wstring const& data)
  {
    typedef std::wstring::const_iterator DataIter;
    typedef boost::spirit::qi::standard::space_type SkipWsT;
    
    typedef boost::spirit::qi::symbols<wchar_t, int> FlagsParser;
    FlagsParser flags_parser;
    flags_parser.add
      (L"None", FindPatternFlags::kNone)
      (L"ThrowOnUnmatch", FindPatternFlags::kThrowOnUnmatch)
      (L"RelativeAddress", FindPatternFlags::kRelativeAddress)
      (L"ScanData", FindPatternFlags::kScanData);
    
    boost::spirit::qi::rule<DataIter, std::vector<int>(), SkipWsT> flags_rule = 
      '(' >> *(flags_parser % ',') >> ')';
    boost::spirit::qi::rule<DataIter, std::wstring()> name_rule = 
      boost::spirit::qi::lexeme[*(~boost::spirit::qi::char_(','))] >> ',';    
    boost::spirit::qi::rule<DataIter, std::wstring()> data_rule = 
      boost::spirit::qi::lexeme[*(~boost::spirit::qi::char_('}'))];    
    boost::spirit::qi::rule<DataIter, detail::PatternInfo(), SkipWsT> pattern_rule = 
      '{' >> name_rule >> data_rule >> '}';
    
    typedef boost::spirit::qi::symbols<wchar_t, int> ManipParser;
    ManipParser manip_parser;
    manip_parser.add
      (L"Add", detail::ManipInfo::Manipulator::kAdd)
      (L"Sub", detail::ManipInfo::Manipulator::kSub)
      (L"Rel", detail::ManipInfo::Manipulator::kRel)
      (L"Lea", detail::ManipInfo::Manipulator::kLea);
    
    boost::spirit::qi::rule<DataIter, int(), SkipWsT> manip_name_rule = 
      manip_parser >> ',';
    // TODO: Use a DWORD_PTR here.
    boost::spirit::qi::rule<DataIter, std::vector<unsigned long>(), SkipWsT> 
      operand_rule = (boost::spirit::ulong_ % ',');
    boost::spirit::qi::rule<DataIter, detail::ManipInfo(), SkipWsT> 
      manip_rule = ('[' >> manip_name_rule >> operand_rule >> ']');
    boost::spirit::qi::rule<DataIter, detail::PatternInfoFull(), SkipWsT> 
      pattern_full_rule = (pattern_rule >> *manip_rule);
    
    std::vector<int> flags_list;
    std::vector<detail::PatternInfoFull> pattern_list;
    
    auto data_beg = std::begin(data);
    auto data_end = std::end(data);
    bool const parsed = boost::spirit::qi::phrase_parse(data_beg, data_end, 
      (
        L"HadesMem Patterns" >> flags_rule >> 
        *pattern_full_rule
      ), 
      boost::spirit::qi::space, 
      flags_list, 
      pattern_list);
    if (!parsed || data_beg != data_end)
    {
      BOOST_THROW_EXCEPTION(Error() << 
        ErrorString("Parsing failed."));
    }
    
    int flags = FindPatternFlags::kNone;
    std::for_each(std::begin(flags_list), std::end(flags_list), 
      [&] (int flag)
      {
        flags |= flag;
      });
    
    for (auto const& p : pattern_list)
    {
      detail::PatternInfo const& pat_info = p.pattern;
      Pattern pattern(*this, pat_info.data, pat_info.name, flags);
      
      std::vector<detail::ManipInfo> const& manip_list = p.manipulators;
      for (auto const& m : manip_list)
      {
        switch (m.type)
        {
        case detail::ManipInfo::Manipulator::kAdd:
          if (m.operands.size() != 1)
          {
            BOOST_THROW_EXCEPTION(Error() << 
              ErrorString("Invalid manipulator operands for 'Add'."));
          }
          
          pattern << pattern_manipulators::Add(m.operands[0]);
          
          break;
          
        case detail::ManipInfo::Manipulator::kSub:
          if (m.operands.size() != 1)
          {
            BOOST_THROW_EXCEPTION(Error() << 
              ErrorString("Invalid manipulator operands for 'Sub'."));
          }
          
          pattern << pattern_manipulators::Sub(m.operands[0]);
          
          break;
          
        case detail::ManipInfo::Manipulator::kRel:
          if (m.operands.size() != 2)
          {
            BOOST_THROW_EXCEPTION(Error() << 
              ErrorString("Invalid manipulator operands for 'Rel'."));
          }
          
          pattern << pattern_manipulators::Rel(m.operands[0], 
            m.operands[1]);
          
          break;
          
        case detail::ManipInfo::Manipulator::kLea:
          if (m.operands.size() != 0)
          {
            BOOST_THROW_EXCEPTION(Error() << 
              ErrorString("Invalid manipulator operands for 'Lea'."));
          }
          
          pattern << pattern_manipulators::Lea();
          
          break;
          
        default:
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorString("Unknown manipulator."));
        }
      }
      
      pattern.Save();
    }
  }
    
  bool operator==(FindPattern const& other) const
  {
    return process_ == other.process_ && 
      base_ == other.base_ && 
      addresses_ == other.addresses_;
  }
    
  bool operator!=(FindPattern const& other) const
  {
    return !(*this == other);
  }

private:
  PVOID Find(std::vector<std::pair<BYTE, bool>> const& data, 
    bool scan_data_secs) const
  {
    HADESMEM_ASSERT(!data.empty());
    
    std::vector<std::pair<PBYTE, PBYTE>> const& scan_regions = 
      scan_data_secs ? data_regions_ : code_regions_;
    for (auto i = std::begin(scan_regions); i != std::end(scan_regions); ++i)
    {
      PBYTE const s_beg = i->first;
      PBYTE const s_end = i->second;
      HADESMEM_ASSERT(s_end > s_beg);
      
      std::ptrdiff_t const mem_size = s_end - s_beg;
      HADESMEM_ASSERT(s_beg <= s_end);
      std::vector<BYTE> const buffer(ReadVector<BYTE>(*process_, s_beg, 
        static_cast<std::size_t>(mem_size)));
      
      auto const iter = std::search(std::begin(buffer), std::end(buffer), 
        std::begin(data), std::end(data), 
        [] (BYTE h_cur, std::pair<BYTE, bool> const& n_cur)
        {
          return (!n_cur.second) || (h_cur == n_cur.first);
        });
      
      if (iter != buffer.cend())
      {
        return (s_beg + std::distance(std::begin(buffer), iter));
      }
    }
    
    return nullptr;
  }

  Process const* process_;
  DWORD_PTR base_;
  std::vector<std::pair<PBYTE, PBYTE>> code_regions_;
  std::vector<std::pair<PBYTE, PBYTE>> data_regions_;
  std::map<std::wstring, PVOID> addresses_;
};

inline Pattern::Pattern(FindPattern& finder, std::wstring const& data, 
  int flags)
  : finder_(&finder), 
  name_(), 
  address_(static_cast<PBYTE>(finder.Find(data, flags))), 
  flags_(flags)
{ }

inline Pattern::Pattern(FindPattern& finder, std::wstring const& data, 
  std::wstring const& name, int flags)
  : finder_(&finder), 
  name_(name), 
  address_(static_cast<PBYTE>(finder.Find(data, flags))), 
  flags_(flags)
{ }
  
inline Pattern::Pattern(Pattern const& other)
  : finder_(other.finder_), 
  name_(other.name_), 
  address_(other.address_), 
  flags_(other.flags_)
{ }

inline Pattern& Pattern::operator=(Pattern const& other)
{
  finder_ = other.finder_;
  name_ = other.name_;
  address_ = other.address_;
  flags_ = other.flags_;
  return *this;
}
    
inline Pattern::Pattern(Pattern&& other) HADESMEM_NOEXCEPT
  : finder_(other.finder_), 
  name_(std::move(other.name_)), 
  address_(other.address_), 
  flags_(other.flags_)
{
  other.finder_ = nullptr;
  other.address_ = nullptr;
  other.flags_ = FindPatternFlags::kNone;
}
    
inline Pattern& Pattern::operator=(Pattern&& other) HADESMEM_NOEXCEPT
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
    
inline Pattern::~Pattern() HADESMEM_NOEXCEPT
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
  
// TODO: More consistent naming.
inline void Pattern::Update(PBYTE address)
{
  address_ = address;
}
    
inline PBYTE Pattern::GetAddress() const HADESMEM_NOEXCEPT
{
  return address_;
}

inline int Pattern::GetFlags() const HADESMEM_NOEXCEPT
{
  return flags_;
}
    
inline DWORD_PTR Pattern::GetBase() const HADESMEM_NOEXCEPT
{
  // TODO: This feels like a hack. Investigate and fix this. (And if 
  // appropriate, remove friendship requirement.)
  return finder_->base_;
}

// TODO: This feels like a hack. Investigate and fix this.
inline Process const* Pattern::GetProcess() const HADESMEM_NOEXCEPT
{
  return finder_->process_;
}

namespace pattern_manipulators
{

// TODO: Should this be pure virtual?
inline void Manipulator::Manipulate(Pattern& /*pattern*/) const
{ }

inline Manipulator::~Manipulator() HADESMEM_NOEXCEPT
{ }

inline void Save::Manipulate(Pattern& pattern) const
{
  pattern.Save();
}

inline Add::Add(DWORD_PTR offset) HADESMEM_NOEXCEPT
  : offset_(offset)
{ }

inline void Add::Manipulate(Pattern& pattern) const
{
  PBYTE address = pattern.GetAddress();
  if (address)
  {
    pattern.Update(address + offset_);
  }
}

inline Sub::Sub(DWORD_PTR offset) HADESMEM_NOEXCEPT
  : offset_(offset)
{ }

inline void Sub::Manipulate(Pattern& pattern) const
{
  PBYTE address = pattern.GetAddress();
  if (address)
  {
    pattern.Update(address - offset_);
  }
}

inline void Lea::Manipulate(Pattern& pattern) const
{
  PBYTE address = pattern.GetAddress();
  if (address)
  {
    try
    {
      bool is_relative_address = 
        (pattern.GetFlags() & FindPatternFlags::kRelativeAddress) == 
        FindPatternFlags::kRelativeAddress;
      DWORD_PTR base = is_relative_address ? pattern.GetBase() : 0;
      address = Read<PBYTE>(*pattern.GetProcess(), pattern.GetAddress() + base);
      pattern.Update(address);
    }
    catch (std::exception const& /*e*/)
    {
      pattern.Update(nullptr);
    }
  }
}

inline Rel::Rel(DWORD_PTR size, DWORD_PTR offset) HADESMEM_NOEXCEPT
  : size_(size), 
  offset_(offset)
{ }

inline void Rel::Manipulate(Pattern& pattern) const
{
  PBYTE address = pattern.GetAddress();
  if (address)
  {
    try
    {
      bool is_relative_address = 
        (pattern.GetFlags() & FindPatternFlags::kRelativeAddress) == 
        FindPatternFlags::kRelativeAddress;
      DWORD_PTR base = is_relative_address ? pattern.GetBase() : 0;
      address = Read<PBYTE>(*pattern.GetProcess(), pattern.GetAddress() + base) + 
        reinterpret_cast<DWORD_PTR>(address + base) + size_ - offset_;
      pattern.Update(address);
    }
    catch (std::exception const& /*e*/)
    {
      pattern.Update(nullptr);
    }
  }
}

inline Pattern& operator<< (Pattern& pattern, 
  Manipulator const& manipulator)
{
  manipulator.Manipulate(pattern);
  return pattern;
}

}

}

#if defined(HADESMEM_CLANG)
#pragma GCC diagnostic pop
#endif // #if defined(HADESMEM_CLANG)
