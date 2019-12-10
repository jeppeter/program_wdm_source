// Main program for wdmstub driver
// Copyright (C) 2000 by Walter Oney
// All rights reserved

// Use this driver as a lower filter for any WDM driver that uses one or more
// of the non-WDM functions this driver supports.

#include "stddcls.h"
#include "driver.h"
#include "version.h"

NTSTATUS AddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT pdo);
VOID DriverUnload(IN PDRIVER_OBJECT fdo);
NTSTATUS DispatchAny(IN PDEVICE_OBJECT fido, IN PIRP Irp);
NTSTATUS DispatchInternalControl(IN PDEVICE_OBJECT fido, IN PIRP Irp);
NTSTATUS DispatchPower(IN PDEVICE_OBJECT fido, IN PIRP Irp);
NTSTATUS DispatchPnp(IN PDEVICE_OBJECT fido, IN PIRP Irp);

BOOLEAN IsWin98();
BOOLEAN StubsDefined = FALSE;	// true if we defined any stubs
PDEVICE_OBJECT DummyDeviceObject = NULL;	// dummy device object
BOOLEAN CheckCloneVersions(PDRIVER_OBJECT DriverObject);

///////////////////////////////////////////////////////////////////////////////
// Declare dummy symbols to bound the pointers emitted by the compiler for
// static initialization functions.

typedef void (__cdecl *INITFUNCTION)();

#pragma data_seg(".CRT$XCA")
static INITFUNCTION BeginInitFunctions[1] = {0};

#pragma data_seg(".CRT$XCZ")
static INITFUNCTION EndInitFunctions[1] = {0};

#pragma data_seg()

///////////////////////////////////////////////////////////////////////////////

#pragma INITCODE

extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath)
	{							// DriverEntry

	// See if we're running under Win98 or NT:

	if (!IsWin98())
		{
		KdPrint((DRIVERNAME " - Driver should only be used in Windows 98"));
		return STATUS_UNSUCCESSFUL;
		}

	// Execute static initialization routines. This became necessary with SP-9
	// when we started using the Xp DDK compiler

	for (INITFUNCTION* p = BeginInitFunctions + 1; p < EndInitFunctions; ++p)
		(*p)();

	// Initialize function pointers

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->DriverExtension->AddDevice = AddDevice;

	for (int i = 0; i < arraysize(DriverObject->MajorFunction); ++i)
		DriverObject->MajorFunction[i] = DispatchAny;

	DriverObject->MajorFunction[IRP_MJ_POWER] = DispatchPower;
	DriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnp;
	DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = DispatchInternalControl;

	// See if there are other instances of WDMSTUB running on this computer. These
	// might include the original WDMSTUB.VXD from the book or an OEM clone of this
	// driver.

	StubsDefined = CheckCloneVersions(DriverObject);

	// Create export definitions for the WDM functions this driver supplies

	if (StubsDefined && !DefineStubs())
		{						// error defining stubs
		ObDereferenceObject(DriverObject);
		IoDeleteDevice(DummyDeviceObject);
		return STATUS_UNSUCCESSFUL;
		}						// error defining stubs

	return STATUS_SUCCESS;
	}							// DriverEntry

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS AddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT pdo)
	{							// AddDevice
	PAGED_CODE();

	NTSTATUS status;

	// Create a function device object to represent the hardware we're managing.

	PDEVICE_OBJECT fdo;
	#define xsize sizeof(DEVICE_EXTENSION)
	status = IoCreateDevice(DriverObject, xsize, NULL,
		FILE_DEVICE_UNKNOWN, 0, FALSE, &fdo);
	if (!NT_SUCCESS(status))
		{						// can't create device object
		KdPrint((DRIVERNAME " - IoCreateDevice failed - %X\n", status));
		return status;
		}						// can't create device object
	
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	// From this point forward, any error will have side effects that need to
	// be cleaned up.

	do
		{						// finish initialization
		pdx->DeviceObject = fdo;
		pdx->Pdo = pdo;

		// Add our device object to the stack and propagate critical settings
		// from the immediately lower device object

		PDEVICE_OBJECT ldo = IoAttachDeviceToDeviceStack(fdo, pdo);
		if (!ldo)
			{						// can't attach device
			KdPrint((DRIVERNAME " - IoAttachDeviceToDeviceStack failed\n"));
			status = STATUS_DEVICE_REMOVED;
			break;;
			}						// can't attach device

		pdx->LowerDeviceObject = ldo;
		fdo->DeviceType = ldo->DeviceType;
		fdo->Characteristics = ldo->Characteristics;

		// Copy the flags related to I/O buffering from the lower device object so the I/O manager
		// will create the expected data structures for reads and writes.

		fdo->Flags |= ldo->Flags & (DO_DIRECT_IO | DO_BUFFERED_IO);

		// Indicate that we have a pagable power handler. The reason we do this is actually fairly
		// complex, as follows. It's okay to have a pagable power handler layered above a non-paged
		// or in-rush power handler, but not the other way around. If we're an upper filter for any
		// kind of device, it's safer to leave this flag set always. If we were to clear it (for
		// example, because we notice that the guy under us is non-paged or becomes so after
		// processing a usage notification), we would have to always set it when passing down
		// usage notifications later on or risk violating the rule.

		// IF WE ARE A LOWER FILTER FOR A DEVICE THAT HAS A NON-PAGABLE POWER HANDLER, OR FOR A
		// DEVICE THAT CHANGES BETWEEN PAGED AND NON-PAGED DEPENDING ON USAGE NOTIFICATIONS, THIS
		// SIMPLISTIC CODE IS WRONG. YOU MUST FIND OUT HOW THE UPPER DRIVER WILL HANDLE THE
		// DO_POWER_PAGABLE FLAG AND DO THE RIGHT THING.

		fdo->Flags |= DO_POWER_PAGABLE;

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
// CheckCloneVersions looks for the VxD version of WDMSTUB or an OEM clone of
// this same filter driver and evaluates the version number(s) of any such.
// It returns TRUE if version checking indicates that we should stub or implement
// at least one function.

#pragma PAGEDCODE

BOOLEAN CheckCloneVersions(PDRIVER_OBJECT DriverObject)
	{							// CheckCloneVersions
	BOOLEAN result = TRUE;

	// See if there is a VxD version of WDMSTUB loaded and, if so, what it's version
	// number is.

	PVMMDDB ddb = Get_DDB(0, "WDMSTUB ");
	if (ddb)
		{						// VxD version loaded
		if (ddb->DDB_Dev_Major_Version > VERMAJOR || (ddb->DDB_Dev_Major_Version == VERMAJOR && ddb->DDB_Dev_Minor_Version > VERMINOR))
			{					// defer to VxD
			KdPrint((DRIVERNAME " - Deferring to WDMSTUB.VXD version %d.%2.2d\n", ddb->DDB_Dev_Major_Version, ddb->DDB_Dev_Minor_Version));
			result = FALSE;
			}					// defer to VxD
		else
			KdPrint((DRIVERNAME " - Overriding WDMSTUB.VXD version %d.%2.2d\n", ddb->DDB_Dev_Major_Version, ddb->DDB_Dev_Minor_Version));
		}						// VxD version loaded

	// See if there is a clone of this filter driver with a higher version number

	ULONG index;				// current driver index
	ULONG avail = 0xFFFFFFFF;	// an available driver index
	WCHAR namebuf[32];
	UNICODE_STRING devname;

	KEVENT event;
	KeInitializeEvent(&event, SynchronizationEvent, FALSE);
	IO_STATUS_BLOCK iostatus;
	NTSTATUS status;

	// Look for dummy device objects WDMSTUB0 through WDMSTUB99

	for (index = 0; index < 100 && result; ++index)
		{						// for each possible driver index
		_snwprintf(namebuf, arraysize(namebuf), L"\\Device\\WDMSTUB%d", index);
		RtlInitUnicodeString(&devname, namebuf);

		PFILE_OBJECT fop;
		PDEVICE_OBJECT clone;
		status = IoGetDeviceObjectPointer(&devname, FILE_READ_DATA, &fop, &clone);

		// If there is a device object with this name, send it a get-version IOCTL

		if (NT_SUCCESS(status))
			{					// found a clone
			ULONG version;
			PIRP Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_WDMSTUB_GET_VERSION, clone, NULL, 0, &version, sizeof(version),
				TRUE, &event, &iostatus);
			if (!Irp)
				{
				KdPrint((DRIVERNAME " - Unable to allocate IRP for version query\n"));
				result = FALSE;
				}
			else
				{				// send IRP
				status = IoCallDriver(clone, Irp);
				if (status == STATUS_PENDING)
					{			// wait for IRP
					KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
					status = iostatus.Status;
					}			// wait for IRP
				if (NT_SUCCESS(status))
					{			// check version #
					if (version >= ((VERMAJOR << 16) | VERMINOR))
						{		// found higher version
						KdPrint((DRIVERNAME " - Deferring to WDMSTUB clone version %d.%2.2d\n", version >> 16, version & 0xFFFF));
						result = FALSE;
						}		// found higher version
					else
						KdPrint((DRIVERNAME " - Overriding WDMSTUB clone version %d.%2.2d\n", version >> 16, version & 0xFFFF));
					}			// check version #
				else
					KdPrint((DRIVERNAME " - IOCTL_INTERNAL_WDMSTUB_GET_VERSION failed to %ws - %X\n", devname, status));
				}				// send IRP

			// Dereference the file object to release our hold on the clone device object

			ObDereferenceObject(fop);
			}					// found a clone

		// If there's no device object by this name, remember the index for when we need to create our own
		// device object

		else if (avail == 0xFFFFFFFF)
			avail = index;
		}						// for each possible driver index

	// Check the result of the version comparisons

	if (!result)
		return FALSE;

	// We have decided to stub some functions. Create a dummy device object so
	// other clones can find us. Also take an extra reference to the driver object
	// to pin us into memory -- some other WDM driver may end up using one of our
	// stubs

	if (avail == 0xFFFFFFFF)
		{
		KdPrint((DRIVERNAME " - More than 100 WDMSTUB clones are loaded in this system!\n"));
		return FALSE;
		}

	_snwprintf(namebuf, arraysize(namebuf), L"\\Device\\WDMSTUB%d", avail);
	RtlInitUnicodeString(&devname, namebuf);

	status = IoCreateDevice(DriverObject, sizeof(DUMMY_DEVICE_EXTENSION), &devname, FILE_DEVICE_UNKNOWN, 0, FALSE, &DummyDeviceObject);
	if (!NT_SUCCESS(status))
		{
		KdPrint((DRIVERNAME " - IoCreateDevice failed - %X\n", status));
		return FALSE;
		}

	ObReferenceObject(DriverObject);	// pin this driver into memory forever

	// Initialize the dummy device extension so IRP dispatch functions can distinguish between this
	// and a layered device object

	PDUMMY_DEVICE_EXTENSION ddx = (PDUMMY_DEVICE_EXTENSION) DummyDeviceObject->DeviceExtension;
	ddx->flags |= DUMMY_DEVICE_FLAG;

	return TRUE;
	}							// CheckCloneVersions

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
	
	// Fail any IRPs addressed to the dummy device object other than CREATE and CLOSE

	if (pdx->flags & DUMMY_DEVICE_FLAG)
		{						// IRP for dummy device
		PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
		NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
		if (stack->MajorFunction == IRP_MJ_CREATE || stack->MajorFunction == IRP_MJ_CLOSE)
			status = STATUS_SUCCESS;
		return CompleteRequest(Irp, status);
		}						// IRP for dummy device

	// Pass down IRPs addressed to the FiDO

	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(pdx->LowerDeviceObject, Irp);
	}							// DispatchAny

///////////////////////////////////////////////////////////////////////////////

NTSTATUS DispatchInternalControl(PDEVICE_OBJECT dummy, PIRP Irp)
	{							// DispatchInternalControl
	PDUMMY_DEVICE_EXTENSION ddx = (PDUMMY_DEVICE_EXTENSION) dummy->DeviceExtension;
	if (!(ddx->flags & DUMMY_DEVICE_FLAG))
		{						// not for us
		IoSkipCurrentIrpStackLocation(Irp);
		return IoCallDriver(((PDEVICE_EXTENSION)(ddx))->LowerDeviceObject, Irp);
		}						// not for us

	NTSTATUS status;
	ULONG info = 0;
	
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;
	ULONG cbin = stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG cbout = stack->Parameters.DeviceIoControl.OutputBufferLength;

	switch (code)
		{						// process internal control operation

	// Version query: return the version number of this driver

	case IOCTL_INTERNAL_WDMSTUB_GET_VERSION:
		if (cbout < sizeof(ULONG))
			{					// wrong output buffer length
			status = STATUS_INVALID_PARAMETER;
			break;
			}					// wrong output buffer length
		*(PULONG)(Irp->UserBuffer) = (VERMAJOR << 16) | VERMINOR;
		info = sizeof(ULONG);
		status = STATUS_SUCCESS;
		break;

	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
		}						// process internal control operation

	return CompleteRequest(Irp, status, info);
	}							// DispatchInternalControl

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

NTSTATUS DispatchPower(IN PDEVICE_OBJECT fido, IN PIRP Irp)
	{							// DispatchPower
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fido->DeviceExtension;
	PoStartNextPowerIrp(Irp);	// must be done while we own the IRP

	if (pdx->flags & DUMMY_DEVICE_FLAG)
		return CompleteRequest(Irp, STATUS_SUCCESS);	// shouldn't happen in the 1st place

	IoSkipCurrentIrpStackLocation(Irp);
	return PoCallDriver(pdx->LowerDeviceObject, Irp);
	}							// DispatchPower

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

NTSTATUS DispatchPnp(IN PDEVICE_OBJECT fido, IN PIRP Irp)
	{							// DispatchPnp
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fido->DeviceExtension;

	// The dummy device shouldn't get any PnP IRPs

	if (pdx->flags & DUMMY_DEVICE_FLAG)
		return CompleteRequest(Irp, STATUS_SUCCESS);	// don't change Information field

	// Pass down PnP request 

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG fcn = stack->MinorFunction;

	NTSTATUS status;

	IoSkipCurrentIrpStackLocation(Irp);
	status = IoCallDriver(pdx->LowerDeviceObject, Irp);

	if (fcn == IRP_MN_REMOVE_DEVICE)
		RemoveDevice(fido);

	return status;
	}							// DispatchPnp

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID DriverUnload(IN PDRIVER_OBJECT DriverObject)
	{							// DriverUnload
	PAGED_CODE();

	// Delete our dummy device object

	if (DummyDeviceObject)
		IoDeleteDevice(DummyDeviceObject);

	// It should not be possible for this driver to be unloaded if we defined
	// any stubs. Don't preclude this, though, because there may be some future
	// mechanism that would make this safe.

	if (StubsDefined)
		UndefineStubs();
	}							// DriverUnload

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID RemoveDevice(IN PDEVICE_OBJECT fdo)
	{							// RemoveDevice
	PAGED_CODE();
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	ASSERT(!(pdx->flags & DUMMY_DEVICE_FLAG));	// shouldn't be possible
	NTSTATUS status;

	if (pdx->LowerDeviceObject)
		IoDetachDevice(pdx->LowerDeviceObject);

	IoDeleteDevice(fdo);
	}							// RemoveDevice

///////////////////////////////////////////////////////////////////////////////

#pragma INITCODE

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

#pragma LOCKEDCODE				// force inline functions into locked code