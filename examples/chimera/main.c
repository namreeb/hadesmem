// Copyright (C) 2010-2015 Joshua Boyce
// See the file COPYING for copying permission.

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
    KdPrint(
      ("[Chimera] [%s] [0x%p]: ExAllocatePoolWithTag failed. Status: [%ld].\n",
       __FUNCTION__,
       KeGetCurrentThread(),
       Status));
    Status = STATUS_INSUFFICIENT_RESOURCES;
    goto exit;
  }

  RtlZeroMemory(ChimeraData, sizeof(ChimeraData));

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
    goto cleanup_1;
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
    goto cleanup_2;
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
    goto cleanup_3;
  }

  goto exit;

cleanup_3:
  IoUnregisterShutdownNotification(pDeviceObject);

cleanup_2:
  IoDeleteDevice(pDeviceObject);

cleanup_1:
  ExFreePoolWithTag(ChimeraData, CHIMERA_POOL_TAG);

exit:
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

  FUNCTION_ENTRY;

  IoUnregisterShutdownNotification(pDeviceObject);

  ExFreePoolWithTag(ChimeraData, CHIMERA_POOL_TAG);

  return;
}
