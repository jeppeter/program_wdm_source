// Read/Write request processors for polling driver
// Copyright (C) 1999 by Walter Oney
// All rights reserved

#include "stddcls.h"
#include "driver.h"

// See the comments near the end of the polling routine about the following includes.
// The project settings for this file need to include the WIN_ME include directory
// in the DDK because of them. Unfortunately, the WIN_ME headers were dropped from the
// .NET DDK during beta, so you need the win98 kit installed in order to compile this file.
// If you have the 2K or XP kits, change the project settings for this file so that the
// "Additional Include Directories" option of "C++/Preprocessor" specifies $(DDKPATH)\inc\win_me
// instead of $(98DDK)\inc\win98.
//
// Note that BUILD isn't sufficiently flexibile to get the right headers included in this
// circumstance. Consequently, I needed to hand-assemble the VxD calls so as to avoid these
// VxD includes. (I did this by generating an ASM listing in Visual Studio and seeing what
// the VxD service ordinals were.)

#ifdef _X86_
	#ifdef DDKBUILD
		#define VMMCall(service) \
			_asm _emit 0xcd \
			_asm _emit 0x20 \
			_asm _emit ((service) & 0xff) \
			_asm _emit ((service) >> 8) & 0xff \
			_asm _emit ((service) >> 16) & 0xff \
			_asm _emit ((service) >> 24) & 0xff \

		#define Get_Cur_VM_Handle 0x00010001
		#define Adjust_Thread_Exec_Priority 0x00010114
		#define HIGH_PRI_DEVICE_BOOST 0x00001000

	#else
		#include <basedef.h>
		#include <vmm.h>
	#endif // DDKBUILD
#endif // _X86_

VOID OnCancelReadWrite(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS StartPollingThread(PDEVICE_EXTENSION pdx);
VOID StopPollingThread(PDEVICE_EXTENSION pdx);

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS DispatchCleanup(PDEVICE_OBJECT fdo, PIRP Irp)
	{							// DispatchCleanup
	PAGED_CODE();
	KdPrint((DRIVERNAME " - IRP_MJ_CLEANUP\n"));
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	CleanupRequests(&pdx->dqReadWrite, stack->FileObject, STATUS_CANCELLED);
	return CompleteRequest(Irp, STATUS_SUCCESS, 0);
	}							// DispatchCleanup

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS DispatchCreate(PDEVICE_OBJECT fdo, PIRP Irp)
	{							// DispatchCreate
	PAGED_CODE();
	KdPrint((DRIVERNAME " - IRP_MJ_CREATE\n"));
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

	NTSTATUS status = STATUS_SUCCESS;
	InterlockedIncrement(&pdx->handles);
	return CompleteRequest(Irp, status, 0);
	}							// DispatchCreate

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS DispatchClose(PDEVICE_OBJECT fdo, PIRP Irp)
	{							// DispatchClose
	PAGED_CODE();
	KdPrint((DRIVERNAME " - IRP_MJ_CLOSE\n"));
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	InterlockedDecrement(&pdx->handles);
	return CompleteRequest(Irp, STATUS_SUCCESS, 0);
	}							// DispatchClose

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS DispatchReadWrite(PDEVICE_OBJECT fdo, PIRP Irp)
	{							// DispatchReadWrite
	PAGED_CODE();
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	IoMarkIrpPending(Irp);
	StartPacket(&pdx->dqReadWrite, fdo, Irp, OnCancelReadWrite);
	return STATUS_PENDING;
	}							// DispatchReadWrite

#pragma LOCKEDCODE

VOID OnCancelReadWrite(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// OnCancelReadWrite
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	CancelRequest(&pdx->dqReadWrite, Irp);
	}							// OnCancelReadWrite

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE				// because of raised IRQL later on

VOID PollingThreadRoutine(PDEVICE_EXTENSION pdx)
	{							// PollingThreadRoutine
	KdPrint(("POLLING - Starting polling thread\n"));
	NTSTATUS status;
	KTIMER timer;
	KeInitializeTimerEx(&timer, SynchronizationTimer);

	// Wait for a request to arrive at our StartIoRead routine or for
	// someone to kill this thread.

	PVOID mainevents[] = {
		(PVOID) &pdx->evKill,
		(PVOID) &pdx->evRequest,
		};

	PVOID pollevents[] = {
		(PVOID) &pdx->evKill,
		(PVOID) &timer,
		};

	C_ASSERT(arraysize(mainevents) <= THREAD_WAIT_OBJECTS);
	C_ASSERT(arraysize(pollevents) <= THREAD_WAIT_OBJECTS);

	BOOLEAN kill = FALSE;
	
	while (!kill)
		{						// until told to quit
		status = KeWaitForMultipleObjects(arraysize(mainevents),
			mainevents, WaitAny, Executive, KernelMode, FALSE, NULL, NULL);
		if (!NT_SUCCESS(status))
			{					// error in wait
			KdPrint(("POLLING - KeWaitForMultipleObjects failed - %X\n", status));
			break;
			}					// error in wait
		if (status == STATUS_WAIT_0)
			break;				// kill event was set

		ULONG numxfer = 0;

		// Starting the timer with a zero due time will cause us to perform the
		// first poll immediately. Thereafter, polls occur at the POLLING_INTERVAL
		// interval (measured in milliseconds).

		LARGE_INTEGER duetime = {0};

		#define POLLING_INTERVAL 500
		KeSetTimerEx(&timer, duetime, POLLING_INTERVAL, NULL);

		PIRP Irp = GetCurrentIrp(&pdx->dqReadWrite);

		while (TRUE)
			{					// read next byte

			// Check to see if we should continue processing this IRP

			if (!Irp)
				break;			// no current IRP (so why did evRequest get signalled??)
			
			if (Irp->Cancel)
				{				// someone wants to cancel this IRP
				status = STATUS_CANCELLED;
				break;
				}				// someone wants to cancel this IRP
			
			if ((status = AreRequestsBeingAborted(&pdx->dqReadWrite)))
				break;			// powering down or something

			// Block until time to poll again

			status = KeWaitForMultipleObjects(arraysize(pollevents),
				pollevents, WaitAny, Executive, KernelMode, FALSE, NULL, NULL);
			
			if (!NT_SUCCESS(status))
				{					// error in wait
				KdPrint(("POLLING - KeWaitForMultipleObjects failed - %X\n", status));
				kill = TRUE;
				break;				// from read next byte
				}					// error in wait
			
			if (status == STATUS_WAIT_0)
				{					// told to quit
				status = STATUS_DELETE_PENDING;
				kill = TRUE;
				break;				// from read next byte
				}					// told to quit

			// Poll the device. Read a data byte if one is ready.

			if (pdx->nbytes)
				{				// request not yet satisfied
				#define CTLPORT_DATA_READY 1
				if (READ_PORT_UCHAR(pdx->portbase) == CTLPORT_DATA_READY)
					{			// data byte is ready
					*pdx->buffer++ = READ_PORT_UCHAR(pdx->portbase + 1);
					--pdx->nbytes;
					++numxfer;
					}			// data byte is ready
				}				// request not yet satisfied

			if (!pdx->nbytes)
				break;			// request now satisfied, leave loop
			}					// read next byte

		KeCancelTimer(&timer);
		StartNextPacket(&pdx->dqReadWrite, pdx->DeviceObject);
		if (Irp)
			CompleteRequest(Irp, status, numxfer);
		}						// until told to quit

	KdPrint(("POLLING - Terminating polling thread\n"));

	// In win98/me, StopPollingThread will be waiting for us to set the evDead
	// event. Conceivably, it would preempt us when we set the event, leaving
	// us unterminated while the driver gets unloaded. This probably doesn't
	// cause any real grief. Just to make sure, though, bump our priority way
	// up so we keep control after that other thread unblocks.

#ifdef _X86_

	if (win98)
		{						// win98 exit

		// The following VxD code is only executed in Win98/Me and therefore
		// won't cause a bug check. However, having the code here means
		// we need to include header files from the WinMe part of the DDK.

		VMMCall(Get_Cur_VM_Handle)
		_asm mov eax, HIGH_PRI_DEVICE_BOOST
		VMMCall(Adjust_Thread_Exec_Priority)

		KeSetEvent(&pdx->evDead, IO_NO_INCREMENT, FALSE);	// for win98/me
		}						// win98 exit

#endif _X86_

	PsTerminateSystemThread(STATUS_SUCCESS);
	}							// PollingThreadRoutine

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS StartDevice(PDEVICE_OBJECT fdo, PCM_PARTIAL_RESOURCE_LIST raw, PCM_PARTIAL_RESOURCE_LIST translated)
	{							// StartDevice
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	NTSTATUS status;

	// Identify the I/O resources we're supposed to use.

	PHYSICAL_ADDRESS portbase;
	BOOLEAN gotport = FALSE;
	
	if (!translated)
		return STATUS_DEVICE_CONFIGURATION_ERROR;		// no resources assigned??

	PCM_PARTIAL_RESOURCE_DESCRIPTOR resource = translated->PartialDescriptors;
	ULONG nres = translated->Count;
	for (ULONG i = 0; i < nres; ++i, ++resource)
		{						// for each resource
		switch (resource->Type)
			{					// switch on resource type

		case CmResourceTypePort:
			portbase = resource->u.Port.Start;
			pdx->nports = resource->u.Port.Length;
			pdx->mappedport = (resource->Flags & CM_RESOURCE_PORT_IO) == 0;
			gotport = TRUE;
			break;

		default:
			KdPrint((DRIVERNAME " - Unexpected I/O resource type %d\n", resource->Type));
			break;
			}					// switch on resource type
		}						// for each resource

	if (!(TRUE
		&& gotport
		))
		{
		KdPrint((DRIVERNAME " - Didn't get expected I/O resources\n"));
		return STATUS_DEVICE_CONFIGURATION_ERROR;
		}

	if (pdx->mappedport)
		{						// map port address for RISC platform
		pdx->portbase = (PUCHAR) MmMapIoSpace(portbase, pdx->nports, MmNonCached);
		if (!pdx->portbase)
			{
			KdPrint((DRIVERNAME " - Unable to map port range %I64X, length %X\n", portbase, pdx->nports));
			return STATUS_INSUFFICIENT_RESOURCES;
			}
		}						// map port address for RISC platform
	else
		pdx->portbase = (PUCHAR) portbase.QuadPart;

	status = StartPollingThread(pdx);

	return status;
	}							// StartDevice

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

VOID StartIo(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// StartIo
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

	pdx->buffer = (PUCHAR) Irp->AssociatedIrp.SystemBuffer;
	pdx->nbytes = stack->Parameters.Read.Length;

	KeSetEvent(&pdx->evRequest, 0, FALSE); // wake up polling thread
	}							// StartIo

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID StopDevice(IN PDEVICE_OBJECT fdo, BOOLEAN oktouch /* = FALSE */)
	{							// StopDevice
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	StopPollingThread(pdx);

	if (pdx->portbase && pdx->mappedport)
		MmUnmapIoSpace(pdx->portbase, pdx->nports);
	pdx->portbase = NULL;
	}							// StopDevice

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS StartPollingThread(PDEVICE_EXTENSION pdx)
	{							// StartPollingThread
	NTSTATUS status;
	HANDLE hthread;

	KeInitializeEvent(&pdx->evKill, NotificationEvent, FALSE);
	KeInitializeEvent(&pdx->evDead, NotificationEvent, FALSE);	// used in 98/me only
	KeInitializeEvent(&pdx->evRequest, SynchronizationEvent, FALSE);

#ifdef _X86_

	// Win98 fails PsCreateSystemThread with STATUS_INVALID_PARAMETER_3 if
	// an object attributes structure is used.

	if (win98)
		status = PsCreateSystemThread(&hthread, THREAD_ALL_ACCESS, NULL, NULL, NULL,
			(PKSTART_ROUTINE) PollingThreadRoutine, pdx);
	else
#endif // _X86_
		{						// NT

		// Make sure process handle is a kernel handle to prevent tampering by
		// user mode before we manage to get the KTHREAD pointer.

		OBJECT_ATTRIBUTES oa;
		InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

		status = PsCreateSystemThread(&hthread, THREAD_ALL_ACCESS, &oa, NULL, NULL,
			(PKSTART_ROUTINE) PollingThreadRoutine, pdx);
		}						// NT

	if (!NT_SUCCESS(status))
		{
		KdPrint((DRIVERNAME " - PsCreateSystemThread failed - %X\n", status));
		return status;
		}

	status = ObReferenceObjectByHandle(hthread, THREAD_ALL_ACCESS, NULL,
		KernelMode, (PVOID*) &pdx->thread, NULL);

	ZwClose(hthread);

	return STATUS_SUCCESS;
	}							// StartPollingThread

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID StopPollingThread(PDEVICE_EXTENSION pdx)
	{							// StopPollingThread
	KeSetEvent(&pdx->evKill, 0, FALSE);
	if (pdx->thread)
		{						// wait for thread to die

	#ifdef _X86_

		if (win98)
			{					// wait for thread to die

			// In Win98/Me, wait for the thread routine to set the "dead" event to
			// indicate it's about to exit. (Waiting on the thread object will cause
			// a crash). The polling thread will raise its priority before setting
			// this event, which should allow it to exit without preemption by us.
			// Note also the workaround here for the "KeWaitForSingleObject sometimes returns
			// -1" bug in Win98/Me.

			while (KeWaitForSingleObject(&pdx->evDead, Executive, KernelMode, FALSE, NULL) == (NTSTATUS) -1)
				;
			}					// wait for thread to die

		else

	#endif // _X86_

			// In NT, wait on the thread object
	
			KeWaitForSingleObject(pdx->thread, Executive, KernelMode, FALSE, NULL);
		
		// The thread object was referenced in StartPollingThread, so dereference it now.
		
		ObDereferenceObject(pdx->thread);
		pdx->thread = NULL;
		}						// wait for thread to die
	}							// StopPollingThread

