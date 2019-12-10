// Main program for powtrace driver
// Copyright (C) 2002 by Walter Oney
// All rights reserved

// POWTRACE.SYS is intended as a lower filter that will show all the power-related irps flowing
// downward from a function driver. The resulting debug trace can be useful in diagnosing power
// management problems with drivers.

#include "stddcls.h"
#include "driver.h"

NTSTATUS AddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT pdo);
NTSTATUS CapabilitiesQueryCompletionRoutine(PDEVICE_OBJECT fido, PIRP Irp, PDEVICE_EXTENSION pdx);
VOID DriverUnload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS DispatchAny(IN PDEVICE_OBJECT fido, IN PIRP Irp);
NTSTATUS DispatchPower(IN PDEVICE_OBJECT fido, IN PIRP Irp);
NTSTATUS DispatchPnp(IN PDEVICE_OBJECT fido, IN PIRP Irp);
NTSTATUS DispatchWmi(IN PDEVICE_OBJECT fido, IN PIRP Irp);
ULONG GetDeviceTypeToUse(PDEVICE_OBJECT pdo);
NTSTATUS PowerCompletionRoutine(PDEVICE_OBJECT fido, PIRP Irp, PDEVICE_EXTENSION pdx);
VOID PowerTrace(PDEVICE_EXTENSION pdx, char* ctl, ...);
VOID ShowCapabilities(PDEVICE_EXTENSION pdx, PDEVICE_CAPABILITIES pdc);
NTSTATUS StartDeviceCompletionRoutine(PDEVICE_OBJECT fido, PIRP Irp, PDEVICE_EXTENSION pdx);
NTSTATUS UsageNotificationCompletionRoutine(PDEVICE_OBJECT fido, PIRP Irp, PDEVICE_EXTENSION pdx);

char* sysnames[] = {
	"PowerSystemUnspecified",
	"PowerSystemWorking",
	"PowerSystemSleeping1",
	"PowerSystemSleeping2",
	"PowerSystemSleeping3",
	"PowerSystemHibernate",
	"PowerSystemShutdown",
	};

char* devnames[] = {
	"PowerDeviceUnspecified",
	"PowerDeviceD0",
	"PowerDeviceD1",
	"PowerDeviceD2",
	"PowerDeviceD3",
	};

char* powminor[] = {
	"IRP_MN_WAIT_WAKE",
	"IRP_MN_POWER_SEQUENCE",
	"IRP_MN_SET_POWER",
	"IRP_MN_QUERY_POWER",
	};

FAST_MUTEX filelock;			// lock for file I/O

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath)
	{							// DriverEntry
	DbgPrint(DRIVERNAME " - Entering DriverEntry: DriverObject %8.8lX\n", DriverObject);

	// Initialize function pointers

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->DriverExtension->AddDevice = AddDevice;

	for (int i = 0; i < arraysize(DriverObject->MajorFunction); ++i)
		DriverObject->MajorFunction[i] = DispatchAny;

	DriverObject->MajorFunction[IRP_MJ_POWER] = DispatchPower;
	DriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnp;

	ExInitializeFastMutex(&filelock);
	
	return STATUS_SUCCESS;
	}							// DriverEntry

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID DriverUnload(IN PDRIVER_OBJECT DriverObject)
	{							// DriverUnload
	PAGED_CODE();
	DbgPrint(DRIVERNAME " - Entering DriverUnload: DriverObject %8.8lX\n", DriverObject);
	}							// DriverUnload

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS AddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT pdo)
	{							// AddDevice
	PAGED_CODE();
	DbgPrint(DRIVERNAME " - Entering AddDevice: DriverObject %8.8lX, pdo %8.8lX\n", DriverObject, pdo);

	NTSTATUS status;

	// Create a filter device object

	PDEVICE_OBJECT fdo;

	status = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION), NULL,
		GetDeviceTypeToUse(pdo), 0, FALSE, &fdo);
	if (!NT_SUCCESS(status))
		{						// can't create device object
		KdPrint((DRIVERNAME " - IoCreateDevice failed - %X\n", status));
		return status;
		}						// can't create device object
	
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	// From this point forward, any error will have side effects that need to
	// be cleaned up. Using a do-once block allows us to modify the program
	// easily without losing track of the side effects.

	do
		{						// finish initialization
		pdx->DeviceObject = fdo;
		pdx->Pdo = pdo;
		IoInitializeRemoveLock(&pdx->RemoveLock, 0, 0, 0);

		// Add our device object to the stack and propagate critical settings
		// from the immediately lower device object

		PDEVICE_OBJECT ldo = IoAttachDeviceToDeviceStack(fdo, pdo);
		if (!ldo)
			{						// can't attach device
			KdPrint((DRIVERNAME " - IoAttachDeviceToDeviceStack failed\n"));
			status = STATUS_DEVICE_REMOVED;
			break;
			}						// can't attach device

		pdx->LowerDeviceObject = ldo;

		// Copy the flags related to I/O buffering from the lower device object so the I/O manager
		// will create the expected data structures for reads and writes.

		fdo->Flags |= ldo->Flags & (DO_DIRECT_IO | DO_BUFFERED_IO | DO_POWER_PAGABLE);

		// Get registry flag FileLogging to decide whether to log to a disk file (\SystemRoot\powtrace.log)
		// or not.

		HANDLE hkey;
		status = IoOpenDeviceRegistryKey(pdo, PLUGPLAY_REGKEY_DEVICE, KEY_READ, &hkey);
		if (NT_SUCCESS(status))
			{					// check FileLogging flag
			UNICODE_STRING valname;
			RtlInitUnicodeString(&valname, L"FileLogging");

			ULONG junk;
			KEY_VALUE_PARTIAL_INFORMATION value;	// has room for a REG_DWORD value

			status = ZwQueryValueKey(hkey, &valname, KeyValuePartialInformation,
				&value, sizeof(value), &junk);

			if (NT_SUCCESS(status) && value.DataLength == sizeof(ULONG))
				pdx->filetrace = *(PULONG) value.Data != 0;

			ZwClose(hkey);
			}					// check FileLogging flag

		status = STATUS_SUCCESS;	// don't actually care if registry query worked or not

		PowerTrace(pdx, DRIVERNAME " - Initiating trace\n");

		// Clear the "initializing" flag so that we can get IRPs

		fdo->Flags &= ~DO_DEVICE_INITIALIZING;
		}						// finish initialization
	while (FALSE);

	if (!NT_SUCCESS(status))
		{					// need to cleanup
		if (pdx->LowerDeviceObject)
			IoDetachDevice(pdx->LowerDeviceObject);
		IoDeleteDevice(fdo);
		}					// need to cleanup

	return status;
	}							// AddDevice

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

NTSTATUS CapabilitiesQueryCompletionRoutine(PDEVICE_OBJECT fido, PIRP Irp, PDEVICE_EXTENSION pdx)
	{							// CapabilitiesQueryCompletionRoutine
	if (Irp->PendingReturned)
		IoMarkIrpPending(Irp);

	if (NT_SUCCESS(Irp->IoStatus.Status))
		ShowCapabilities(pdx, IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceCapabilities.Capabilities);

	IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	return STATUS_SUCCESS;
	}							// CapabilitiesQueryCompletionRoutine

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

NTSTATUS CompleteRequest(IN PIRP Irp, IN NTSTATUS status, IN ULONG_PTR info)
	{							// CompleteRequest
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
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

NTSTATUS DispatchPnp(IN PDEVICE_OBJECT fido, IN PIRP Irp)
	{							// DispatchPnp
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG fcn = stack->MinorFunction;

	NTSTATUS status;
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fido->DeviceExtension;
	status = IoAcquireRemoveLock(&pdx->RemoveLock, Irp);
	if (!NT_SUCCESS(status))
		return CompleteRequest(Irp, status, 0);

	// Handle usage notification specially in order to track power pageable
	// flag correctly

	if (fcn == IRP_MN_DEVICE_USAGE_NOTIFICATION)
		{						// usage notification
		if (!fido->AttachedDevice || (fido->AttachedDevice->Flags & DO_POWER_PAGABLE))
			fido->Flags |= DO_POWER_PAGABLE;
		IoCopyCurrentIrpStackLocationToNext(Irp);
		IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE) UsageNotificationCompletionRoutine,
			(PVOID) pdx, TRUE, TRUE, TRUE);
		return IoCallDriver(pdx->LowerDeviceObject, Irp);
		}						// usage notification

	// Handle start device specially in order to correctly inherit
	// FILE_REMOVABLE_MEDIA

	if (fcn == IRP_MN_START_DEVICE)
		{						// device start
		IoCopyCurrentIrpStackLocationToNext(Irp);
		IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE) StartDeviceCompletionRoutine,
			(PVOID) pdx, TRUE, TRUE, TRUE);
		return IoCallDriver(pdx->LowerDeviceObject, Irp);
		}						// device start

	// Handle remove device specially in order to cleanup device stack

	if (fcn == IRP_MN_REMOVE_DEVICE)
		{						// remove device
		IoSkipCurrentIrpStackLocation(Irp);
		status = IoCallDriver(pdx->LowerDeviceObject, Irp);
		IoReleaseRemoveLockAndWait(&pdx->RemoveLock, Irp);
		RemoveDevice(fido);
		return status;
		}						// remove device

	// Handle capabilities query specially in order to show the structure

	if (fcn == IRP_MN_QUERY_CAPABILITIES && !pdx->CapabilitiesDumped)
		{						// capabilities query
		pdx->CapabilitiesDumped = TRUE;	// only do this once
		IoCopyCurrentIrpStackLocationToNext(Irp);
		IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE) CapabilitiesQueryCompletionRoutine,
			(PVOID) pdx, TRUE, TRUE, TRUE);
		return IoCallDriver(pdx->LowerDeviceObject, Irp);
		}						// capabilities query

	// Simply forward any other type of PnP request

	IoSkipCurrentIrpStackLocation(Irp);
	status = IoCallDriver(pdx->LowerDeviceObject, Irp);
	IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	return status;
	}							// DispatchPnp

///////////////////////////////////////////////////////////////////////////////

NTSTATUS DispatchPower(IN PDEVICE_OBJECT fido, IN PIRP Irp)
	{							// DispatchPower
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fido->DeviceExtension;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG fcn = stack->MinorFunction;

	if (fcn == IRP_MN_SET_POWER || fcn == IRP_MN_QUERY_POWER)
		{
		POWER_STATE_TYPE type = stack->Parameters.Power.Type;

		if (type == SystemPowerState)
			PowerTrace(pdx, DRIVERNAME " - IRP_MJ_POWER (%s), SystemPowerState = %s\n", powminor[fcn], sysnames[stack->Parameters.Power.State.SystemState]);
		else
			PowerTrace(pdx, DRIVERNAME " - IRP_MJ_POWER (%s), DevicePowerState = %s\n", powminor[fcn], devnames[stack->Parameters.Power.State.DeviceState]);
		}
	else if (fcn == IRP_MN_WAIT_WAKE)
		{
		SYSTEM_POWER_STATE sysstate = stack->Parameters.WaitWake.PowerState;
		PowerTrace(pdx, DRIVERNAME " - IRP_MJ_POWER (IRP_MN_WAIT_WAKE), PowerState = %s\n", sysnames[sysstate]);
		}
	else if (fcn < arraysize(powminor))
		PowerTrace(pdx, DRIVERNAME " - IRP_MJ_POWER (%s)\n", powminor[fcn]);
	else
		PowerTrace(pdx, DRIVERNAME " - IRP_MJ_POWER (%X)\n", fcn);

	PoStartNextPowerIrp(Irp);	// must be done while we own the IRP
	NTSTATUS status;
	status = IoAcquireRemoveLock(&pdx->RemoveLock, Irp);
	if (!NT_SUCCESS(status))
		return CompleteRequest(Irp, status, 0);

	IoCopyCurrentIrpStackLocationToNext(Irp);
	IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE) PowerCompletionRoutine, (PVOID) pdx, TRUE, TRUE, TRUE);
	status = PoCallDriver(pdx->LowerDeviceObject, Irp);

	if (fcn < arraysize(powminor))
		PowerTrace(pdx, DRIVERNAME " - IRP_MJ_POWER (%s) dispatch routine returns %X\n", powminor[fcn], status);
	else
		PowerTrace(pdx, DRIVERNAME " - IRP_MJ_POWER (%X) dispatch routine returns %X\n", fcn, status);

	return status;
	}							// DispatchPower

///////////////////////////////////////////////////////////////////////////////
// GetDeviceTypeToUse returns the device object type of the next lower device
// object. This helps overcome a bug in some Win2K file systems which expect the
// topmost FiDO in a storage stack to have a VPB and, hence, to have been created
// with a type such as FILE_DEVICE_DISK.

#pragma PAGEDCODE

ULONG GetDeviceTypeToUse(PDEVICE_OBJECT pdo)
	{							// GetDeviceTypeToUse
	PDEVICE_OBJECT ldo = IoGetAttachedDeviceReference(pdo);
	if (!ldo)
		return FILE_DEVICE_UNKNOWN;
	ULONG devtype = ldo->DeviceType;
	ObDereferenceObject(ldo);
	return devtype;
	}							// GetDeviceTypeToUse

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

NTSTATUS PowerCompletionRoutine(PDEVICE_OBJECT fido, PIRP Irp, PDEVICE_EXTENSION pdx)
	{							// PowerCompletionRoutine

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG fcn = stack->MinorFunction;
	NTSTATUS status = Irp->IoStatus.Status;

	if (fcn < arraysize(powminor))
		PowerTrace(pdx, DRIVERNAME " - IRP_MJ_POWER (%s) completes with %X\n", powminor[fcn], status);
	else
		PowerTrace(pdx, DRIVERNAME " - IRP_MJ_POWER (%X) completes with %X\n", fcn, status);

	IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	return STATUS_SUCCESS;
	}							// PowerCompletionRoutine

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID PowerTraceMessage(char* msg)
	{							// PowerTraceMessage

	// Serialize access to the output file

	KeEnterCriticalRegion();
	ExAcquireFastMutexUnsafe(&filelock);

	// Open logging file. We want output flushed through to disk each time.

	UNICODE_STRING filename;
	RtlInitUnicodeString(&filename, L"\\SystemRoot\\powtrace.log");
	OBJECT_ATTRIBUTES oa;
	InitializeObjectAttributes(&oa, &filename, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
		NULL, NULL);
	HANDLE hfile;
	IO_STATUS_BLOCK iosb;
	NTSTATUS status = ZwCreateFile(&hfile, FILE_APPEND_DATA, &oa, &iosb, NULL, FILE_ATTRIBUTE_NORMAL,
		0, FILE_OPEN_IF, FILE_WRITE_THROUGH | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
	if (!NT_SUCCESS(status))
		{
		KdPrint((DRIVERNAME " - ZwCreateFile failed - %X\n", status));
		}

	// Write line to file, then cleanup and exit

	else
		{
		ZwWriteFile(hfile, NULL, NULL, NULL, &iosb, msg, strlen(msg), NULL, NULL);
		ZwClose(hfile);
		}

	ExReleaseFastMutexUnsafe(&filelock);
	KeLeaveCriticalRegion();
	}							// PowerTraceMessage

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID PowerTraceWorkitem(PDEVICE_OBJECT fdo, PPOWTRACE_CONTEXT ctx)
	{							// PowerTraceWorkitem
	PowerTraceMessage(ctx->msg);
	IoFreeWorkItem(ctx->item);
	ExFreePool(ctx);
	}							// PowerTraceWorkitem

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

VOID PowerTrace(PDEVICE_EXTENSION pdx, char* ctl, ...)
	{							// PowerTrace
	char msgbuf[256];
	_vsnprintf(msgbuf, arraysize(msgbuf), ctl, (va_list) (&ctl + 1));
	DbgPrint("%s", msgbuf);

	if (!pdx || !pdx->filetrace)
		return;

	if (KeGetCurrentIrql() < DISPATCH_LEVEL)
		PowerTraceMessage(msgbuf);
	else
		{
		PPOWTRACE_CONTEXT ctx = (PPOWTRACE_CONTEXT) ExAllocatePool(NonPagedPool, sizeof(POWTRACE_CONTEXT) + strlen(msgbuf));
		if (!ctx)
			return;
		strcpy(ctx->msg, msgbuf);
		ctx->item = IoAllocateWorkItem(pdx->DeviceObject);
		if (!ctx->item)
			{
			ExFreePool(ctx);
			return;
			}
		IoQueueWorkItem(ctx->item,	(PIO_WORKITEM_ROUTINE) PowerTraceWorkitem, CriticalWorkQueue, ctx);
		}
	}							// PowerTrace

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID RemoveDevice(IN PDEVICE_OBJECT fdo)
	{							// RemoveDevice
	PAGED_CODE();
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	NTSTATUS status;

	if (pdx->LowerDeviceObject)
		IoDetachDevice(pdx->LowerDeviceObject);

	IoDeleteDevice(fdo);
	}							// RemoveDevice

///////////////////////////////////////////////////////////////////////////////	

#pragma LOCKEDCODE

VOID ShowCapabilities(PDEVICE_EXTENSION pdx, PDEVICE_CAPABILITIES pdc)
	{							// ShowCapabilities
	PowerTrace(pdx, DRIVERNAME " - Results of IRP_MN_QUERY_CAPABILITIES\n");
	PowerTrace(pdx, DRIVERNAME " - Size = %X\n", pdc->Size);
	PowerTrace(pdx, DRIVERNAME " - Version = %X\n", pdc->Version);
	PowerTrace(pdx, DRIVERNAME " - DeviceD1 = %X\n", pdc->DeviceD1);
	PowerTrace(pdx, DRIVERNAME " - DeviceD2 = %X\n", pdc->DeviceD2);
	PowerTrace(pdx, DRIVERNAME " - LockSupported = %X\n", pdc->LockSupported);
	PowerTrace(pdx, DRIVERNAME " - EjectSupported = %X\n", pdc->EjectSupported);
	PowerTrace(pdx, DRIVERNAME " - Removable = %X\n", pdc->Removable);
	PowerTrace(pdx, DRIVERNAME " - DockDevice = %X\n", pdc->DockDevice);
	PowerTrace(pdx, DRIVERNAME " - UniqueID = %X\n", pdc->UniqueID);
	PowerTrace(pdx, DRIVERNAME " - SilentInstall = %X\n", pdc->SilentInstall);
	PowerTrace(pdx, DRIVERNAME " - RawDeviceOK = %X\n", pdc->RawDeviceOK);
	PowerTrace(pdx, DRIVERNAME " - SurpriseRemovalOK = %X\n", pdc->SurpriseRemovalOK);
	PowerTrace(pdx, DRIVERNAME " - WakeFromD0 = %X\n", pdc->WakeFromD0);
	PowerTrace(pdx, DRIVERNAME " - WakeFromD1 = %X\n", pdc->WakeFromD1);
	PowerTrace(pdx, DRIVERNAME " - WakeFromD2 = %X\n", pdc->WakeFromD2);
	PowerTrace(pdx, DRIVERNAME " - WakeFromD3 = %X\n", pdc->WakeFromD3);
	PowerTrace(pdx, DRIVERNAME " - HardwareDisabled = %X\n", pdc->HardwareDisabled);
	PowerTrace(pdx, DRIVERNAME " - NonDynamic = %X\n", pdc->NonDynamic);
	PowerTrace(pdx, DRIVERNAME " - WarmEjectSupported = %X\n", pdc->WarmEjectSupported);
	PowerTrace(pdx, DRIVERNAME " - NoDisplayInUI = %X\n", pdc->NoDisplayInUI);

	PowerTrace(pdx, DRIVERNAME " - Address = %X\n", pdc->Address);
	PowerTrace(pdx, DRIVERNAME " - UINumber = %X\n", pdc->UINumber);

	for (int i = 0; i < (int) POWER_SYSTEM_MAXIMUM; ++i)
		PowerTrace(pdx, DRIVERNAME " - DeviceState[%s] = %s\n", sysnames[i], devnames[pdc->DeviceState[i]]);

	PowerTrace(pdx, DRIVERNAME " - SystemWake = %s\n", sysnames[pdc->SystemWake]);
	PowerTrace(pdx, DRIVERNAME " - DeviceWake = %s\n", devnames[pdc->DeviceWake]);
	PowerTrace(pdx, DRIVERNAME " - D1Latency = %X\n", pdc->D1Latency);
	PowerTrace(pdx, DRIVERNAME " - D2Latency = %X\n", pdc->D2Latency);
	PowerTrace(pdx, DRIVERNAME " - D3Latency = %X\n", pdc->D3Latency);
	}							// ShowCapabilities

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

NTSTATUS StartDeviceCompletionRoutine(PDEVICE_OBJECT fido, PIRP Irp, PDEVICE_EXTENSION pdx)
	{							// StartDeviceCompletionRoutine
	if (Irp->PendingReturned)
		IoMarkIrpPending(Irp);

	// Inherit FILE_REMOVABLE_MEDIA flag from lower object. This is necessary
	// for a disk filter, but it isn't available until start-device time. Drivers
	// above us may examine the flag as part of their own start-device processing, too.

	if (pdx->LowerDeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
		fido->Characteristics |= FILE_REMOVABLE_MEDIA;

	IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	return STATUS_SUCCESS;
	}							// StartDeviceCompletionRoutine

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

NTSTATUS UsageNotificationCompletionRoutine(PDEVICE_OBJECT fido, PIRP Irp, PDEVICE_EXTENSION pdx)
	{							// UsageNotificationCompletionRoutine
	if (Irp->PendingReturned)
		IoMarkIrpPending(Irp);

	// If lower driver cleared pageable flag, we must do the same

	if (!(pdx->LowerDeviceObject->Flags & DO_POWER_PAGABLE))
		fido->Flags &= ~DO_POWER_PAGABLE;

	IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	return STATUS_SUCCESS;
	}							// UsageNotificationCompletionRoutine

#pragma LOCKEDCODE				// force inline functions into nonpaged code
