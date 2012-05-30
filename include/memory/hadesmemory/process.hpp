#pragma once

#include <string>

#include <windows.h>

#include "hadesmemory/error.hpp"

namespace hadesmem
{

class Process
{
public:
  class Error : public virtual HadesMemError
  { };
  
  explicit Process(DWORD proc_id);
  
  Process(Process&& other);
  
  Process& operator=(Process&& other);
  
  ~Process();
  
  DWORD GetId() const;
  
  HANDLE GetHandle() const;
  
  std::string GetPath() const;
  
  bool IsWoW64() const;
  
private:
  Process(Process const&);
  Process& operator=(Process const&);
  
  void SetWoW64();
  
  void Cleanup();
  
  DWORD id_;
  HANDLE handle_;
  bool is_wow64_;
};

bool operator==(Process const& lhs, Process const& rhs);

bool operator!=(Process const& lhs, Process const& rhs);

}
