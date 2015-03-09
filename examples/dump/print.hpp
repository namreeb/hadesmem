// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstddef>
#include <iomanip>
#include <locale>
#include <ostream>
#include <string>
#include <vector>

template <typename CharT> class StreamFlagSaver
{
public:
  explicit StreamFlagSaver(std::basic_ios<CharT>& str)
    : str_(&str), flags_(str.flags()), width_(str.width()), fill_(str.fill())
  {
  }

  ~StreamFlagSaver()
  {
    str_->flags(flags_);
    str_->width(width_);
    str_->fill(fill_);
  }

  StreamFlagSaver(StreamFlagSaver const&) = delete;
  StreamFlagSaver& operator=(StreamFlagSaver const&) = delete;

private:
  std::basic_ios<CharT>* str_;
  std::ios_base::fmtflags flags_;
  std::streamsize width_;
  CharT fill_;
};

template <typename T>
inline void WriteNamedHex(std::wostream& out,
                          std::wstring const& name,
                          T const& num,
                          std::size_t tabs)
{
  StreamFlagSaver<wchar_t> flags(out);
  out << std::wstring(tabs, '\t') << name << ": 0x" << std::hex
      << std::setw(sizeof(num) * 2) << std::setfill(L'0') << num << '\n';
}

template <typename T>
inline void WriteNamedHexSuffix(std::wostream& out,
                                std::wstring const& name,
                                T const& num,
                                std::wstring const& suffix,
                                std::size_t tabs)
{
  StreamFlagSaver<wchar_t> flags(out);
  out << std::wstring(tabs, '\t') << name << ": 0x" << std::hex
      << std::setw(sizeof(num) * 2) << std::setfill(L'0') << num << L" ("
      << suffix << L")" << '\n';
}

template <typename C>
inline void WriteNamedHexContainer(std::wostream& out,
                                   std::wstring const& name,
                                   C const& c,
                                   std::size_t tabs)
{
  StreamFlagSaver<wchar_t> flags(out);
  out << std::wstring(tabs, '\t') << name << ": " << std::hex
      << std::setw(sizeof(typename C::value_type) * 2) << std::setfill(L'0');
  for (auto const& e : c)
  {
    out << " 0x" << e;
  }
  out << '\n';
}

template <typename T>
inline void WriteNamedNormal(std::wostream& out,
                             std::wstring const& name,
                             T const& t,
                             std::size_t tabs)
{
  StreamFlagSaver<wchar_t> flags(out);
  out << std::wstring(tabs, '\t') << name << ": " << t << '\n';
}

template <typename T>
inline void WriteNormal(std::wostream& out, T const& t, std::size_t tabs)
{
  StreamFlagSaver<wchar_t> flags(out);
  out << std::wstring(tabs, '\t') << t << '\n';
}

inline void WriteNewline(std::wostream& out)
{
  out << L'\n';
}
