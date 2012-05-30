#pragma once

#include <string>

#include <windows.h>

#include "hadesmemory/error.hpp"

namespace hadesmem
{

class Process
{
public:
  explicit Process(DWORD id);
  
  Process(Process&& other);
  
  Process& operator=(Process&& other);
  
  ~Process();
  
  DWORD GetId() const;
  
  HANDLE GetHandle() const;
  
private:
  Process(Process const&);
  Process& operator=(Process const&);
  
  void CheckWoW64() const;
  
  HANDLE Open(DWORD id);
  
  void Cleanup();
  
  DWORD id_;
  HANDLE handle_;
};

bool operator==(Process const& lhs, Process const& rhs);

bool operator!=(Process const& lhs, Process const& rhs);

std::string GetPath(Process const& process);

bool IsWoW64(Process const& process);

}
