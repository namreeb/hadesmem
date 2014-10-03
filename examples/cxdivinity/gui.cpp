// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "gui.hpp"

#include <hadesmem/config.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/error.hpp>

namespace
{

void TW_CALL DumpInfoCallbackTw(void* /*client_data*/)
{
  HADESMEM_DETAIL_TRACE_A("Called.");
}

void OnAntTweakBarInitialize(
  hadesmem::cerberus::AntTweakBarInterface* ant_tweak_bar)
{
  HADESMEM_DETAIL_TRACE_A("Initializing AntTweakBar.");

  auto const bar = ant_tweak_bar->TwNewBar("CXDivinity");
  if (!bar)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TwNewBar failed."}
                        << hadesmem::ErrorStringOther{TwGetLastError()});
  }

  auto const dump_button = ant_tweak_bar->TwAddButton(bar,
                                                      "CXDivinity_DumpBtn",
                                                      &DumpInfoCallbackTw,
                                                      nullptr,
                                                      " label='Dump Info' ");
  if (!dump_button)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TwAddButton failed."}
                        << hadesmem::ErrorStringOther{TwGetLastError()});
  }
}

void OnAntTweakBarCleanup(
  hadesmem::cerberus::AntTweakBarInterface* /*ant_tweak_bar*/)
{
  HADESMEM_DETAIL_TRACE_A("Cleaning up AntTweakBar.");
}
}

void InitializeGui(hadesmem::cerberus::PluginInterface* cerberus)
{
  cerberus->GetRenderInterface()->RegisterOnAntTweakBarInitialize(
    &OnAntTweakBarInitialize);
  cerberus->GetRenderInterface()->RegisterOnAntTweakBarCleanup(
    &OnAntTweakBarCleanup);
}
