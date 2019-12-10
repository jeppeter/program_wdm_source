// Read/Write request processors for loopback driver
// Copyright (C) 1999 by Walter Oney
// All rights reserved

#include "stddcls.h"
#include "driver.h"

#if DBG
	#define MSGUSBSTRING(d,s,i) { \
		UNICODE_STRING sd; \
		if (i && NT_SUCCESS(GetStringDescriptor(d,i,&sd))) { \
			DbgPrint(s, sd.Buffer); \
			RtlFreeUnicodeString(&sd); \
		}}
#else
	#define MSGUSBSTRING(d,i,s)
#endif

struct _RWCONTEXT : public _URB
	{							// struct _RWCONTEXT
	ULONG_PTR va;				// virtual address for next segment of transfer
	ULONG length;				// length remaining to transfer
	PMDL mdl;					// partial MDL
	ULONG numxfer;				// cumulate transfer count
	BOOLEAN multistage;			// TRUE if multistage transfer

	// The following fields are used for error recovery only. Making this
	// structure bigger to contain them simply saves extra complication in the
	// code when we decide we need to recover the error

	PIO_WORKITEM rcitem;		// work item created for recovery
	PIRP Irp;					// the main IRP that we're going to fail
	};							// struct _RWCONTEXT

typedef struct _RWCONTEXT RWCONTEXT, *PRWCONTEXT;

ULONG GetStatus(PDEVICE_OBJECT fdo);
NTSTATUS OnReadWriteComplete(PDEVICE_OBJECT fdo, PIRP Irp, PRWCONTEXT ctx);
VOID OnCancelReadWrite(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
VOID RecoverFromError(PDEVICE_OBJECT fdo, PRWCONTEXT ctx);

///////////////////////////////////////////////////////////////////////////////
// AbortPipe is called as part of an attempt to recover after an I/O error to
// abort pending requests for a given pipe.

#pragma PAGEDCODE

VOID AbortPipe(PDEVICE_OBJECT fdo, USBD_PIPE_HANDLE hpipe)
	{							// AbortPipe
	PAGED_CODE();
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	URB urb;

	urb.UrbHeader.Length = (USHORT) sizeof(_URB_PIPE_REQUEST);
	urb.UrbHeader.Function = URB_FUNCTION_ABORT_PIPE;
	urb.UrbPipeRequest.PipeHandle = hpipe;

	NTSTATUS status = SendAwaitUrb(fdo, &urb);
	if (!NT_SUCCESS(status))
		KdPrint((DRIVERNAME " - Error %X in AbortPipe\n", status));
	}							// AbortPipe

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
// I (WCO) revised DispatchReadWrite in SP-7 to use a queue. The
// code that used to be in this routine was moved to StartIo. A queue is
// needed to avoid mixing segments from different IRPs in the calls
// down to the bus driver.

#pragma PAGEDCODE

NTSTATUS DispatchReadWrite(PDEVICE_OBJECT fdo, PIRP Irp)
	{							// DispatchReadWrite
	PAGED_CODE();
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	// This device can only handle up to 4096 bytes in one transfer

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	if (stack->Parameters.Read.Length > 4096)
		{
		KdPrint((DRIVERNAME " - Failing read/write bigger than 4096 bytes\n"));
		return CompleteRequest(Irp, STATUS_INVALID_PARAMETER, 0);
		}

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
// GetStatus is called during an attempt to recover after an error to determine
// the USBD status for the device.

#pragma PAGEDCODE

ULONG GetStatus(PDEVICE_OBJECT fdo)
	{							// GetStatus
	PAGED_CODE();
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	KEVENT event;
	KeInitializeEvent(&event, NotificationEvent, FALSE);
	IO_STATUS_BLOCK iostatus;
	ULONG portstatus;

	PIRP Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_GET_PORT_STATUS,
		pdx->LowerDeviceObject, NULL, 0, NULL, 0, TRUE, &event, &iostatus);
	if (!Irp)
		return 0;
	IoGetNextIrpStackLocation(Irp)->Parameters.Others.Argument1 = (PVOID) &portstatus;

	NTSTATUS status = IoCallDriver(pdx->LowerDeviceObject, Irp);
	if (status == STATUS_PENDING)
		{
		KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
		status = iostatus.Status;
		}
	
	if (!NT_SUCCESS(status))
		{
		KdPrint((DRIVERNAME " - Error %X trying to reset device\n", status));
		return 0;
		}

	return portstatus;
	}							// GetStatus

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS GetStringDescriptor(PDEVICE_OBJECT fdo, UCHAR istring, PUNICODE_STRING s)
	{							// GetStringDescriptor
	NTSTATUS status;
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	URB urb;

	UCHAR data[256];			// maximum-length buffer

	// If this is the first time here, read string descriptor zero and arbitrarily select
	// the first language identifer as the one to use in subsequent get-descriptor calls.

	if (!pdx->langid)
		{						// determine default language id
		UsbBuildGetDescriptorRequest(&urb, sizeof(_URB_CONTROL_DESCRIPTOR_REQUEST), USB_STRING_DESCRIPTOR_TYPE,
			0, 0, data, NULL, sizeof(data), NULL);
		status = SendAwaitUrb(fdo, &urb);
		if (!NT_SUCCESS(status))
			return status;
		pdx->langid = *(LANGID*)(data + 2);
		}						// determine default language id

	// Fetch the designated string descriptor.

	UsbBuildGetDescriptorRequest(&urb, sizeof(_URB_CONTROL_DESCRIPTOR_REQUEST), USB_STRING_DESCRIPTOR_TYPE,
		istring, pdx->langid, data, NULL, sizeof(data), NULL);
	status = SendAwaitUrb(fdo, &urb);
	if (!NT_SUCCESS(status))
		return status;

	ULONG nchars = (data[0] - sizeof(WCHAR)) / sizeof(WCHAR);
	if (nchars > 127)
		nchars = 127;
	PWSTR p = (PWSTR) ExAllocatePool(PagedPool, (nchars + 1) * sizeof(WCHAR));
	if (!p)
		return STATUS_INSUFFICIENT_RESOURCES;

	memcpy(p, data + 2, nchars * sizeof(WCHAR));
	p[nchars] = 0;

	s->Length = (USHORT) (sizeof(WCHAR) * nchars);
	s->MaximumLength = (USHORT) ((sizeof(WCHAR) * nchars) + sizeof(WCHAR));
	s->Buffer = p;

	return STATUS_SUCCESS;
	}							// GetStringDescriptor

///////////////////////////////////////////////////////////////////////////////
// This is the completion routine for IRP_MJ_READ and IRP_MJ_WRITE requests.

#pragma LOCKEDCODE

NTSTATUS OnReadWriteComplete(PDEVICE_OBJECT fdo, PIRP Irp, PRWCONTEXT ctx)
	{							// OnReadWriteComplete
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	BOOLEAN read = (ctx->UrbBulkOrInterruptTransfer.TransferFlags & USBD_TRANSFER_DIRECTION_IN) != 0;
	ctx->numxfer += ctx->UrbBulkOrInterruptTransfer.TransferBufferLength;

#if DBG
	USBD_STATUS urbstatus = URB_STATUS(ctx);
	if (!USBD_SUCCESS(urbstatus))
		KdPrint((DRIVERNAME " - %s failed with USBD status %X\n", read ? "Read" : "Write", urbstatus));
#endif

	// If this stage completed without error, resubmit the URB to perform the
	// next stage

	NTSTATUS status = Irp->IoStatus.Status;
	if (NT_SUCCESS(status) && ctx->length && !Irp->Cancel)
		{						// start next stage

		// Calculate length of next stage of the transfer

		ULONG seglen = ctx->length;
		if (seglen > pdx->maxtransfer)
			seglen = pdx->maxtransfer;

		// We're now in arbitrary thread context, so the virtual address of the
		// user buffer is currently meaningless. IoBuildPartialMdl copies physical
		// page numbers from the original IRP's probed-and-locked MDL, however,
		// so our process context doesn't matter.

		PMDL mdl = ctx->mdl;
		MmPrepareMdlForReuse(mdl);
		IoBuildPartialMdl(Irp->MdlAddress, mdl, (PVOID) ctx->va, seglen);

		// See the comment about this in DispatchReadWrite...

		if (!GenericGetSystemAddressForMdl(mdl))
			{						// can't map transfer segment
			KdPrint((DRIVERNAME " - Can't map memory for read or write\n"));
			status = STATUS_INSUFFICIENT_RESOURCES;
			Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;	// okay to change because DispatchReadWrite returned STATUS_PENDING
			goto FinishCompletion;
			}						// can't map transfer segment

		// Reinitialize the URB

		ctx->UrbBulkOrInterruptTransfer.TransferBufferLength = seglen;

		// The "next" stack location is the one belonging to the driver
		// underneath us. It's been mostly set to zero and must be reinitialized.

		PIO_STACK_LOCATION stack = IoGetNextIrpStackLocation(Irp);
		stack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
		stack->Parameters.Others.Argument1 = (PVOID) (PURB) ctx;
		stack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
		IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE) OnReadWriteComplete,
			(PVOID) ctx, TRUE, TRUE, TRUE);

		ctx->va += seglen;
		ctx->length -= seglen;

		// Pass the recycled IRP down and ignore the return code from IoCallDriver. If
		// this stage gets completed synchronously, this very completion routine will
		// already have been called recursively to deal with it. If not, the IRP is
		// now active again. In either case, we want the current invocation IoCompleteRequest
		// to stop working on this IRP.

		IoCallDriver(pdx->LowerDeviceObject, Irp);
		return STATUS_MORE_PROCESSING_REQUIRED; // halt completion process in its tracks!
		}						// start next stage

	// The request is complete now

FinishCompletion:				// label reached if MDL locking fails
	if (NT_SUCCESS(status))
		Irp->IoStatus.Information = ctx->numxfer;
	else
		{					// had an error
		if (read)
			InterlockedIncrement(&pdx->inerror);
		else
			InterlockedIncrement(&pdx->outerror);

		KdPrint((DRIVERNAME " - %s finished with error %X\n", read ? "Read" : "Write", status));

		if (status != STATUS_CANCELLED)
			{					// recover after I/O error

			// Attempt to recover after an error. Whether or not the attempt succeeds, this IRP
			// is a goner and will eventually get completed with its current error status. Since
			// StartIo is serializing IRPs, we needn't worry about anyone else trying to read or
			// write the device while this is going on. We have to call RecoverFromError at PASSIVE_LEVEL,
			// though. If we were tricky and did that directly from here, we would need to
			// use IoSetCompletionRoutineEx because whatever unload protection our caller has
			// done expires as soon as the main IRP completes (and RecoverFromError completes the
			// main IRP.) Therefore, always queue a work item, which independently protects us
			// from unloading.

			ctx->rcitem = IoAllocateWorkItem(fdo);
			if (!ctx->rcitem)
				KdPrint((DRIVERNAME " - Can't allocate work item for error recovery\n"));
			else
				{			// try to recover
				ctx->Irp = Irp;
				IoQueueWorkItem(ctx->rcitem, (PIO_WORKITEM_ROUTINE) RecoverFromError, CriticalWorkQueue, (PVOID) ctx);
				return STATUS_MORE_PROCESSING_REQUIRED;	// defer completion a while longer
				}			// try to recover
			}					// recover after I/O error
		}					// had an error

	IoFreeMdl(ctx->mdl);
	ExFreePool(ctx);
	StartNextPacket(&pdx->dqReadWrite, fdo);
	IoReleaseRemoveLock(&pdx->RemoveLock, Irp);

	return STATUS_SUCCESS;		// allow IRP to continue completing
	}							// OnReadWriteComplete

///////////////////////////////////////////////////////////////////////////////
// RecoverFromError is called in a system worker thread to attempt recovery
// after a device error.

#pragma PAGEDCODE

VOID RecoverFromError(PDEVICE_OBJECT fdo, PRWCONTEXT ctx)
	{							// RecoverFromError
	PAGED_CODE();
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	BOOLEAN read = (ctx->UrbBulkOrInterruptTransfer.TransferFlags & USBD_TRANSFER_DIRECTION_IN) != 0;

	KdPrint((DRIVERNAME " - Beginning error recovery\n"));

	// Determine the status of our device

	ULONG portstatus = GetStatus(fdo);

	// If the port is connected but not enabled, try resetting it

	USBD_PIPE_HANDLE hpipe = read ? pdx->hinpipe : pdx->houtpipe;
	if (!(portstatus & USBD_PORT_ENABLED) && (portstatus & USBD_PORT_CONNECTED))
		{						// reset port
		KdPrint((DRIVERNAME " - Error recovery aborting pipe\n"));
		AbortPipe(fdo, hpipe);

		KdPrint((DRIVERNAME " - Error recovery resetting device\n"));
		ResetDevice(fdo);
		}						// reset port

	// Now reset the failed endpoint

	ResetPipe(fdo, hpipe);

	KdPrint((DRIVERNAME " - Finished with error recovery\n"));

	IoFreeWorkItem(ctx->rcitem);

	// Finish completing the failed IRP. Note that our completion routine will
	// not be called this time, so we have to do all the cleanup here

	PIRP Irp = ctx->Irp;
	IoFreeMdl(ctx->mdl);
	ExFreePool(ctx);
	StartNextPacket(&pdx->dqReadWrite, fdo);
	IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	IoCompleteRequest(Irp, IO_NO_INCREMENT);	// resume completion of failed IRP
	}							// RecoverFromError

///////////////////////////////////////////////////////////////////////////////
// ResetDevice is called during an attempt to recover after an error in order to
// reset the device.

#pragma PAGEDCODE

VOID ResetDevice(PDEVICE_OBJECT fdo)
	{							// ResetDevice
	PAGED_CODE();
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	KEVENT event;
	KeInitializeEvent(&event, NotificationEvent, FALSE);
	IO_STATUS_BLOCK iostatus;

	PIRP Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_RESET_PORT,
		pdx->LowerDeviceObject, NULL, 0, NULL, 0, TRUE, &event, &iostatus);
	if (!Irp)
		return;

	NTSTATUS status = IoCallDriver(pdx->LowerDeviceObject, Irp);
	if (status == STATUS_PENDING)
		{
		KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
		status = iostatus.Status;
		}
	
	if (!NT_SUCCESS(status))
		KdPrint((DRIVERNAME " - Error %X trying to reset device\n", status));
	}							// ResetDevice

///////////////////////////////////////////////////////////////////////////////
// ResetDevice is called during an attempt to recover after an error in order to
// reset the pipe that had the error

#pragma PAGEDCODE

NTSTATUS ResetPipe(PDEVICE_OBJECT fdo, USBD_PIPE_HANDLE hpipe)
	{							// ResetPipe
	PAGED_CODE();
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	URB urb;

	urb.UrbHeader.Length = (USHORT) sizeof(_URB_PIPE_REQUEST);
	urb.UrbHeader.Function = URB_FUNCTION_RESET_PIPE;
	urb.UrbPipeRequest.PipeHandle = hpipe;

	NTSTATUS status = SendAwaitUrb(fdo, &urb);
	if (!NT_SUCCESS(status))
		KdPrint((DRIVERNAME " - Error %X trying to reset a pipe\n", status));
	return status;
	}							// ResetPipe

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS SendAwaitUrb(PDEVICE_OBJECT fdo, PURB urb)
	{							// SendAwaitUrb
	PAGED_CODE();
	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	KEVENT event;
	KeInitializeEvent(&event, NotificationEvent, FALSE);

	IO_STATUS_BLOCK iostatus;
	PIRP Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_SUBMIT_URB,
		pdx->LowerDeviceObject, NULL, 0, NULL, 0, TRUE, &event, &iostatus);

	if (!Irp)
		{
		KdPrint((DRIVERNAME " - Unable to allocate IRP for sending URB\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
		}

	PIO_STACK_LOCATION stack = IoGetNextIrpStackLocation(Irp);
	stack->Parameters.Others.Argument1 = (PVOID) urb;
	NTSTATUS status = IoCallDriver(pdx->LowerDeviceObject, Irp);
	if (status == STATUS_PENDING)
		{
		KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
		status = iostatus.Status;
		}
	return status;
	}							// SendAwaitUrb

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS StartDevice(PDEVICE_OBJECT fdo, PCM_PARTIAL_RESOURCE_LIST raw, PCM_PARTIAL_RESOURCE_LIST translated)
	{							// StartDevice
	PAGED_CODE();
	NTSTATUS status;
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	URB urb;					// URB for use in this subroutine

	// Read our device descriptor. The only real purpose to this would be to find out how many
	// configurations there are so we can read their descriptors. In this simplest of examples,
	// there's only one configuration.

	UsbBuildGetDescriptorRequest(&urb, sizeof(_URB_CONTROL_DESCRIPTOR_REQUEST), USB_DEVICE_DESCRIPTOR_TYPE,
		0, 0, &pdx->dd, NULL, sizeof(pdx->dd), NULL);
	status = SendAwaitUrb(fdo, &urb);
	if (!NT_SUCCESS(status))
		{
		KdPrint((DRIVERNAME " - Error %X trying to read device descriptor\n", status));
		return status;
		}

	MSGUSBSTRING(fdo, DRIVERNAME " - Configuring device from %ws\n", pdx->dd.iManufacturer);
	MSGUSBSTRING(fdo, DRIVERNAME " - Product is %ws\n", pdx->dd.iProduct);
	MSGUSBSTRING(fdo, DRIVERNAME " - Serial number is %ws\n", pdx->dd.iSerialNumber);

	// Read the descriptor of the first configuration. This requires two steps. The first step
	// reads the fixed-size configuration descriptor alone. The second step reads the
	// configuration descriptor plus all imbedded interface and endpoint descriptors.

	USB_CONFIGURATION_DESCRIPTOR tcd;
	UsbBuildGetDescriptorRequest(&urb, sizeof(_URB_CONTROL_DESCRIPTOR_REQUEST), USB_CONFIGURATION_DESCRIPTOR_TYPE,
		0, 0, &tcd, NULL, sizeof(tcd), NULL);
	status = SendAwaitUrb(fdo, &urb);
	if (!NT_SUCCESS(status))
		{
		KdPrint((DRIVERNAME " - Error %X trying to read configuration descriptor 1\n", status));
		return status;
		}

	ULONG size = tcd.wTotalLength;
	PUSB_CONFIGURATION_DESCRIPTOR pcd = (PUSB_CONFIGURATION_DESCRIPTOR) ExAllocatePool(NonPagedPool, size);
	if (!pcd)
		{
		KdPrint((DRIVERNAME " - Unable to allocate %X bytes for configuration descriptor\n", size));
		return STATUS_INSUFFICIENT_RESOURCES;
		}

	__try
		{
		UsbBuildGetDescriptorRequest(&urb, sizeof(_URB_CONTROL_DESCRIPTOR_REQUEST), USB_CONFIGURATION_DESCRIPTOR_TYPE,
			0, 0, pcd, NULL, size, NULL);
		status = SendAwaitUrb(fdo, &urb);
		if (!NT_SUCCESS(status))
			{
			KdPrint((DRIVERNAME " - Error %X trying to read configuration descriptor 1\n", status));
			return status;
			}
                                   
		MSGUSBSTRING(fdo, DRIVERNAME " - Selecting configuration named %ws\n", pcd->iConfiguration);

		// Locate the descriptor for the one and only interface we expect to find

		PUSB_INTERFACE_DESCRIPTOR pid = USBD_ParseConfigurationDescriptorEx(pcd, pcd,
			-1, -1, -1, -1, -1);
		ASSERT(pid);
                                   
		MSGUSBSTRING(fdo, DRIVERNAME " - Selecting interface named %ws\n", pid->iInterface);

		// Create a URB to use in selecting a configuration.

		USBD_INTERFACE_LIST_ENTRY interfaces[2] = {
			{pid, NULL},
			{NULL, NULL},		// fence to terminate the array
			};

		PURB selurb = USBD_CreateConfigurationRequestEx(pcd, interfaces);
		if (!selurb)
			{
			KdPrint((DRIVERNAME " - Unable to create configuration request\n"));
			return STATUS_INSUFFICIENT_RESOURCES;
			}

		__try
			{

			// Verify that the interface describes exactly the endpoints we expect

			if (pid->bNumEndpoints != 2)
				{
				KdPrint((DRIVERNAME " - %d is the wrong number of endpoints\n", pid->bNumEndpoints));
				return STATUS_DEVICE_CONFIGURATION_ERROR;
				}

			PUSB_ENDPOINT_DESCRIPTOR ped = (PUSB_ENDPOINT_DESCRIPTOR) pid;
			ped = (PUSB_ENDPOINT_DESCRIPTOR) USBD_ParseDescriptors(pcd, tcd.wTotalLength, ped, USB_ENDPOINT_DESCRIPTOR_TYPE);
			if (!ped || ped->bEndpointAddress != 0x82 || ped->bmAttributes != USB_ENDPOINT_TYPE_BULK || ped->wMaxPacketSize != 64)
				{
				KdPrint((DRIVERNAME " - Endpoint has wrong attributes\n"));
				return STATUS_DEVICE_CONFIGURATION_ERROR;
				}
			++ped;
			if (!ped || ped->bEndpointAddress != 0x2 || ped->bmAttributes != USB_ENDPOINT_TYPE_BULK || ped->wMaxPacketSize != 64)
				{
				KdPrint((DRIVERNAME " - Endpoint has wrong attributes\n"));
				return STATUS_DEVICE_CONFIGURATION_ERROR;
				}
			++ped;

			PUSBD_INTERFACE_INFORMATION pii = interfaces[0].Interface;
			ASSERT(pii->NumberOfPipes == pid->bNumEndpoints);

			// Initialize the maximum transfer size for each of the endpoints. The
			// default would be PAGE_SIZE. The firmware itself only has a 4096-byte
			// ring buffer, though. We need to restrict the test applet to that many
			// bytes. In order to exercise the multi-segment aspect of the transfer code,
			// therefore, reduce the maximum transfer size to 1024 bytes.

			pii->Pipes[0].MaximumTransferSize = 1024;
			pii->Pipes[1].MaximumTransferSize = 1024;
			pdx->maxtransfer = 1024;	// save for use in handling reads & writes

			// Submit the set-configuration request

			status = SendAwaitUrb(fdo, selurb);
			if (!NT_SUCCESS(status))
				{
				KdPrint((DRIVERNAME " - Error %X trying to select configuration\n", status));
				return status;
				}

			// Save the configuration and pipe handles

			pdx->hconfig = selurb->UrbSelectConfiguration.ConfigurationHandle;
			pdx->hinpipe = pii->Pipes[0].PipeHandle;
			pdx->houtpipe = pii->Pipes[1].PipeHandle;

			// Transfer ownership of the configuration descriptor to the device extension
			
			pdx->pcd = pcd;
			pcd = NULL;
			}
		__finally
			{
			ExFreePool(selurb);
			}

		}
	__finally
		{
		if (pcd)
			ExFreePool(pcd);
		}

	return STATUS_SUCCESS;
	}							// StartDevice

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

VOID StartIo(PDEVICE_OBJECT fdo, PIRP Irp)
	{							// StartIo
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	// Use the remove lock to guard against lower drivers disappearing while 
	// this IRP is active. Whoever sent us this IRP is protecting us from removal
	// at least until our completion routine returns STATUS_SUCCESS, which is why
	// we don't need to use IoSetCompletionRoutineEx in this driver.

	NTSTATUS status = IoAcquireRemoveLock(&pdx->RemoveLock, Irp);
	if (!NT_SUCCESS(status))
		{
		CompleteRequest(Irp, status, 0);
		return;
		}
		
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	BOOLEAN read = stack->MajorFunction == IRP_MJ_READ;

	USBD_PIPE_HANDLE hpipe = read ? pdx->hinpipe : pdx->houtpipe;

	// Our strategy will be to do the transfer in stages up to MAXTRANSFER bytes
	// long using a single URB that we resubmit during the completion routine.
	// Note that chained URBs via UrbLink are not supported in either Win98 or Win2K.

	PRWCONTEXT ctx = (PRWCONTEXT) ExAllocatePool(NonPagedPool, sizeof(RWCONTEXT));
	if (!ctx)
		{
		KdPrint((DRIVERNAME " - Can't allocate memory for context structure\n"));
		StartNextPacket(&pdx->dqReadWrite, fdo);
		CompleteRequest(Irp, STATUS_INSUFFICIENT_RESOURCES, 0);
		IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
		return;
		}
	RtlZeroMemory(ctx, sizeof(RWCONTEXT));

	ULONG length = Irp->MdlAddress ?  MmGetMdlByteCount(Irp->MdlAddress) : 0;
	if (!length)
		{						// zero-length read
		StartNextPacket(&pdx->dqReadWrite, fdo);
		CompleteRequest(Irp, STATUS_SUCCESS, 0);
		IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
		return;
		}						// zero-length read
	ULONG_PTR va = (ULONG_PTR) MmGetMdlVirtualAddress(Irp->MdlAddress);

	ULONG urbflags =  USBD_SHORT_TRANSFER_OK | (read ? USBD_TRANSFER_DIRECTION_IN : USBD_TRANSFER_DIRECTION_OUT);

	// Calculate the length of the segment we'll transfer in the first stage. In the original
	// (pre-SP-5) version of this driver, I was transferring just enough data in the first
	// stage to get to a page boundary. John Hyde pointed out that this is incorrect unless the
	// buffer happens to be aligned on a 64-byte boundary because (according to section 5.8.3 of
	// the USB spec) all transfers except the last must be of maximum length. Therefore, I revised
	// this seample to just transfer the pipe maximum each time.

	ULONG seglen = length;
	if (seglen > pdx->maxtransfer)
		{
		seglen = pdx->maxtransfer;
		KdPrint((DRIVERNAME " - Read/write of %d bytes will be done in segments of %d\n",
			length, seglen));
		}

	// Allocate an MDL for each segment of the transfer. The parameters are chosen so
	// that the MDL will have room for a maximum-sized buffer in the worst case where
	// it starts just before a page boundary. (Note that the virtual address argument to
	// IoAllocateMdl is not actually used as an address.)

	PMDL mdl = IoAllocateMdl((PVOID) (PAGE_SIZE - 1), seglen, FALSE, FALSE, NULL);
	if (!mdl)
		{					// can't allocate MDL
		KdPrint((DRIVERNAME " - Can't allocate memory for MDL\n"));
		ExFreePool(ctx);
		StartNextPacket(&pdx->dqReadWrite, fdo);
		CompleteRequest(Irp, STATUS_INSUFFICIENT_RESOURCES, 0);
		IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
		}					// can't allocate MDL

	// Initialize the (partial) MDL to describe the first segment's subset of the user
	// buffer.

	IoBuildPartialMdl(Irp->MdlAddress, mdl, (PVOID) va, seglen);

	// Reader Peter Diaconesco ran across an apparent bug in the Win2K version of UHCD.SYS. Under
	// heavy load conditions, UHCD was bug-checking because its internal call to MmGetSystemAddressForMdl
	// was apparently returning NULL (even though it's not supposed to). We can prevent that problem
	// by mapping the pages in the following "safe" manner:

	if (!GenericGetSystemAddressForMdl(mdl))
		{						// can't map transfer segment
		KdPrint((DRIVERNAME " - Can't map memory for read or write\n"));
		ExFreePool(ctx);
		StartNextPacket(&pdx->dqReadWrite, fdo);
		CompleteRequest(Irp, STATUS_INSUFFICIENT_RESOURCES, 0);
		IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
		return;
		}						// can't map transfer segment

	UsbBuildInterruptOrBulkTransferRequest(ctx, sizeof(_URB_BULK_OR_INTERRUPT_TRANSFER),
		hpipe, NULL, mdl, seglen, urbflags, NULL);

	// Set context structure parameters to pick up where we just left off

	ctx->va = va + seglen;
	ctx->length = length - seglen;
	ctx->mdl = mdl;
	ctx->numxfer = 0;

	// Use the original Read or Write IRP as a container for the URB

	stack = IoGetNextIrpStackLocation(Irp);
	stack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	stack->Parameters.Others.Argument1 = (PVOID) (PURB) ctx;
	stack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

	IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE) OnReadWriteComplete,
		(PVOID) ctx, TRUE, TRUE, TRUE);

	IoCallDriver(pdx->LowerDeviceObject, Irp);
	}							// StartIo

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID StopDevice(IN PDEVICE_OBJECT fdo, BOOLEAN oktouch /* = FALSE */)
	{							// StopDevice
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	// If it's okay to touch our hardware (i.e., we're processing an IRP_MN_STOP_DEVICE),
	// deconfigure the device.
	
	if (oktouch)
		{						// deconfigure device
		URB urb;
		UsbBuildSelectConfigurationRequest(&urb, sizeof(_URB_SELECT_CONFIGURATION), NULL);
		NTSTATUS status = SendAwaitUrb(fdo, &urb);
		if (!NT_SUCCESS(status))
			KdPrint((DRIVERNAME " - Error %X trying to deconfigure device\n", status));
		}						// deconfigure device

	if (pdx->pcd)
		ExFreePool(pdx->pcd);
	pdx->pcd = NULL;
	}							// StopDevice
