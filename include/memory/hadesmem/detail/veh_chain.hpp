// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <cstdint>
#include <list>

#include <hadesmem/config.hpp>
#include <hadesmem/detail/assert.hpp>
#include <hadesmem/error.hpp>
#include <hadesmem/process.hpp>
#include <hadesmem/detail/peb.hpp>
#include <hadesmem/detail/winternl.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/find_pattern.hpp>

// TODO. Add support for reading VEH chains from remote processes.

// TODO. Test on x64 although the pattern looked the same on first glance.

// TODO. Should I be returning the actual VEH chain as a linked list of
// winternl::VEH_NODE pointers or keep the std::list<> implementation?

// TODO. Ensure that the pattern works across all ntdll.dll versions available
// on >= Windows 7 systems (unless hadesmem supports XP and Vista too).

// TODO. Cleanup the code.

namespace hadesmem
{
namespace detail
{
inline
std::list<winternl::VEH_NODE> GetVehChain(hadesmem::Process const& process)
{
  if (process.GetId() != ::GetCurrentProcessId())
  {
	HADESMEM_DETAIL_THROW_EXCEPTION(
	  Error{} << ErrorString{"GetVehChain only supported on local process."});
  }

  std::list<winternl::VEH_NODE> chain;

  void* find_pattern =
	hadesmem::Find(process,
	  L"ntdll.dll",
	  L"81 C3 ?? ?? ?? ?? 53 89 46 0C",
	  hadesmem::PatternFlags::kThrowOnUnmatch,
	  0U);

  HADESMEM_DETAIL_TRACE_FORMAT_A(
	"ID = %d, Pattern = %p",
	process.GetId(),
	find_pattern);

  // .text:6B2A9D35  add  ebx, offset g_VEHChainHead
  // The add instruction is two bytes in size so we're
  // walking over it to get to the address of the head node.
  auto curr_code = static_cast<std::uint8_t*>(find_pattern);
  curr_code += 2;

  // .text:6B2A9D3F  lea     edi, [ebx+4]
  auto address =
	reinterpret_cast<std::uint8_t*>(*reinterpret_cast<std::uintptr_t*>(curr_code));
  address += 4;
  winternl::VEH_NODE* head_node = reinterpret_cast<winternl::VEH_NODE*>(address);
  winternl::VEH_NODE* node_iter = head_node;

  HADESMEM_DETAIL_TRACE_FORMAT_A(
	"Code pointer = %p, Head = %p",
	curr_code,
	head_node);

  do
  {
	chain.push_back(*node_iter);
	node_iter = node_iter->Next;
  } while (node_iter != head_node);

  return chain;
}
}
}