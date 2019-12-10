// Main program for hidfake driver
// Copyright (C) 2001 by Walter Oney
// All rights reserved

#include "stddcls.h"
#include "driver.h"

NTSTATUS AddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT fdo);
VOID DriverUnload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS PowerUpCompletionRoutine(PDEVICE_OBJECT fdo, PIRP Irp, PDEVICE_EXTENSION pdx);

struct INIT_STRUCT : public _GENERIC_INIT_STRUCT {
//	QSIO morequeues[1];			// additional devqueue/sio pointers
	};

BOOLEAN win98 = FALSE;
BOOLEAN winXP = FALSE;

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath)
	{							// DriverEntry
	KdPrint((DRIVERNAME " - Entering DriverEntry: DriverObject %8.8lX\n", DriverObject));

	if (GenericGetVersion() < 0x00010003)
		{
		KdPrint((DRIVERNAME " - Required version (1.3) of GENERIC.SYS not installed\n"));
		return STATUS_UNSUCCESSFUL;
		}

	// See if we're running under Win98 or NT:

	win98 = IsWin98();

#if DBG
	if (win98)
		KdPrint((DRIVERNAME " - Running under Windows 98\n"));
	else
		KdPrint((DRIVERNAME " - Running under NT\n"));
#endif

	// We need to handle IRP_MN_QUERY_ID specially in systems prior to XP, so determine
	// now whether we're in XP or not

	winXP = IoIsWdmVersionAvailable(1, 0x20);

	// In other of my sample drivers, I copy the RegistryPath key here. There's no point in
	// a HID minidriver because HIDCLASS will be handling any WMI requests that occur (and none
	// will, because there are no Microsoft standard classes for HID devices).

	// Initialize function pointers

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->DriverExtension->AddDevice = AddDevice;

	DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = DispatchInternalControl;
	DriverObject->MajorFunction[IRP_MJ_POWER] = DispatchPower;
	DriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnp;

	// This is a HID minidriver, so register with HIDCLASS. To make it easier
	// for the test application to give us dummy report data, we claim to be
	// polled. Most real HID devices are not polled, actually.

	HID_MINIDRIVER_REGISTRATION reg;
	RtlZeroMemory(&reg, sizeof(reg));
	reg.Revision = HID_REVISION;
	reg.DriverObject = DriverObject;
	reg.RegistryPath = RegistryPath;
	reg.DeviceExtensionSize = sizeof(DEVICE_EXTENSION) + GetSizeofGenericExtension();
	reg.DevicesArePolled = TRUE;
	
	return HidRegisterMinidriver(&reg);
	}							// DriverEntry

///////////////////////////////////////////////////////////////////////////////
// The DriverUnload callback for a HID minidriver releases resources allocated
// in DriverEntry. This driver doesn't need any cleanup, however, so this is
// a nearly empty function.

#pragma PAGEDCODE

VOID DriverUnload(IN PDRIVER_OBJECT DriverObject)
	{							// DriverUnload
	PAGED_CODE();
	KdPrint((DRIVERNAME " - Entering DriverUnload: DriverObject %8.8lX\n", DriverObject));
	}							// DriverUnload

///////////////////////////////////////////////////////////////////////////////
// AddDevice callback. Note that we do *not* create a device object in this
// callback. HIDCLASS has already done that. Our job is simply to initialize
// our device extension.

#pragma PAGEDCODE

NTSTATUS AddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT fdo)
	{							// AddDevice
	PAGED_CODE();
	KdPrint((DRIVERNAME " - Entering AddDevice: DriverObject %8.8lX, fdo %8.8lX\n", DriverObject, fdo));

	NTSTATUS status = STATUS_SUCCESS;
	
	PDEVICE_EXTENSION pdx = PDX(fdo);
	BOOLEAN ginit = FALSE;

	// From this point forward, any error will have side effects that need to
	// be cleaned up. Using a do-once block allows us to modify the program
	// easily without losing track of the side effects.

	do
		{						// finish initialization

		// Initialize to use the GENERIC.SYS library

		pdx->pgx = (PGENERIC_EXTENSION) ((PUCHAR) pdx + sizeof(DEVICE_EXTENSION));

		INIT_STRUCT gis;
		RtlZeroMemory(&gis, sizeof(gis));
		gis.Size = sizeof(gis);
		gis.DeviceObject = fdo;
		gis.Pdo = PDO(fdo);
		gis.Ldo = LDO(fdo);
		gis.RemoveLock = &pdx->RemoveLock;
		gis.StartDevice = StartDevice;
		gis.StopDevice = StopDevice;
		gis.RemoveDevice = RemoveDevice;
		RtlInitUnicodeString(&gis.DebugName, LDRIVERNAME);

		status = InitializeGenericExtension(pdx->pgx, &gis);
		if (!NT_SUCCESS(status))
			{
			KdPrint((DRIVERNAME " - InitializeGenericExtension failed - %X\n", status));
			break;
			}
		ginit = TRUE;

		// Indicate that the device is initially in the D0 power state

		pdx->devpower = PowerDeviceD0;

		}						// finish initialization
	while (FALSE);

	if (!NT_SUCCESS(status))
		{					// need to cleanup
		if (ginit)
			CleanupGenericExtension(pdx->pgx);
		}					// need to cleanup

	// HIDCLASS ignores our return code, so save the status code in the
	// device extension for use by StartDevice

	pdx->AddDeviceStatus = status;
	return STATUS_SUCCESS;
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

NTSTATUS CompleteRequest(PIRP Irp, NTSTATUS status)
	{							// CompleteRequest
	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
	}							// CompleteRequest

NTSTATUS CompleteRequest(PIRP Irp)
	{							// CompleteRequest
	NTSTATUS status = Irp->IoStatus.Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
	}							// CompleteRequest

///////////////////////////////////////////////////////////////////////////////
// The DispatchPnp callback for a HID minidriver does pretty much what any
// function driver would do. We delegate the IRPs to GENERIC, which calls
// back to StartDevice, StopDevice, and RemoveDevice functions.

#pragma PAGEDCODE

NTSTATUS DispatchPnp(PDEVICE_OBJECT fdo, PIRP Irp)
	{							// DispatchPnp
	PDEVICE_EXTENSION pdx = PDX(fdo);	
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

	// Handle IRP_MN_QUERY_ID separately prior to XP because the root enumerator
	// will succeed the request and give back a NULL id

	if (!winXP 
		&& stack->MinorFunction == IRP_MN_QUERY_ID 
		&& !NT_SUCCESS(Irp->IoStatus.Status))
		{						// handle QUERY_ID
		ASSERT(!Irp->IoStatus.Information);	// should be NULL if no-one has handled this IRP
		PWCHAR idstring;

#if DBG
		static char* names[] = {
			"BusQueryDeviceID",
			"BusQueryHardwareIDs",
			"BusQueryCompatibleIDs",
			"BusQueryInstanceID",
			"BusQueryDeviceSerialNumber",
			};

		ULONG type = stack->Parameters.QueryId.IdType;
		if (type < arraysize(names))
			KdPrint((DRIVERNAME " - IRP_MN_QUERY_ID(%s)\n", names[type]));
		else
			KdPrint((DRIVERNAME " - IRP_MN_QUERY_ID(%d)\n", type));
#endif

		switch (stack->Parameters.QueryId.IdType)
			{					// select on query type

		case BusQueryInstanceID:
			idstring = L"0000";
			break;

		case BusQueryDeviceID:
			idstring = L"ROOT\\*WCO0D01";
			break;

		case BusQueryHardwareIDs:
			idstring = L"*WCO0D01";
			break;

		default:
			return CompleteRequest(Irp);
			}					// select on query type

		ULONG nchars = wcslen(idstring);
		ULONG size = (nchars + 2) * sizeof(WCHAR);	// room for 2 null terminators
		PWCHAR id = (PWCHAR) ExAllocatePool(PagedPool, size);
		if (!id)
			return CompleteRequest(Irp, STATUS_INSUFFICIENT_RESOURCES);
		wcscpy(id, idstring);
		id[nchars + 1] = 0;		// in case someone is expecting MULTI_SZ
		return CompleteRequest(Irp, STATUS_SUCCESS, (ULONG_PTR) id);
		}						// handle QUERY_ID

	return GenericDispatchPnp(PDX(fdo)->pgx, Irp);
	}							// DispatchPnp

///////////////////////////////////////////////////////////////////////////////
// The DispatchPower callback for a HID minidriver need only worry about saving and
// restoring device context. The framework provided by HIDCLASS might force you
// to block the system thread while doing this, which is not ideal but won't cause
// deadlocks as long as you don't issue any power IRPs.

#pragma PAGEDCODE

NTSTATUS DispatchPower(PDEVICE_OBJECT fdo, PIRP Irp)
	{							// DispatchPower
	PDEVICE_EXTENSION pdx = PDX(fdo);	
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

	IoCopyCurrentIrpStackLocationToNext(Irp);

	// Check for D-set operation. Note that we needn't call PoStartNextPowerIrp
	// because HIDCLASS has already done so.

	if (stack->MinorFunction == IRP_MN_SET_POWER && stack->Parameters.Power.Type == DevicePowerState)
		{						// device set-power
		DEVICE_POWER_STATE newstate = stack->Parameters.Power.State.DeviceState;
		KdPrint((DRIVERNAME " - Setting power state D%d\n", newstate - 1));
		if (newstate == PowerDeviceD0)
			{					// restoring power
			IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE) PowerUpCompletionRoutine, (PVOID) pdx,
				TRUE, TRUE, TRUE);
			}					// restoring power
		else if (pdx->devpower == PowerDeviceD0)
			{					// powering down
			// TODO save context information, if any
			pdx->devpower = newstate;
			}					// powering down
		}						// device set-power

	// Forward all other IRPs (and D-sets that lower power below D0)

	return PoCallDriver(LDO(fdo), Irp);
	}							// DispatchPower

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

NTSTATUS PowerUpCompletionRoutine(PDEVICE_OBJECT fdo, PIRP Irp, PDEVICE_EXTENSION pdx)
	{							// PowerUpCompletionRoutine
	// TODO restore device context without blocking this thread
	pdx->devpower = PowerDeviceD0;
	return STATUS_SUCCESS;		// allow IRP to continue completing
	}							// PowerUpCompletionRoutine

///////////////////////////////////////////////////////////////////////////////
// The RemoveDevice function for a HID minidriver merely cleans up resources
// allocated in the AddDevice callback. It does not delete the device object
// (HIDCLASS does that).

#pragma PAGEDCODE

VOID RemoveDevice(IN PDEVICE_OBJECT fdo)
	{							// RemoveDevice
	PAGED_CODE();
	}							// RemoveDevice

#pragma LOCKEDCODE				// force inline functions into nonpaged code
