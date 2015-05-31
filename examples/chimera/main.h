// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

#pragma once

#include <ntddk.h>

#define CHIMERA_DEVICE_NAME_U L"\\Device\\Chimera"
#define CHIMERA_DOS_DEVICE_NAME_U L"\\DosDevices\\Chimera"
#define CHIMERA_POOL_TAG 'mihC'

#define FUNCTION_ENTRY KdPrint(("Chimera.sys: %s", __FUNCTION__))

typedef struct _CHIMERA_DATA
{
  ULONG Data;
} CHIMERA_DATA, *PCHIMERA_DATA;

extern PCHIMERA_DATA ChimeraData;

DRIVER_INITIALIZE DriverEntry;

__drv_dispatchType(IRP_MJ_CREATE)
  __drv_dispatchType(IRP_MJ_CLEANUP) DRIVER_DISPATCH ChimeraDispatchCreateClose;

__drv_dispatchType(IRP_MJ_DEVICE_CONTROL)
  DRIVER_DISPATCH ChimeraDispatchDeviceControl;

__drv_dispatchType(IRP_MJ_SHUTDOWN) DRIVER_DISPATCH ChimeraDispatchShutdown;

__drv_dispatchType_other DRIVER_DISPATCH ChimeraDispatchUnsupported;

DRIVER_UNLOAD DriverUnload;
