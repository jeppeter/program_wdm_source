// Control.cpp -- IOCTL handlers for wmiextra driver
// Copyright (C) 1999 by Walter Oney
// All rights reserved

#include "stddcls.h"
#include "driver.h"
#include "ioctls.h"
#include "wmiextra.h"

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS DispatchControl(PDEVICE_OBJECT fdo, PIRP Irp)
	{							// DispatchControl
	PAGED_CODE();
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	NTSTATUS status = STATUS_SUCCESS;
	ULONG info = 0;

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG cbin = stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG cbout = stack->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;

	switch (code)
		{						// process request

	case IOCTL_FIRE_EVENT:				// code == 0x800
		{						// IOCTL_FIRE_EVENT
		if (!pdx->evenable)
			{					// event not enabled
			status = STATUS_INVALID_DEVICE_REQUEST;
			break;
			}					// event not enabled

		// Construct an event object in nonpaged memory. The system will free this
		// when it's no longer needed

		Pwmiextra_event junk = (Pwmiextra_event) ExAllocatePool(NonPagedPool, wmiextra_event_SIZE);
		if (!junk)
			{
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
			}

		junk->EventInfo = 42;
		status = WmiFireEvent(fdo, (LPGUID) &wmiextra_event_GUID, 0, wmiextra_event_SIZE, junk);
		if (!NT_SUCCESS(status))
			KdPrint((DRIVERNAME " - WmiFireEvent failed - %X\n", status));
		break;
		}						// IOCTL_FIRE_EVENT

	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;

		}						// process request

	return CompleteRequest(Irp, status, info);
	}							// DispatchControl
