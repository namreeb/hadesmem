// Copyright (C) 2010-2013 Joshua Boyce.
// See the file COPYING for copying permission.

#pragma once

#include <cstddef>
#include <ostream>
#include <string>

namespace hadesmem
{
class Process;
class PeFile;
}

void DumpPeFile(hadesmem::Process const& process,
                hadesmem::PeFile const& pe_file,
                std::wstring const& path);

enum class WarningType : int
{
  kSuspicious,
  kUnsupported,
  kAll = -1
};

void SetCurrentFilePath(std::wstring const& path);

void WarnForCurrentFile(WarningType warned_type);

void ClearWarnForCurrentFile();

class StreamFlagSaver
{
public:
  explicit StreamFlagSaver(std::ios_base& str) : str_(&str), flags_(str.flags())
  {
  }

  ~StreamFlagSaver()
  {
    str_->flags(flags_);
  }

  StreamFlagSaver(StreamFlagSaver const&) = delete;
  StreamFlagSaver& operator=(StreamFlagSaver const&) = delete;

private:
  std::ios_base* str_;
  std::ios_base::fmtflags flags_;
};

template <typename T>
inline void WriteNamedHex(std::wostream& out,
                          std::wstring const& name,
                          T const& num,
                          std::size_t tabs)
{
  StreamFlagSaver flags(out);
  out << std::wstring(tabs, '\t') << name << ": " << std::hex << num << '\n';
}

template <typename T>
inline void WriteNamedHexSuffix(std::wostream& out,
                                std::wstring const& name,
                                T const& num,
                                std::wstring const& suffix,
                                std::size_t tabs)
{
  StreamFlagSaver flags(out);
  out << std::wstring(tabs, '\t') << name << ": 0x" << std::hex << num << L" ("
      << suffix << L")" << '\n';
}

template <typename C>
inline void WriteNamedHexContainer(std::wostream& out,
                                   std::wstring const& name,
                                   C const& c,
                                   std::size_t tabs)
{
  StreamFlagSaver flags(out);
  out << std::wstring(tabs, '\t') << name << ":" << std::hex;
  for (auto const& e : c)
  {
    out << ' ' << e;
  }
  out << '\n';
}

template <typename T>
inline void WriteNamedNormal(std::wostream& out,
                             std::wstring const& name,
                             T const& t,
                             std::size_t tabs)
{
  StreamFlagSaver flags(out);
  out << std::wstring(tabs, '\t') << name << ": " << t << '\n';
}

template <typename T>
inline void WriteNormal(std::wostream& out, T const& t, std::size_t tabs)
{
  StreamFlagSaver flags(out);
  out << std::wstring(tabs, '\t') << t << '\n';
}

inline void WriteNewline(std::wostream& out)
{
  out << L'\n';
}
