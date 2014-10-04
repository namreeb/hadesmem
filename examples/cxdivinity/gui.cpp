// Copyright (C) 2010-2014 Joshua Boyce.
// See the file COPYING for copying permission.

#include "gui.hpp"

#include <hadesmem/config.hpp>
#include <hadesmem/detail/trace.hpp>
#include <hadesmem/error.hpp>

#include "dump.hpp"

namespace
{

std::uint32_t g_on_ant_tweak_bar_initialize_callback_id =
  static_cast<std::uint32_t>(-1);

std::uint32_t g_on_ant_tweak_bar_cleanup_callback_id =
  static_cast<std::uint32_t>(-1);

TwBar* g_tweak_bar = nullptr;

void TW_CALL DumpFullInfoCallbackTw(void* /*client_data*/)
{
  HADESMEM_DETAIL_TRACE_A("Called.");

  DumpFullInfoCallback();
}

void TW_CALL DumpPartyInfoCallbackTw(void* /*client_data*/)
{
  HADESMEM_DETAIL_TRACE_A("Called.");

  DumpPartyInfoCallback();
}

void OnAntTweakBarInitialize(
  hadesmem::cerberus::AntTweakBarInterface* ant_tweak_bar)
{
  HADESMEM_DETAIL_TRACE_A("Initializing AntTweakBar.");

  if (g_tweak_bar)
  {
    HADESMEM_DETAIL_TRACE_A(
      "WARNING! AntTweakBar is already initialized. Skipping.");
    return;
  }

  if (!ant_tweak_bar->IsInitialized())
  {
    HADESMEM_DETAIL_TRACE_A(
      "WARNING! AntTweakBar is not initialized by Cerberus. Skipping.");
    return;
  }

  g_tweak_bar = ant_tweak_bar->TwNewBar("CXDivinity");
  if (!g_tweak_bar)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TwNewBar failed."}
                        << hadesmem::ErrorStringOther{TwGetLastError()});
  }

  auto const dump_full_button =
    ant_tweak_bar->TwAddButton(g_tweak_bar,
                               "CXDivinity_DumpFullBtn",
                               &DumpFullInfoCallbackTw,
                               nullptr,
                               " label='Dump Full Info' ");
  if (!dump_full_button)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TwAddButton failed."}
                        << hadesmem::ErrorStringOther{TwGetLastError()});
  }

  auto const dump_party_button =
    ant_tweak_bar->TwAddButton(g_tweak_bar,
                               "CXDivinity_DumpPartyBtn",
                               &DumpPartyInfoCallbackTw,
                               nullptr,
                               " label='Dump Party Info' ");
  if (!dump_party_button)
  {
    HADESMEM_DETAIL_THROW_EXCEPTION(
      hadesmem::Error{} << hadesmem::ErrorString{"TwAddButton failed."}
                        << hadesmem::ErrorStringOther{TwGetLastError()});
  }
}

void
  OnAntTweakBarCleanup(hadesmem::cerberus::AntTweakBarInterface* ant_tweak_bar)
{
  HADESMEM_DETAIL_TRACE_A("Cleaning up AntTweakBar.");

  if (!ant_tweak_bar->IsInitialized())
  {
    HADESMEM_DETAIL_TRACE_A(
      "WARNING! AntTweakBar is not initialized by Cerberus. Skipping.");
    return;
  }

  if (g_tweak_bar != nullptr)
  {
    ant_tweak_bar->TwDeleteBar(g_tweak_bar);
    g_tweak_bar = nullptr;
  }
}
}

void InitializeGui(hadesmem::cerberus::PluginInterface* cerberus)
{
  g_on_ant_tweak_bar_initialize_callback_id =
    cerberus->GetRenderInterface()->RegisterOnAntTweakBarInitialize(
      &OnAntTweakBarInitialize);
  g_on_ant_tweak_bar_cleanup_callback_id =
    cerberus->GetRenderInterface()->RegisterOnAntTweakBarCleanup(
      &OnAntTweakBarCleanup);

  OnAntTweakBarInitialize(
    cerberus->GetRenderInterface()->GetAntTweakBarInterface());
}

void CleanupGui(hadesmem::cerberus::PluginInterface* cerberus)
{
  if (g_on_ant_tweak_bar_initialize_callback_id !=
      static_cast<std::uint32_t>(-1))
  {
    cerberus->GetRenderInterface()->UnregisterOnAntTweakBarInitialize(
      g_on_ant_tweak_bar_initialize_callback_id);
  }

  if (g_on_ant_tweak_bar_cleanup_callback_id != static_cast<std::uint32_t>(-1))
  {
    cerberus->GetRenderInterface()->UnregisterOnAntTweakBarCleanup(
      g_on_ant_tweak_bar_cleanup_callback_id);
  }

  OnAntTweakBarCleanup(
    cerberus->GetRenderInterface()->GetAntTweakBarInterface());
}
