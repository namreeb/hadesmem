/*
This file is part of HadesMem.
Copyright © 2010 Cypherjb (aka Chazwazza, aka Cypher). 
<http://www.cypherjb.com/> <cypher.jb@gmail.com>

HadesMem is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

HadesMem is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with HadesMem.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

// C++ Standard Library
#include <string>
#include <vector>
#include <utility>
#include <fstream>
#include <iostream>

// Boost C++ Libraries
#pragma warning(push, 1)
#pragma warning(disable: 4702)
#include <boost/format.hpp>
#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/categories.hpp>
#pragma warning(pop)

// Windows API
#include <Windows.h>

// Hades
#include "Error.h"
#include "Filesystem.h"

namespace Hades
{
  namespace Util
  {
    // Logger exception type
    class LoggerError : public virtual HadesError
    { };

    // Logging class
    template <typename CharT>
    class Logger
    {
    public:
      // Sink information
      typedef CharT char_type;
      typedef boost::iostreams::sink_tag category;

      // STL typedefs
      typedef std::basic_ofstream<char_type> tofstream;

      // Stream typedef
      typedef boost::iostreams::stream<Logger<CharT>> Stream;

      // Constructor
      Logger(boost::filesystem::wpath const& LogDirPath, 
        std::wstring const& Filename) 
        : m_LogPath(GeneratePath(LogDirPath, Filename))
      { }

      // Get path to log file
      std::wstring GetLogPath() const 
      {
        return m_LogPath;
      }

      // Generate path to log file.
      std::wstring GeneratePath(boost::filesystem::wpath const& LogDirPath, 
        std::wstring const& Filename)
      {
        // Get local time
        auto const Time(boost::posix_time::second_clock::local_time());
        // Convert time to string YYYY-MM-DDTHH:MM:SS
        auto TimeStr(boost::posix_time::to_iso_extended_wstring(Time));
        // Reformat time YYYY-MM-DD_HH-MM-SS
        TimeStr[10] = '_'; TimeStr[13] = '-'; TimeStr[16] = '-';

        // Generate file path relative to initial directory
        auto const LogFile((boost::wformat(L"%s-%s.log") %Filename %TimeStr).
          str());

        // Make full path to log file
        auto const LogPath(LogDirPath / LogFile);

        // Return path to log file
        return LogPath.file_string();
      }

      // Writes n characters from s
      std::streamsize write(const char_type* s, std::streamsize n)
      {
        // Get time
        auto const Time(boost::posix_time::second_clock::local_time());
        // Convert time to string YYYY-MM-DDTHH:MM:SS
        auto TimeStr(boost::posix_time::to_iso_extended_string_type<char_type>(
          Time));
        // Reformat time YYYY-MM-DD_HH-MM-SS
        TimeStr[10] = '_'; TimeStr[13] = '-'; TimeStr[16] = '-';

        // Open file
        tofstream Out(m_LogPath.c_str(), tofstream::out | tofstream::app);

        // Check if file access succeeded
        if(Out)
        {
          // Write time as string
          Out << '[' << TimeStr << "]: ";

          // Write data
          Out.write(s, n);
        }

        // Return size
        return n;
      }

    private:
      // Path to log file
      std::wstring m_LogPath;
    };

    // Initialize logging. Returns wide path to self.
    inline void InitLogger(std::wstring const& OutName, 
      std::wstring const& LogName)
    {
      // Check if we actually need to continue
      if (OutName.empty() && LogName.empty())
      {
        return;
      }

      // Path to self dir
      auto const SelfDirPath(Windows::GetSelfDirPath());

      // Ensure Logs directory exists
      auto const LogsPath(SelfDirPath / L"/Logs/");
      boost::filesystem::create_directory(LogsPath);

      // Redirect standard output streams to file
      if (!OutName.empty())
      {
        static Logger<char>::Stream AnsiStream;
        static Logger<wchar_t>::Stream WideStream;
        AnsiStream.open(Logger<char>(LogsPath, OutName));
        std::cout.rdbuf(AnsiStream.rdbuf());
        WideStream.open(Logger<wchar_t>(LogsPath, OutName));
        std::wcout.rdbuf(WideStream.rdbuf());

        std::wcout << "Logger initialized." << std::endl;
      }

      // Redirect standard log output streams to file
      if (!LogName.empty())
      {
        static Logger<char>::Stream AnsiStream;
        static Logger<wchar_t>::Stream WideStream;
        AnsiStream.open(Logger<char>(LogsPath, LogName));
        std::clog.rdbuf(AnsiStream.rdbuf());
        WideStream.open(Logger<wchar_t>(LogsPath, LogName));
        std::wclog.rdbuf(WideStream.rdbuf());

        std::wclog << "Logger initialized." << std::endl;
      }
    }
  }
}
