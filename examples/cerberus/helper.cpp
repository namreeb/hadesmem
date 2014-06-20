// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "helper.hpp"

#include <hadesmem/detail/trace.hpp>

namespace hadesmem
{

namespace cerberus
{

void UndetourFunc(std::wstring const& name,
                  std::unique_ptr<hadesmem::PatchDetour>& detour,
                  std::atomic<std::uint32_t>& ref_count,
                  bool remove)
{
  if (detour)
  {
    remove ? detour->Remove() : detour->Detach();
    HADESMEM_DETAIL_TRACE_FORMAT_W(L"%s undetoured.", name.c_str());
    detour = nullptr;

    while (ref_count.load())
    {
      HADESMEM_DETAIL_TRACE_FORMAT_W(L"Spinning on %s ref count.",
                                     name.c_str());
    }
    HADESMEM_DETAIL_TRACE_FORMAT_W(L"%s free of references.", name.c_str());
  }
  else
  {
    HADESMEM_DETAIL_TRACE_FORMAT_W(L"%s not detoured. Skipping.", name.c_str());
  }
}
}
}