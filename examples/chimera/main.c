// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

// TODO: Move this to its own solution when we start working on this again, and
// clean up our hadesmem solution (split into multiple(?), use props files,
// generate with cmake(?), etc). It's okay to use CMake for everything else and
// still have only a VS project for this, we can't make this compiler-portable
// anyway.

// TODO: Fix build configuration to get rid of the OS-specific configurations.
// http://insider.osr.com/2015/ntinsider_2015_02.pdf

// TODO: Actually start implementing this! e.g. Custom OpenProcess,
// ReadProcessMemory, etc. APIs to work around ACs like EAC.
// http://bit.ly/1RHDW6Y

// TODO: Remove LLKHF_INJECTED from injected input. http://bit.ly/1VTFDUw

#include "main.h"

#include <ntddk.h>
#include <ntstrsafe.h>

PCHIMERA_DATA ChimeraData = NULL;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#endif

NTSTATUS
DriverEntry(__in PDRIVER_OBJECT pDriverObject,
            __in PUNICODE_STRING pusRegistryPath)
{
  NTSTATUS Status = STATUS_SUCCESS;
  ULONG i;
  UNICODE_STRING DeviceName = {0};
  UNICODE_STRING DosDeviceName = {0};
  PDEVICE_OBJECT pDeviceObject = NULL;

  // TODO: Get name dynamically instead of hardcoding.
  UNREFERENCED_PARAMETER(pusRegistryPath);

  FUNCTION_ENTRY;

  PAGED_CODE();

  ChimeraData =
    ExAllocatePoolWithTag(NonPagedPool, sizeof(CHIMERA_DATA), CHIMERA_POOL_TAG);
  if (NULL == ChimeraData)
  {
    KdPrint(("[Chimera] [%s] [0x%p]: ExAllocatePoolWithTag failed.\n",
             __FUNCTION__,
             KeGetCurrentThread()));
    Status = STATUS_INSUFFICIENT_RESOURCES;
    goto Exit;
  }

  RtlZeroMemory(ChimeraData, sizeof(*ChimeraData));

  RtlUnicodeStringInit(&DeviceName, CHIMERA_DEVICE_NAME_U);

  Status = IoCreateDevice(pDriverObject,
                          0,
                          &DeviceName,
                          FILE_DEVICE_UNKNOWN,
                          0,
                          FALSE,
                          &pDeviceObject);
  if (!NT_SUCCESS(Status))
  {
    KdPrint(("[Chimera] [%s] [0x%p]: IoCreateDevice failed. Status: [%ld].\n",
             __FUNCTION__,
             KeGetCurrentThread(),
             Status));
    goto Cleanup_1;
  }

  for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
  {
    pDriverObject->MajorFunction[i] = ChimeraDispatchUnsupported;
  }

  pDriverObject->MajorFunction[IRP_MJ_CREATE] = ChimeraDispatchCreateClose;
  pDriverObject->MajorFunction[IRP_MJ_CLEANUP] = ChimeraDispatchCreateClose;
  pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =
    ChimeraDispatchDeviceControl;
  pDriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = ChimeraDispatchShutdown;
  pDriverObject->DriverUnload = DriverUnload;

  Status = IoRegisterShutdownNotification(pDeviceObject);
  if (!NT_SUCCESS(Status))
  {
    KdPrint(("[Chimera] [%s] [0x%p]: IoRegisterShutdownNotification failed. "
             "Status: [%ld].\n",
             __FUNCTION__,
             KeGetCurrentThread(),
             Status));
    goto Cleanup_2;
  }

  RtlUnicodeStringInit(&DosDeviceName, CHIMERA_DOS_DEVICE_NAME_U);
  Status = IoCreateSymbolicLink(&DosDeviceName, &DeviceName);
  if (!NT_SUCCESS(Status))
  {
    KdPrint(
      ("[Chimera] [%s] [0x%p]: IoCreateSymbolicLink failed. Status: [%ld].\n",
       __FUNCTION__,
       KeGetCurrentThread(),
       Status));
    goto Cleanup_3;
  }

  goto Exit;

Cleanup_3:
  IoUnregisterShutdownNotification(pDeviceObject);

Cleanup_2:
  IoDeleteDevice(pDeviceObject);

Cleanup_1:
  ExFreePoolWithTag(ChimeraData, CHIMERA_POOL_TAG);

Exit:
  return Status;
}

NTSTATUS
ChimeraDispatchCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
  NTSTATUS Status = STATUS_SUCCESS;
  PIO_STACK_LOCATION IrpStack;

  UNREFERENCED_PARAMETER(DeviceObject);

  FUNCTION_ENTRY;

  IrpStack = IoGetCurrentIrpStackLocation(Irp);

  switch (IrpStack->MajorFunction)
  {
  case IRP_MJ_CREATE:
    Status = STATUS_SUCCESS;
    break;

  case IRP_MJ_CLEANUP:
    Status = STATUS_SUCCESS;
    break;

  default:
    Status = STATUS_INVALID_DEVICE_REQUEST;
    break;
  }

  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = 0;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return Status;
}

NTSTATUS
ChimeraDispatchDeviceControl(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
  NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;

  UNREFERENCED_PARAMETER(pDeviceObject);

  FUNCTION_ENTRY;

  pIrp->IoStatus.Status = Status;
  pIrp->IoStatus.Information = 0;

  IoCompleteRequest(pIrp, IO_NO_INCREMENT);

  return Status;
}

NTSTATUS
ChimeraDispatchShutdown(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
  NTSTATUS Status = STATUS_SUCCESS;

  UNREFERENCED_PARAMETER(pDeviceObject);

  FUNCTION_ENTRY;

  pIrp->IoStatus.Status = Status;
  pIrp->IoStatus.Information = 0;

  IoCompleteRequest(pIrp, IO_NO_INCREMENT);

  return Status;
}

NTSTATUS
ChimeraDispatchUnsupported(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIrp)
{
  NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;

  UNREFERENCED_PARAMETER(pDeviceObject);

  FUNCTION_ENTRY;

  pIrp->IoStatus.Status = Status;
  pIrp->IoStatus.Information = 0;

  IoCompleteRequest(pIrp, IO_NO_INCREMENT);

  return Status;
}

void DriverUnload(__in PDRIVER_OBJECT pDriverObject)
{
  PDEVICE_OBJECT pDeviceObject = pDriverObject->DeviceObject;
  UNICODE_STRING DosDeviceName = {0};

  FUNCTION_ENTRY;

  IoUnregisterShutdownNotification(pDeviceObject);

  ExFreePoolWithTag(ChimeraData, CHIMERA_POOL_TAG);

  RtlUnicodeStringInit(&DosDeviceName, CHIMERA_DOS_DEVICE_NAME_U);
  IoDeleteSymbolicLink(&DosDeviceName);

  IoDeleteDevice(pDeviceObject);

  return;
}
