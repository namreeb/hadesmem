/*
This file is part of HadesMem.
Copyright (C) 2010 Joshua Boyce (aka RaptorFactor, Cypherjb, Cypher, Chazwazza).
<http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

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
#pragma warning(disable: 4100)
#pragma warning(disable: 4127)
#pragma warning(disable: 4702)
#include <boost/thread.hpp>
#include <boost/format.hpp>
#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/iostreams/categories.hpp>
#pragma warning(pop)

// Windows API
#include <Windows.h>

// Hades
#include "Error.hpp"
#include "Filesystem.hpp"

#define HADES_LOG_THREAD_SAFE(x)\
{\
boost::lock_guard<boost::mutex> MyLock(Hades::Util::Logger<char>::GetMutex());\
x;\
}\

namespace Hades
{
  namespace Util
  {
    // Logging class
    template <typename CharT>
    class Logger
    {
    public:
      // Logger error type
      class Error : public virtual HadesError 
      { };

      // Get logger mutex
      static boost::mutex& GetMutex()
      {
        static boost::mutex MyMutex;
        return MyMutex;
      }

      // Sink information
      typedef CharT char_type;
      typedef boost::iostreams::sink_tag category;

      // STL typedefs
      typedef boost::filesystem::basic_ofstream<char_type> tofstream;

      // Stream typedef
      typedef boost::iostreams::stream<Logger<CharT>> Stream;

      // Constructor
      Logger(boost::filesystem::path const& LogDirPath, 
        std::wstring const& Filename) 
        : m_LogPath(GeneratePath(LogDirPath, Filename))
      { }

      // Get path to log file
      boost::filesystem::path GetLogPath() const 
      {
        return m_LogPath;
      }

      // Generate path to log file.
      boost::filesystem::path GeneratePath(
        boost::filesystem::path const& LogDirPath, 
        std::wstring const& Filename)
      {
        // Get local time
        auto const Time(boost::posix_time::second_clock::local_time());
        // Convert time to string YYYY-MM-DDTHH:MM:SS
        auto TimeStr(boost::posix_time::to_iso_extended_wstring(Time));
        // Reformat time YYYY-MM-DD_HH-MM-SS
        TimeStr[10] = L'_'; TimeStr[13] = L'-'; TimeStr[16] = L'-';

        // Generate file path relative to initial directory
        std::wstring const LogFile(Filename + L"-" + TimeStr + L".log");

        // Make full path to log file
        boost::filesystem::path const LogPath(LogDirPath / LogFile);

        // Return path to log file
        return LogPath;
      }

      // Writes n characters from s
      std::streamsize write(char_type const* s, std::streamsize n)
      {
        // Get time
        auto const Time(boost::posix_time::second_clock::local_time());
        // Convert time to string YYYY-MM-DDTHH:MM:SS
        auto TimeStr(boost::posix_time::to_iso_extended_string_type<char_type>(
          Time));
        // Reformat time YYYY-MM-DD_HH-MM-SS
        TimeStr[10] = '_'; TimeStr[13] = '-'; TimeStr[16] = '-';

        // Open file
        tofstream Out(m_LogPath, tofstream::out | tofstream::app);
        if(!Out)
        {
          BOOST_THROW_EXCEPTION(Error() << 
            ErrorFunction("Logger::write") << 
            ErrorString("Could not open file."));
        }

        // Write time as string
        Out << '[' << TimeStr << "]: ";

        // Write data
        Out.write(s, n);

        // Return size
        return n;
      }

    private:
      // Path to log file
      boost::filesystem::path m_LogPath;
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
      boost::filesystem::path const SelfDirPath(Windows::GetSelfDirPath());

      // Ensure Logs directory exists
      boost::filesystem::path const LogsPath(SelfDirPath / L"/Logs/");
      boost::filesystem::create_directory(LogsPath);

      // Redirect standard output streams to file
      if (!OutName.empty())
      {
        static Logger<char> AnsiLogger(LogsPath, OutName);
        static Logger<char>::Stream AnsiStream(AnsiLogger);
        static Logger<wchar_t> WideLogger(LogsPath, OutName);
        static Logger<wchar_t>::Stream WideStream(WideLogger);
        std::cout.rdbuf(AnsiStream.rdbuf());
        std::wcout.rdbuf(WideStream.rdbuf());

        std::wcout << "Logger initialized." << std::endl;
      }

      // Redirect standard log output streams to file
      if (!LogName.empty())
      {
        static Logger<char> AnsiLogger(LogsPath, LogName);
        static Logger<char>::Stream AnsiStream(AnsiLogger);
        static Logger<wchar_t> WideLogger(LogsPath, LogName);
        static Logger<wchar_t>::Stream WideStream(WideLogger);
        std::clog.rdbuf(AnsiStream.rdbuf());
        std::wclog.rdbuf(WideStream.rdbuf());

        std::wclog << "Logger initialized." << std::endl;
      }
    }
  }
}
