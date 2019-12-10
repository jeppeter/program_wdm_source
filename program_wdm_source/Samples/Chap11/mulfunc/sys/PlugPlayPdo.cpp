// Plug and Play handlers for child device PDOs
// Copyright (C) 1999 by Walter Oney
// All rights reserved

#include "stddcls.h"
#include "driver.h"
#include <initguid.h>
#include "suballoc.h"

static NTSTATUS HandleQueryId(PDEVICE_OBJECT pdo, PIRP Irp);
static NTSTATUS HandleQueryInterface(PDEVICE_OBJECT pdo, PIRP Irp);
static NTSTATUS HandleQueryRelations(PDEVICE_OBJECT pdo, PIRP Irp);
static NTSTATUS IgnoreRequest(PDEVICE_OBJECT pdo, PIRP Irp);
static NTSTATUS OnRepeaterComplete(PDEVICE_OBJECT tdo, PIRP subirp, PVOID needsvote);
static NTSTATUS RepeatRequest(PDEVICE_OBJECT pdo, PIRP Irp);
static NTSTATUS SucceedRequest(PDEVICE_OBJECT pdo, PIRP Irp);
static VOID SuballocInterfaceReference(PDEVICE_EXTENSION fdx); 
static VOID SuballocInterfaceDereference(PDEVICE_EXTENSION fdx); 
static NTSTATUS GetChildResources(PDEVICE_EXTENSION fdx, PDEVICE_OBJECT pdo, PCM_RESOURCE_LIST* raw, PCM_RESOURCE_LIST* translated);

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS DispatchPnpPdo(IN PDEVICE_OBJECT pdo, IN PIRP Irp)
	{							// DispatchPnpPdo
	PAGED_CODE();
	PPDO_EXTENSION pdx = (PPDO_EXTENSION) pdo->DeviceExtension;

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	ASSERT(stack->MajorFunction == IRP_MJ_PNP);
	ASSERT(pdx->flags & ISPDO);
	NTSTATUS status;

	static NTSTATUS (*fcntab[])(IN PDEVICE_OBJECT pdo, IN PIRP Irp) = {
		SucceedRequest,			// IRP_MN_START_DEVICE
		SucceedRequest,			// IRP_MN_QUERY_REMOVE_DEVICE
		SucceedRequest,			// IRP_MN_REMOVE_DEVICE
		SucceedRequest,			// IRP_MN_CANCEL_REMOVE_DEVICE
		SucceedRequest,			// IRP_MN_STOP_DEVICE
		SucceedRequest,			// IRP_MN_QUERY_STOP_DEVICE
		SucceedRequest,			// IRP_MN_CANCEL_STOP_DEVICE
		HandleQueryRelations,	// IRP_MN_QUERY_DEVICE_RELATIONS
		HandleQueryInterface,	// IRP_MN_QUERY_INTERFACE
		RepeatRequest,			// IRP_MN_QUERY_CAPABILITIES
		SucceedRequest,			// IRP_MN_QUERY_RESOURCES
		SucceedRequest,			// IRP_MN_QUERY_RESOURCE_REQUIREMENTS
		IgnoreRequest,			// IRP_MN_QUERY_DEVICE_TEXT
		SucceedRequest,			// IRP_MN_FILTER_RESOURCE_REQUIREMENTS
		SucceedRequest,			// 
		RepeatRequest,			// IRP_MN_READ_CONFIG
		RepeatRequest,			// IRP_MN_WRITE_CONFIG
		RepeatRequest,			// IRP_MN_EJECT
		RepeatRequest,			// IRP_MN_SET_LOCK
		HandleQueryId,			// IRP_MN_QUERY_ID
		RepeatRequest,			// IRP_MN_QUERY_PNP_DEVICE_STATE
		RepeatRequest,			// IRP_MN_QUERY_BUS_INFORMATION
		RepeatRequest,			// IRP_MN_DEVICE_USAGE_NOTIFICATION
		SucceedRequest,			// IRP_MN_SURPRISE_REMOVAL
		RepeatRequest,			// IRP_MN_QUERY_LEGACY_BUS_INFORMATION
		};

	ULONG fcn = stack->MinorFunction;
	if (fcn >= arraysize(fcntab))
		{						// unknown function
		status = IgnoreRequest(pdo, Irp); // some function we don't know about
		return status;
		}						// unknown function

#if DBG
	static char* fcnname[] = {
		"IRP_MN_START_DEVICE",
		"IRP_MN_QUERY_REMOVE_DEVICE",
		"IRP_MN_REMOVE_DEVICE",
		"IRP_MN_CANCEL_REMOVE_DEVICE",
		"IRP_MN_STOP_DEVICE",
		"IRP_MN_QUERY_STOP_DEVICE",
		"IRP_MN_CANCEL_STOP_DEVICE",
		"IRP_MN_QUERY_DEVICE_RELATIONS",
		"IRP_MN_QUERY_INTERFACE",
		"IRP_MN_QUERY_CAPABILITIES",
		"IRP_MN_QUERY_RESOURCES",
		"IRP_MN_QUERY_RESOURCE_REQUIREMENTS",
		"IRP_MN_QUERY_DEVICE_TEXT",
		"IRP_MN_FILTER_RESOURCE_REQUIREMENTS",
		"",
		"IRP_MN_READ_CONFIG",
		"IRP_MN_WRITE_CONFIG",
		"IRP_MN_EJECT",
		"IRP_MN_SET_LOCK",
		"IRP_MN_QUERY_ID",
		"IRP_MN_QUERY_PNP_DEVICE_STATE",
		"IRP_MN_QUERY_BUS_INFORMATION",
		"IRP_MN_DEVICE_USAGE_NOTIFICATION",
		"IRP_MN_SURPRISE_REMOVAL",
		"IRP_MN_QUERY_LEGACY_BUS_INFORMATION",
		};

	KdPrint((DRIVERNAME " (PDO) - PNP Request (%s)\n", fcnname[fcn]));
#endif // DBG

	status = (*fcntab[fcn])(pdo, Irp);
	return status;
	}							// DispatchPnp

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

static NTSTATUS HandleQueryId(PDEVICE_OBJECT pdo, PIRP Irp)
	{							// HandleQueryId
	PPDO_EXTENSION pdx = (PPDO_EXTENSION) pdo->DeviceExtension;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

	PWCHAR idstring;

	switch (stack->Parameters.QueryId.IdType)
		{						// select based on id type

	case BusQueryInstanceID:
		idstring = L"0000";
		break;

	// For the device ID, we need to supply an enumerator name plus a device identifer.
	// The enumerator name is something you should choose to be unique, which is why
	// I used the name of the driver in this instance.

	case BusQueryDeviceID:
		idstring = (pdx->flags & CHILDTYPEA) ? LDRIVERNAME L"\\*WCO1104" : LDRIVERNAME L"\\*WCO1105";
		break;

	case BusQueryHardwareIDs:
		idstring = (pdx->flags & CHILDTYPEA) ? L"*WCO1104" : L"*WCO1105";
		break;

	case BusQueryCompatibleIDs:
	default:
		return CompleteRequest(Irp, STATUS_NOT_SUPPORTED, 0);

		}						// select based on id type

	ULONG nchars = wcslen(idstring);
	ULONG size = (nchars + 2) * sizeof(WCHAR);
	PWCHAR id = (PWCHAR) ExAllocatePool(PagedPool, size);
	if (!id)
		return CompleteRequest(Irp, STATUS_INSUFFICIENT_RESOURCES, 0);
	wcscpy(id, idstring);
	id[nchars+1] = 0;			// extra null terminator

	return CompleteRequest(Irp, STATUS_SUCCESS, (ULONG_PTR) id);
	}							// HandleQueryId

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

static NTSTATUS HandleQueryInterface(PDEVICE_OBJECT pdo, PIRP Irp)
	{							// HandleQueryInterface
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	
	// See if caller is asking for the resource suballocation interface. If not,
	// repeat this request in the parent device stack. Even though the DDK doc says
	// you shouldn't do this, there is no other way for a child-stack driver to get
	// at one of the interfaces implemented by a parent-stack driver. And, yes, it
	// is true that the parent stack can't be removed until all references to such
	// interfaces are released, but that's not especially fascinating news.

	if (*stack->Parameters.QueryInterface.InterfaceType != GUID_RESOURCE_SUBALLOCATE_STANDARD)
		return RepeatRequest(pdo, Irp);

	// Determine what version of the interface to return.

	USHORT version = RESOURCE_SUBALLOCATE_STANDARD_VERSION; // assume current version
	if (version > stack->Parameters.QueryInterface.Version)
		version = stack->Parameters.QueryInterface.Version; // negotiate down to requested version
	if (version == 0)
		return CompleteRequest(Irp, Irp->IoStatus.Status);	// fail -- no compatible version supported
	
	PPDO_EXTENSION pdx = (PPDO_EXTENSION) pdo->DeviceExtension;
	PDEVICE_OBJECT fdo = pdx->Fdo;
	PDEVICE_EXTENSION fdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	// Fill in caller's interface descriptor structure

	switch (version)
		{						// fill in specified version of interface structure

	case 1:
		{						// version 1
		if (stack->Parameters.QueryInterface.Size < sizeof(RESOURCE_SUBALLOCATE_STANDARD))
			return CompleteRequest(Irp, STATUS_INVALID_PARAMETER); // not big enough
		PRESOURCE_SUBALLOCATE_STANDARD ifp = (PRESOURCE_SUBALLOCATE_STANDARD) stack->Parameters.QueryInterface.Interface;
		ifp->Size = sizeof(RESOURCE_SUBALLOCATE_STANDARD);
		ifp->Version = 1;
		ifp->Context = (PVOID) fdx;
		ifp->InterfaceReference = (PINTERFACE_REFERENCE) SuballocInterfaceReference;
		ifp->InterfaceDereference = (PINTERFACE_DEREFERENCE) SuballocInterfaceDereference;
		ifp->GetResources = (PGETRESOURCES) GetChildResources;
		break;
		}						// version 1

	default:
		ASSERT(FALSE);			// version negotiation above went awry
		return CompleteRequest(Irp, Irp->IoStatus.Status);
		}						// fill in specified version of interface structure

	SuballocInterfaceReference(fdx);
	return CompleteRequest(Irp, STATUS_SUCCESS, 0);
	}							// HandleQueryInterface

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

static NTSTATUS HandleQueryRelations(PDEVICE_OBJECT pdo, PIRP Irp)
	{							// HandleQueryRelations
	PDEVICE_RELATIONS oldrel = (PDEVICE_RELATIONS) Irp->IoStatus.Information;
	PDEVICE_RELATIONS newrel = NULL;
	NTSTATUS status = Irp->IoStatus.Status;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

	if (stack->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation)
		{							// query for PDO address
		ASSERT(!oldrel);			// no-one has any business with this but us!
		newrel = (PDEVICE_RELATIONS) ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS));
		if (newrel)
			{
			newrel->Count = 1;
			newrel->Objects[0] = pdo;
			ObReferenceObject(pdo);
			status = STATUS_SUCCESS;
			}
		else
			status = STATUS_INSUFFICIENT_RESOURCES;
		}							// query for PDO address

	if (newrel)
		{							// install new relation list
		if (oldrel)
			ExFreePool(oldrel);
		Irp->IoStatus.Information = (ULONG_PTR) newrel;
		}							// install new relation list

	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
	}							// HandleQueryRelations

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

static NTSTATUS IgnoreRequest(PDEVICE_OBJECT pdo, PIRP Irp)
	{							// IgnoreRequest
	NTSTATUS status = Irp->IoStatus.Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
	}							// IgnoreRequest

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

static NTSTATUS OnRepeaterComplete(PDEVICE_OBJECT tdo, PIRP subirp, PVOID needsvote)
	{							// OnRepeaterComplete
	ObDereferenceObject(tdo);
	PIO_STACK_LOCATION substack = IoGetCurrentIrpStackLocation(subirp);
	PIRP Irp = (PIRP) substack->Parameters.Others.Argument1;

	// Complete the main IRP. We need to copy IoStatus.Information from the
	// now-complete repeater IRP, inasmuch as it contains the "return value"
	// from the IRP. If the repeater IRP still holds STATUS_NOT_SUPPORTED as
	// its status, the parent device stack didn't actually process the IRP.
	// The status we then set depends on the "needsvote" value developed by
	// RepeatRequest.

	if (subirp->IoStatus.Status == STATUS_NOT_SUPPORTED)
		{						// parent stack ignored this IRP
		if (needsvote)
			Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
		}						// parent stack ignored this IRP
	else
		Irp->IoStatus = subirp->IoStatus; // propagate status from parent stack

	Irp->IoStatus.Information = subirp->IoStatus.Information;

	IoFreeIrp(subirp);
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_MORE_PROCESSING_REQUIRED;
	}							// OnRepeaterComplete

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

static NTSTATUS RepeatRequest(PDEVICE_OBJECT pdo, PIRP Irp)
	{							// RepeatRequest
	PPDO_EXTENSION pdx = (PPDO_EXTENSION) pdo->DeviceExtension;
	PDEVICE_OBJECT fdo = pdx->Fdo;
	PDEVICE_EXTENSION pfx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

	PDEVICE_OBJECT tdo = IoGetAttachedDeviceReference(fdo);
	PIRP subirp = IoAllocateIrp(tdo->StackSize + 1, FALSE);

	if (!subirp)
		return CompleteRequest(Irp, STATUS_INSUFFICIENT_RESOURCES, Irp->IoStatus.Information);

	// Setup extra stack location at the top so we can cleanup and complete
	// the main IRP

	PIO_STACK_LOCATION substack = IoGetNextIrpStackLocation(subirp);
	substack->DeviceObject = tdo;
	substack->Parameters.Others.Argument1 = (PVOID) Irp;

	// Setup next stack location for the FDO stack to process the repeater IRP

	IoSetNextIrpStackLocation(subirp);
	substack = IoGetNextIrpStackLocation(subirp);
	RtlCopyMemory(substack, stack, FIELD_OFFSET(IO_STACK_LOCATION, CompletionRoutine));
	substack->Control = 0;

	// Our completion routine needs a way to know what to do if the parent driver stack
	// completes the subsidiary IRP with STATUS_NOT_SUPPORTED. For some IRPs (ones that
	// require a parental "vote" on their status), we want to fail the IRPs if no-one
	// in the parent stack handles it. For others, we don't want to change the status
	// passed down to us from the child stack. Since we're not using the context
	// argument for any other purpose in our completion routine, we pass a simple
	// boolean value to distinguish the two cases.

	UCHAR subfunc = stack->MinorFunction;
	static BOOLEAN voteflag[] = {
		FALSE,			// IRP_MN_START_DEVICE [don't care since not repeated]
		FALSE,			// IRP_MN_QUERY_REMOVE_DEVICE [don't care since not repeated]
		FALSE,			// IRP_MN_REMOVE_DEVICE [don't care since not repeated]
		FALSE,			// IRP_MN_CANCEL_REMOVE_DEVICE [don't care since not repeated]
		FALSE,			// IRP_MN_STOP_DEVICE [don't care since not repeated]
		FALSE,			// IRP_MN_QUERY_STOP_DEVICE [don't care since not repeated]
		FALSE,			// IRP_MN_CANCEL_STOP_DEVICE [don't care since not repeated]
		FALSE,			// IRP_MN_QUERY_DEVICE_RELATIONS [don't care since not repeated]
		FALSE,			// IRP_MN_QUERY_INTERFACE
		FALSE,			// IRP_MN_QUERY_CAPABILITIES
		FALSE,			// IRP_MN_QUERY_RESOURCES [don't care since not repeated]
		FALSE,			// IRP_MN_QUERY_RESOURCE_REQUIREMENTS [don't care since not repeated]
		FALSE,			// IRP_MN_QUERY_DEVICE_TEXT [don't care since not repeated]
		FALSE,			// IRP_MN_FILTER_RESOURCE_REQUIREMENTS [don't care since not repeated]
		FALSE,			//  [don't care since not repeated]
		TRUE,			// IRP_MN_READ_CONFIG
		TRUE,			// IRP_MN_WRITE_CONFIG
		TRUE,			// IRP_MN_EJECT [no-one above us should have changed status anyway]
		TRUE,			// IRP_MN_SET_LOCK [no-one above us should have changed status anyway]
		FALSE,			// IRP_MN_QUERY_ID [don't care since not repeated]
		FALSE,			// IRP_MN_QUERY_PNP_DEVICE_STATE
		TRUE,			// IRP_MN_QUERY_BUS_INFORMATION
		FALSE,			// IRP_MN_DEVICE_USAGE_NOTIFICATION
		FALSE,			// IRP_MN_SURPRISE_REMOVAL [don't care since not repeated]
		TRUE,			// IRP_MN_QUERY_LEGACY_BUS_INFORMATION
		};

	BOOLEAN needsvote = subfunc < arraysize(voteflag) ? voteflag[subfunc] : TRUE;

	IoSetCompletionRoutine(subirp, (PIO_COMPLETION_ROUTINE) OnRepeaterComplete, (PVOID) needsvote, TRUE, TRUE, TRUE);

	// Initialize completion status for the repeater IRP to STATUS_NOT_SUPPORTED.
	// This is required to avoid a bug check in the driver verifier. The underlying
	// reason is that some drivers in some parent stacks (not MULFUNC's, because it's
	// not a real device, but in general) need a way to know when someone truly handles
	// this IRP.

	subirp->IoStatus.Status = STATUS_NOT_SUPPORTED;

	// Return STATUS_PENDING because we won't know until completion time what the
	// right status is (and it may not be what the parent stack returns here).

	IoMarkIrpPending(Irp);
	IoCallDriver(tdo, subirp);
	return STATUS_PENDING;
	}							// RepeatRequest

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

static NTSTATUS SucceedRequest(PDEVICE_OBJECT pdo, PIRP Irp)
	{							// SucceedRequest
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
	}							// SucceedRequest

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VOID SuballocInterfaceReference(PDEVICE_EXTENSION fdx)
	{							// SuballocInterfaceReference
	InterlockedIncrement(&fdx->sarefcnt);
	}							// SuballocInterfaceReference

VOID SuballocInterfaceDereference(PDEVICE_EXTENSION fdx)
	{							// SuballocInterfaceDereference
	LONG count = InterlockedDecrement(&fdx->sarefcnt);
	ASSERT(count >= 0);
	}							// SuballocInterfaceDereference

NTSTATUS GetChildResources(PDEVICE_EXTENSION fdx, PDEVICE_OBJECT pdo, PCM_RESOURCE_LIST* praw, PCM_RESOURCE_LIST* ptranslated)
	{							// GetChildResources
	ASSERT(fdx->sarefcnt > 0);	// someone needs to still have a reference

	if (pdo != fdx->ChildA && pdo != fdx->ChildB)
		return STATUS_DEVICE_CONFIGURATION_ERROR; // who's that asking??

	ULONG size = sizeof(CM_RESOURCE_LIST);
	
	PCM_RESOURCE_LIST raw, translated;
	raw = (PCM_RESOURCE_LIST) ExAllocatePool(PagedPool, size);
	if (!raw)
		return STATUS_INSUFFICIENT_RESOURCES;
	translated = (PCM_RESOURCE_LIST) ExAllocatePool(PagedPool, size);
	if (!translated)
		{
		ExFreePool(raw);
		return STATUS_INSUFFICIENT_RESOURCES;
		}

	RtlZeroMemory(raw, size);
	RtlZeroMemory(translated, size);

	raw->Count = 1;
	raw->List[0].PartialResourceList.Version = 1;
	raw->List[0].PartialResourceList.Revision = 1;
	raw->List[0].PartialResourceList.Count = 1;
	raw->List[0].PartialResourceList.PartialDescriptors[0] = (pdo == fdx->ChildA) ? fdx->PortARaw : fdx->PortBRaw;

	translated->Count = 1;
	translated->List[0].PartialResourceList.Version = 1;
	translated->List[0].PartialResourceList.Revision = 1;
	translated->List[0].PartialResourceList.Count = 1;
	translated->List[0].PartialResourceList.PartialDescriptors[0] = (pdo == fdx->ChildA) ? fdx->PortATranslated : fdx->PortBTranslated;

	*praw = raw;
	*ptranslated = translated;
	return STATUS_SUCCESS;
	}							// GetChildResources