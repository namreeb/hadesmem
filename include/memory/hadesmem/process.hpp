#pragma once

#include <string>

#include <windows.h>

#include "hadesmem/error.hpp"
#include "hadesmem/config.hpp"

namespace hadesmem
{

class Process
{
public:
  explicit Process(DWORD id);
  
  Process(Process&& other) BOOST_NOEXCEPT;
  
  Process& operator=(Process&& other) BOOST_NOEXCEPT;
  
  ~Process() BOOST_NOEXCEPT;
  
  DWORD GetId() const BOOST_NOEXCEPT;
  
  HANDLE GetHandle() const BOOST_NOEXCEPT;
  
private:
  Process(Process const&);
  Process& operator=(Process const&);
  
  void CheckWoW64() const;
  
  HANDLE Open(DWORD id);
  
  void Cleanup();
  
  DWORD id_;
  HANDLE handle_;
};

bool operator==(Process const& lhs, Process const& rhs) BOOST_NOEXCEPT;

bool operator!=(Process const& lhs, Process const& rhs) BOOST_NOEXCEPT;

std::string GetPath(Process const& process);

bool IsWoW64(Process const& process);

}
