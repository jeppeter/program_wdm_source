// Control.cpp -- IOCTL handlers for evwait driver
// Copyright (C) 1999 by Walter Oney
// All rights reserved

#include "stddcls.h"
#include "driver.h"
#include "ioctls.h"

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE				// locked because uses a spin lock

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

	///////////////////////////////////////////////////////////////////////////
	// IOCTL_REGISTER_EVENT registers or deregisters an event object that the
	// driver shares with a user-mode application.

	case IOCTL_REGISTER_EVENT:				// code == 0x800
		{						// IOCTL_REGISTER_EVENT
		if (cbin < sizeof(HANDLE))
			{
			status = STATUS_INVALID_PARAMETER;
			break;
			}

		// If a new event is being registered, get a pointer to the underlying
		// KEVENT object. This must be done at PASSIVE_LEVEL. This function call is
		// really the main thing this sample illustrates, by the way. Note that the
		// only access we require to the event is EVENT_MODIFY_STATE, so that's all
		// we insist be present for this handle.

		HANDLE hevent = *(PHANDLE) Irp->AssociatedIrp.SystemBuffer;
		PKEVENT pevent;
		if (hevent)
			status = ObReferenceObjectByHandle(hevent, EVENT_MODIFY_STATE, *ExEventObjectType,
				Irp->RequestorMode, (PVOID*) &pevent, NULL);
		else
			pevent = NULL;

		if (!NT_SUCCESS(status))
			break;				// punt -- there's a problem with the handle

		// This driver only allows one handle to be open at a time, so we don't
		// need to worry about where to store the event pointers registered by
		// multiple applications. If you wanted to allow multiple opens and to
		// allow each handle owner to have its own event, you could use
		// stack->FileObject->FsContext or FsContext2 for that purpose, for example.
		//
		// Even though there can be just one handle, an application could use overlapped
		// I/O operations to issue several conflicting calls simultaneously. Hence the
		// use of a synchronization object to guard access to the "event" pointer. In
		// this particular driver, all access to the event pointer occurs at PASSIVE_LEVEL
		// and we could therefore use, e.g., a fast mutex. In general, though, you're going
		// to want to set the event from DISPATCH_LEVEL code. To make your life easier
		// if you're just cutting and pasting code, I therefore used a spin lock. Note
		// that this routine therefore has to be in the locked code segment.

		KIRQL oldirql;
		KeAcquireSpinLock(&pdx->eventlock, &oldirql);

		// First deregister any old event. Then remember the one we just located
		// or NULL.

		if (pdx->event)
			ObDereferenceObject(pdx->event);	// safe to call at <= DISPATCH_LEVEL
		pdx->event = pevent;

		// Now release the synchronization

		KeReleaseSpinLock(&pdx->eventlock, oldirql);
		break;
		}						// IOCTL_REGISTER_EVENT

	///////////////////////////////////////////////////////////////////////////
	// IOCTL_SIGNAL_EVENT event signals the shared event. This particular IOCTL
	// is a hack so you can have a way to test the registration aspect of this sample.
	// In a real situation, there would be some hardware event that would trigger
	// the signalling operation.

	case IOCTL_SIGNAL_EVENT:				// code == 0x801
		{						// IOCTL_SIGNAL_EVENT

		// Synchronize access to the event pointer. In particular, we need to worry about
		// someone coming along and dereferencing the event pointer while we're doing
		// our thing here. You would need synchronization like this in a real driver,
		// where a hardware event triggers the signalling.

		KIRQL oldirql;
		KeAcquireSpinLock(&pdx->eventlock, &oldirql);

		if (pdx->event)
			KeSetEvent(pdx->event, EVENT_INCREMENT, FALSE);

		KeReleaseSpinLock(&pdx->eventlock, oldirql);

		break;
		}						// IOCTL_SIGNAL_EVENT

	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;

		}						// process request

	return CompleteRequest(Irp, status, info);
	}							// DispatchControl
