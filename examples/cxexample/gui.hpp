// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

namespace hadesmem
{
namespace cerberus
{
class PluginInterface;
}
}

void InitializeGui(hadesmem::cerberus::PluginInterface* cerberus);

void CleanupGui(hadesmem::cerberus::PluginInterface* cerberus);
