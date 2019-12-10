// Main program for suballoc driver
// Copyright (C) 1999 by Walter Oney
// All rights reserved

#include "stddcls.h"
#include "driver.h"
#include <initguid.h>
#include "suballoc.h"

NTSTATUS AddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT pdo);
VOID DriverUnload(IN PDRIVER_OBJECT fido);
NTSTATUS DispatchAny(IN PDEVICE_OBJECT fido, IN PIRP Irp);
NTSTATUS DispatchPower(IN PDEVICE_OBJECT fido, IN PIRP Irp);
NTSTATUS DispatchPnp(IN PDEVICE_OBJECT fido, IN PIRP Irp);
NTSTATUS DispatchWmi(IN PDEVICE_OBJECT fido, IN PIRP Irp);
NTSTATUS HandleStartDevice(PDEVICE_EXTENSION pdx, PIRP Irp);

BOOLEAN IsWin98();
BOOLEAN win98 = FALSE;

UNICODE_STRING servkey;

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath)
	{							// DriverEntry

	// Insist that OS support at least the WDM level of the DDK we use

	if (!IoIsWdmVersionAvailable(1, 0))
		{
		KdPrint((DRIVERNAME " - Expected version of WDM (%d.%2.2d) not available\n", 1, 0));
		return STATUS_UNSUCCESSFUL;
		}

	// See if we're running under Win98 or NT:

	win98 = IsWin98();


	// Save the name of the service key

	servkey.Buffer = (PWSTR) ExAllocatePool(PagedPool, RegistryPath->Length + sizeof(WCHAR));
	if (!servkey.Buffer)
		{
		KdPrint((DRIVERNAME " - Unable to allocate %d bytes for copy of service key name\n", RegistryPath->Length + sizeof(WCHAR)));
		return STATUS_INSUFFICIENT_RESOURCES;
		}
	servkey.MaximumLength = RegistryPath->Length + sizeof(WCHAR);
	RtlCopyUnicodeString(&servkey, RegistryPath);
	servkey.Buffer[RegistryPath->Length / 2] = 0;

	// Initialize function pointers

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->DriverExtension->AddDevice = AddDevice;

	for (int i = 0; i < arraysize(DriverObject->MajorFunction); ++i)
		DriverObject->MajorFunction[i] = DispatchAny;

	DriverObject->MajorFunction[IRP_MJ_POWER] = DispatchPower;
	DriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnp;
	DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = DispatchWmi;
	
	return STATUS_SUCCESS;
	}							// DriverEntry

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID DriverUnload(IN PDRIVER_OBJECT DriverObject)
	{							// DriverUnload
	PAGED_CODE();
	RtlFreeUnicodeString(&servkey);
	}							// DriverUnload

///////////////////////////////////////////////////////////////////////////////

NTSTATUS AddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT pdo)
	{							// AddDevice
	PAGED_CODE();

	NTSTATUS status;

	// Create a filter device object to represent the hardware we're managing.

	PDEVICE_OBJECT fido;
	#define xsize sizeof(DEVICE_EXTENSION)
	status = IoCreateDevice(DriverObject, xsize, NULL,
		FILE_DEVICE_UNKNOWN, 0, FALSE, &fido);
	if (!NT_SUCCESS(status))
		{						// can't create device object
		KdPrint((DRIVERNAME " - IoCreateDevice failed - %X\n", status));
		return status;
		}						// can't create device object
	
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fido->DeviceExtension;

	// From this point forward, any error will have side effects that need to
	// be cleaned up.

	do
		{						// finish initialization
		pdx->DeviceObject = fido;
		pdx->Pdo = pdo;
		IoInitializeRemoveLock(&pdx->RemoveLock, 0, 0, 0);

		// Add our device object to the stack and propagate critical settings
		// from the immediately lower device object

		PDEVICE_OBJECT ldo = IoAttachDeviceToDeviceStack(fido, pdo);
		if (!ldo)
			{						// can't attach device
			KdPrint((DRIVERNAME " - IoAttachDeviceToDeviceStack failed\n"));
			status = STATUS_DEVICE_REMOVED;
			break;;
			}						// can't attach device

		pdx->LowerDeviceObject = ldo;
		fido->Flags |= ldo->Flags & (DO_DIRECT_IO | DO_BUFFERED_IO | DO_POWER_PAGABLE);
		fido->DeviceType = ldo->DeviceType;
		fido->Characteristics = ldo->Characteristics;

		// Clear the "initializing" flag so that we can get IRPs

		fido->Flags &= ~DO_DEVICE_INITIALIZING;
		}						// finish initialization
	while (FALSE);

	if (!NT_SUCCESS(status))
		{					// need to cleanup
		if (pdx->LowerDeviceObject)
			IoDetachDevice(pdx->LowerDeviceObject);
		IoDeleteDevice(fido);
		}					// need to cleanup

	return status;
	}							// AddDevice

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

NTSTATUS CompleteRequest(IN PIRP Irp, IN NTSTATUS status, IN ULONG_PTR info)
	{							// CompleteRequest
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
	}							// CompleteRequest

NTSTATUS CompleteRequest(IN PIRP Irp, IN NTSTATUS status)
	{							// CompleteRequest
	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
	}							// CompleteRequest

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE				// make no assumptions about pageability of dispatch fcns

NTSTATUS DispatchAny(IN PDEVICE_OBJECT fido, IN PIRP Irp)
	{							// DispatchAny
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fido->DeviceExtension;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

	// Pass request down without additional processing

	NTSTATUS status;
	status = IoAcquireRemoveLock(&pdx->RemoveLock, Irp);
	if (!NT_SUCCESS(status))
		return CompleteRequest(Irp, status, 0);
	IoSkipCurrentIrpStackLocation(Irp);
	status = IoCallDriver(pdx->LowerDeviceObject, Irp);
	IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	return status;
	}							// DispatchAny

///////////////////////////////////////////////////////////////////////////////

NTSTATUS DispatchPower(IN PDEVICE_OBJECT fido, IN PIRP Irp)
	{							// DispatchPower

	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fido->DeviceExtension;
	PoStartNextPowerIrp(Irp);	// must be done while we own the IRP
	NTSTATUS status;
	status = IoAcquireRemoveLock(&pdx->RemoveLock, Irp);
	if (!NT_SUCCESS(status))
		return CompleteRequest(Irp, status, 0);
	IoSkipCurrentIrpStackLocation(Irp);
	status = PoCallDriver(pdx->LowerDeviceObject, Irp);
	IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	return status;
	}							// DispatchPower

///////////////////////////////////////////////////////////////////////////////

NTSTATUS DispatchPnp(IN PDEVICE_OBJECT fido, IN PIRP Irp)
	{							// DispatchPnp
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG fcn = stack->MinorFunction;

	NTSTATUS status;
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fido->DeviceExtension;
	status = IoAcquireRemoveLock(&pdx->RemoveLock, Irp);
	if (!NT_SUCCESS(status))
		return CompleteRequest(Irp, status, 0);

	// Handle START_DEVICE specially in order to suballocate resources from the
	// parent device
	
	if (fcn == IRP_MN_START_DEVICE)
		status = HandleStartDevice(pdx, Irp);

	// Pass other requests down the stack

	else
		{
		IoSkipCurrentIrpStackLocation(Irp);
		status = IoCallDriver(pdx->LowerDeviceObject, Irp);
		}

	if (fcn == IRP_MN_REMOVE_DEVICE)
		{
		IoReleaseRemoveLockAndWait(&pdx->RemoveLock, Irp);
		RemoveDevice(fido);
		}
	else
		IoReleaseRemoveLock(&pdx->RemoveLock, Irp);

	return status;
	}							// DispatchPnp

///////////////////////////////////////////////////////////////////////////////

NTSTATUS DispatchWmi(IN PDEVICE_OBJECT fido, IN PIRP Irp)
	{							// DispatchWmi

	NTSTATUS status;
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fido->DeviceExtension;
	status = IoAcquireRemoveLock(&pdx->RemoveLock, Irp);
	if (!NT_SUCCESS(status))
		return CompleteRequest(Irp, status, 0);
	IoSkipCurrentIrpStackLocation(Irp);
	status = IoCallDriver(pdx->LowerDeviceObject, Irp);
	IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	return status;
	}							// DispatchWmi

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID RemoveDevice(IN PDEVICE_OBJECT fido)
	{							// RemoveDevice
	PAGED_CODE();
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fido->DeviceExtension;
	NTSTATUS status;


	if (pdx->LowerDeviceObject)
		IoDetachDevice(pdx->LowerDeviceObject);

	IoDeleteDevice(fido);
	}							// RemoveDevice

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

BOOLEAN IsWin98()
	{							// IsWin98
#ifdef _X86_

	// Windows 98 (including 2d ed) supports WDM version 1.0, whereas Win2K
	// supports 1.10.

	return !IoIsWdmVersionAvailable(1, 0x10);
#else // not _X86_
	return FALSE;
#endif // not _X86_
	}							// IsWin98

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

NTSTATUS StartDeviceCompletionRoutine(PDEVICE_OBJECT fido, PIRP Irp, PKEVENT pev)
	{							// StartDeviceCompletionRoutine
	KeSetEvent(pev, EVENT_INCREMENT, FALSE);
	return STATUS_MORE_PROCESSING_REQUIRED;
	}							// StartDeviceCompletionRoutine

///////////////////////////////////////////////////////////////////////////////
// HandleStartDevice is the central point of this filter driver. The IRP_MN_START_DEVICE
// for a child device won't have any resources because, as far as the PnP Manager is
// concerned, the parent driver is the one that owns all the resources. We
// therefore use a direct-call interface (the RESOURCE_SUBALLOCATE_STANDARD) to
// get the right set of resources from the parent driver, and we put them into
// the START_DEVICE IRP before passing it down to the child function driver.

#pragma PAGEDCODE

NTSTATUS HandleStartDevice(PDEVICE_EXTENSION pdx, PIRP Irp)
	{							// HandleStartDevice
	KEVENT event;
	KeInitializeEvent(&event, NotificationEvent, FALSE);

	// Send a QUERY_INTERFACE request to the multifunction parent device driver to
	// locate the resource suballocation interface.

	RESOURCE_SUBALLOCATE_STANDARD suballoc;

	IoCopyCurrentIrpStackLocationToNext(Irp);
	PIO_STACK_LOCATION stack = IoGetNextIrpStackLocation(Irp);
	stack->MajorFunction = IRP_MJ_PNP;
	stack->MinorFunction = IRP_MN_QUERY_INTERFACE;
	stack->Parameters.QueryInterface.InterfaceType = &GUID_RESOURCE_SUBALLOCATE_STANDARD;
	stack->Parameters.QueryInterface.Size = sizeof(suballoc);
	stack->Parameters.QueryInterface.Version = RESOURCE_SUBALLOCATE_STANDARD_VERSION;
	stack->Parameters.QueryInterface.Interface = &suballoc;
	stack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

	IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE) StartDeviceCompletionRoutine, (PVOID) &event, TRUE, TRUE, TRUE);

	NTSTATUS status = IoCallDriver(pdx->LowerDeviceObject, Irp);
	if (status == STATUS_PENDING)
		{
		KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
		status = Irp->IoStatus.Status;
		}

	if (!NT_SUCCESS(status))
		return CompleteRequest(Irp, status);	// can't get interface pointer

	// Determine the I/O resources for our child device.

	PCM_RESOURCE_LIST raw, translated;
	status = suballoc.GetResources(suballoc.Context, pdx->Pdo, &raw, &translated);
	suballoc.InterfaceDereference(suballoc.Context);

	if (!NT_SUCCESS(status))
		return CompleteRequest(Irp, status);	// can't get suballocated resources

	// Pass the START_DEVICE down synchronously. Then release the resource lists
	// that the parent driver allocated for us

	KeClearEvent(&event);
	IoCopyCurrentIrpStackLocationToNext(Irp);
	stack = IoGetNextIrpStackLocation(Irp);
	stack->Parameters.StartDevice.AllocatedResources = raw;
	stack->Parameters.StartDevice.AllocatedResourcesTranslated = translated;

	IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE) StartDeviceCompletionRoutine, (PVOID) &event, TRUE, TRUE, TRUE);

	status = IoCallDriver(pdx->LowerDeviceObject, Irp);
	if (status == STATUS_PENDING)
		{
		KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
		status = Irp->IoStatus.Status;
		}

	ExFreePool(raw);
	ExFreePool(translated);

	return CompleteRequest(Irp, status);
	}							// HandleStartDevice

#pragma LOCKEDCODE				// force inline functions into locked code