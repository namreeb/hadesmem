// Copyright Joshua Boyce 2010-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// This file is part of HadesMem.
// <http://www.raptorfactor.com/> <raptorfactor@raptorfactor.com>

#pragma once

#include <memory>
#include <vector>

#include <windows.h>

namespace hadesmem
{

class Process;
class Allocator;

class Patch
{
public:
  explicit Patch(Process const* process);

  Patch(Patch&& other);

  Patch& operator=(Patch&& other);

  virtual ~Patch();

  virtual void Apply();

  virtual void Remove();

  bool IsApplied() const;

protected:
  Process const* process_;
  bool applied_;

private:
  Patch(Patch const&);
  Patch& operator=(Patch const&);
};

class PatchRaw : public Patch
{
public:
  PatchRaw(Process const* process, PVOID target, 
    std::vector<BYTE> const& data);

  // TOOD: Templated constructor to convert T to std::vector<BYTE>.

  PatchRaw(PatchRaw&& other);

  PatchRaw& operator=(PatchRaw&& other);

  virtual ~PatchRaw();

  virtual void Apply();

  virtual void Remove();
   
private:
  PatchRaw(PatchRaw const&);
  PatchRaw& operator=(PatchRaw const&);

  PVOID target_;
  std::vector<BYTE> data_;
  std::vector<BYTE> orig_;
};

class PatchDetour : public Patch
{
public:
  PatchDetour(Process const* process, PVOID target, LPCVOID detour);

  PatchDetour(PatchDetour&& other);

  PatchDetour& operator=(PatchDetour&& other);

  virtual ~PatchDetour();

  virtual void Apply();

  virtual void Remove();

  LPCVOID GetTrampoline() const;

private:
  PatchDetour(PatchDetour const&);
  PatchDetour& operator=(PatchDetour const&);

  void WriteJump(PVOID address, LPCVOID target);

  unsigned int GetJumpSize() const;

  PVOID target_;
  LPCVOID detour_;
  std::unique_ptr<Allocator> trampoline_;
  std::vector<BYTE> orig_;
};

}
