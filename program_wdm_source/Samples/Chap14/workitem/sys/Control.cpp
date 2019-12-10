// Control.cpp -- IOCTL handlers for workitem driver
// Copyright (C) 1999 by Walter Oney
// All rights reserved

#include "stddcls.h"
#include "driver.h"
#include "ioctls.h"

struct _RANDOM_JUNK
	{
	PDEVICE_EXTENSION DeviceExtension;
	ULONG value;
	PIRP nfyirp;
	PIO_WORKITEM item;
	};
typedef _RANDOM_JUNK RANDOM_JUNK, *PRANDOM_JUNK;

VOID WorkItemCallback(PDEVICE_OBJECT fdo, PRANDOM_JUNK item);

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS DispatchControl(PDEVICE_OBJECT fdo, PIRP Irp)
	{							// DispatchControl
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	NTSTATUS status = IoAcquireRemoveLock(&pdx->RemoveLock, Irp);
	if (!NT_SUCCESS(status))
		return CompleteRequest(Irp, status, 0);
	ULONG info = 0;

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG cbin = stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG cbout = stack->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;

	switch (code)
		{						// process request

	case IOCTL_SUBMIT_ITEM:				// code == 0x800
		{						// IOCTL_SUBMIT_ITEM
		if (cbout < sizeof(ULONG))
			status = STATUS_INVALID_PARAMETER;
		else
			status = GenericCacheControlRequest(pdx->pgx, Irp, &pdx->NotifyIrp);

		if (status != STATUS_PENDING)
			break;				// something wrong, so quit

		// Create an executive work item that will complete the notification IRP.
		// Since there's no way to cancel a queued work item, leave the remove
		// lock claim we already have in place and use the new IoXxxWorkItem routines
		// to protect the device object until all of our code has finished executing.

		// Link this driver with ntoskrnl.lib instead of wdm.lib because (at RC-2, anyway),
		// the latter didn't define the IoXxxWorkItem routines. To test this driver under
		// Win98, you must also have WDMSTUB.VXD installed.

		PRANDOM_JUNK junk = (PRANDOM_JUNK) ExAllocatePool(PagedPool, sizeof(RANDOM_JUNK));
		if (!junk)
			{
			KdPrint((DRIVERNAME " - ExAllocatePool failed to allocate %d bytes for RANDOM_JUNK structure\n", sizeof(RANDOM_JUNK)));
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
			}

		junk->DeviceExtension = pdx;
		junk->value = *(PULONG) Irp->AssociatedIrp.SystemBuffer;
		junk->nfyirp = Irp;		// only used in call to IoReleaseRemoveLock

		PIO_WORKITEM item = IoAllocateWorkItem(fdo);
		if (!item)
			{
			ExFreePool(junk);
			KdPrint((DRIVERNAME " - IoAllocateWorkItem failed\n"));
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
			}

		junk->item = item;
		
		// Queue the work item for delayed execution. IoQueueWorkItem will take out an
		// extra reference to our device object that will persist until after the
		// callback routine returns, thereby preventing this driver from unloading before
		// the last instruction in this driver finishes executing.
		
		IoQueueWorkItem(item, (PIO_WORKITEM_ROUTINE) WorkItemCallback, DelayedWorkQueue, junk);
		KdPrint((DRIVERNAME " - Work item queued\n"));

		return STATUS_PENDING;
		}						// IOCTL_SUBMIT_ITEM

	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;

		}						// process request

	IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	return status == STATUS_PENDING ? status : CompleteRequest(Irp, status, info);
	}							// DispatchControl

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID WorkItemCallback(PDEVICE_OBJECT fdo, PRANDOM_JUNK junk)
	{							// WorkItemCallback
	PAGED_CODE();
	KdPrint((DRIVERNAME " - Work item callback has occurred\n"));
	PDEVICE_EXTENSION pdx = junk->DeviceExtension;
	ULONG value = junk->value;

	// Locate and complete the control IRP used for testing.

	PIRP nfyirp = GenericUncacheControlRequest(pdx->pgx, &pdx->NotifyIrp);

	if (nfyirp)
		{						// complete the notification IRP	
		*(PULONG) nfyirp->AssociatedIrp.SystemBuffer = junk->value;
		CompleteRequest(nfyirp, STATUS_SUCCESS, sizeof(ULONG));
		}						// complete the notification IRP

	// Release the remove lock to balance the acquisition done in DispatchControl.
	
	IoReleaseRemoveLock(&pdx->RemoveLock, junk->nfyirp);

	// Release the memory occupied by the work item

	IoFreeWorkItem(junk->item);

	// Finally, release our context structure

	ExFreePool(junk);
	}							// WorkItemCallback
