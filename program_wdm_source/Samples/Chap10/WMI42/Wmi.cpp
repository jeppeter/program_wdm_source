// Windows Management Instrumentation handlers for wmi42 driver
// Copyright (C) 1999 by Walter Oney
// All rights reserved

#include "stddcls.h"
#include <wmilib.h>
#include "driver.h"

#include <initguid.h>
#include <wdmguid.h>
#include "wmi42.h"

NTSTATUS QueryRegInfo(PDEVICE_OBJECT fdo, PULONG flags, PUNICODE_STRING instname, PUNICODE_STRING* regpath, PUNICODE_STRING resname, PDEVICE_OBJECT* pdo);
NTSTATUS QueryDataBlock(PDEVICE_OBJECT fdo, PIRP Irp, ULONG guidindex, ULONG instindex, ULONG instcount, PULONG instlength, ULONG bufsize, PUCHAR buffer);
NTSTATUS SetDataBlock(PDEVICE_OBJECT fdo, PIRP Irp, ULONG guidindex, ULONG instindex, ULONG bufsize, PUCHAR buffer);
NTSTATUS SetDataItem(PDEVICE_OBJECT fdo, PIRP Irp, ULONG guidindex, ULONG instindex, ULONG id, ULONG bufsize, PUCHAR buffer);
NTSTATUS ExecuteMethod(PDEVICE_OBJECT fdo, PIRP Irp, ULONG guidindex, ULONG instindex, ULONG id, ULONG cbInbuf, ULONG cbOutbuf, PUCHAR buffer);
NTSTATUS FunctionControl(PDEVICE_OBJECT fdo, PIRP Irp, ULONG guidindex, WMIENABLEDISABLECONTROL fcn, BOOLEAN enable);

WMIGUIDREGINFO guidlist[] = {
	{&Wmi42_GUID, 1, WMIREG_FLAG_INSTANCE_PDO},
	};

WMILIB_CONTEXT libinfo = {
	arraysize(guidlist),		// GuidCount
	guidlist,					// GuidList
	QueryRegInfo,
	QueryDataBlock,
	SetDataBlock,
	SetDataItem,
	ExecuteMethod,
	FunctionControl,
	};
									  
///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS DispatchWmi(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// DispatchWmi
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	NTSTATUS status;

#if DBG
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG fcn = stack->MinorFunction;

	static char* fcnname[] = {
		"IRP_MN_QUERY_ALL_DATA",
		"IRP_MN_QUERY_SINGLE_INSTANCE",
		"IRP_MN_CHANGE_SINGLE_INSTANCE",
		"IRP_MN_CHANGE_SINGLE_ITEM",
		"IRP_MN_ENABLE_EVENTS",
		"IRP_MN_DISABLE_EVENTS",
		"IRP_MN_ENABLE_COLLECTION",
		"IRP_MN_DISABLE_COLLECTION",
		"IRP_MN_REGINFO",
		"IRP_MN_EXECUTE_METHOD",
		"<reserved>",
		"IRP_MN_REGINFO_EX",
		};

	KdPrint((DRIVERNAME " - WMI Request (%s)\n", fcnname[fcn]));
#endif // DBG

	// Delegate processing to the WMILIB helper library

	SYSCTL_IRP_DISPOSITION disposition;
	status = WmiSystemControl(&libinfo, fdo, Irp, &disposition);

	switch (disposition)
		{						// finish handling IRP

	case IrpProcessed:
		break;

	case IrpNotCompleted:
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		break;

	default:
		IoSkipCurrentIrpStackLocation(Irp);
		status = IoCallDriver(pdx->LowerDeviceObject, Irp);
		break;
		}						// finish handling IRP

	return status;
	}							// DispatchWmi

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

void WmiInitialize(PDEVICE_OBJECT fdo)
	{							// WmiInitialize
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	NTSTATUS status = IoWMIRegistrationControl(fdo, WMIREG_ACTION_REGISTER);
	if (!NT_SUCCESS(status))
		KdPrint((DRIVERNAME " - IoWMIRegistrationControl failed - %X\n", status));
	pdx->TheAnswer = 42;		// as always, the answer
	}							// WmiInitialize

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

void WmiTerminate(PDEVICE_OBJECT fdo)
	{							// WmiTerminate
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	IoWMIRegistrationControl(fdo, WMIREG_ACTION_DEREGISTER);
	}							// WmiTerminate

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS QueryRegInfo(PDEVICE_OBJECT fdo, PULONG flags, PUNICODE_STRING instname, 
	PUNICODE_STRING* regpath, PUNICODE_STRING resname, PDEVICE_OBJECT* pdo)
	{							// QueryRegInfo
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	*flags = WMIREG_FLAG_INSTANCE_PDO; // use static instance names based on PDO's name
	*regpath = &servkey;
	RtlInitUnicodeString(resname, L"MofResource");
	*pdo = pdx->Pdo;
	return STATUS_SUCCESS;
	}							// QueryRegInfo

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS QueryDataBlock(PDEVICE_OBJECT fdo, PIRP Irp, ULONG guidindex, 
	ULONG instindex, ULONG instcount, PULONG instlength, ULONG bufsize, PUCHAR buffer)
	{							// QueryDataBlock
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	// WMILIB is supposed to check whether we support the GUID, so the index ought
	// never to be out of bounds. Also, we should never be asked for more than
	// just instance zero.

	if (guidindex >= arraysize(guidlist))
		return WmiCompleteRequest(fdo, Irp, STATUS_WMI_GUID_NOT_FOUND, 0, IO_NO_INCREMENT);
	if (instindex != 0 || instcount != 1)
		return WmiCompleteRequest(fdo, Irp, STATUS_WMI_INSTANCE_NOT_FOUND, 0, IO_NO_INCREMENT);

	// If no instance length array has been supplied, complete the IRP with the
	// buffer size which would be required

	if (!instlength || bufsize < Wmi42_SIZE)
		return WmiCompleteRequest(fdo, Irp, STATUS_BUFFER_TOO_SMALL, Wmi42_SIZE, IO_NO_INCREMENT);

	PWmi42 pvalue = (PWmi42) buffer;
	pvalue->TheAnswer = pdx->TheAnswer;
	instlength[0] = Wmi42_SIZE;

	return WmiCompleteRequest(fdo, Irp, STATUS_SUCCESS, Wmi42_SIZE, IO_NO_INCREMENT);
	}							// QueryDataBlock

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS SetDataBlock(PDEVICE_OBJECT fdo, PIRP Irp, ULONG guidindex, ULONG instindex, ULONG bufsize, PUCHAR buffer)
	{							// SetDataBlock
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	NTSTATUS status;
	ULONG info;

	if (guidindex >= arraysize(guidlist))
		return WmiCompleteRequest(fdo, Irp, STATUS_WMI_GUID_NOT_FOUND, 0, IO_NO_INCREMENT);
	if (instindex != 0)
		return WmiCompleteRequest(fdo, Irp, STATUS_WMI_INSTANCE_NOT_FOUND, 0, IO_NO_INCREMENT);

	if (bufsize == Wmi42_SIZE)
		{						// correct buffer length	
		pdx->TheAnswer = ((PWmi42) buffer)->TheAnswer;
		status = STATUS_SUCCESS;
		info = Wmi42_SIZE;
		}						// correct buffer length
	else
		status = STATUS_INFO_LENGTH_MISMATCH, info = 0;

	return WmiCompleteRequest(fdo, Irp, status, info, IO_NO_INCREMENT);
	}							// SetDataBlock

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS SetDataItem(PDEVICE_OBJECT fdo, PIRP Irp, ULONG guidindex, ULONG instindex, ULONG id, ULONG bufsize, PUCHAR buffer)
	{							// SetDataItem
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	NTSTATUS status;
	ULONG info;

	if (guidindex >= arraysize(guidlist))
		return WmiCompleteRequest(fdo, Irp, STATUS_WMI_GUID_NOT_FOUND, 0, IO_NO_INCREMENT);
	if (instindex != 0)
		return WmiCompleteRequest(fdo, Irp, STATUS_WMI_INSTANCE_NOT_FOUND, 0, IO_NO_INCREMENT);
	if (id != Wmi42_TheAnswer_ID)
		return WmiCompleteRequest(fdo, Irp, STATUS_WMI_ITEMID_NOT_FOUND, 0, IO_NO_INCREMENT);

	if (bufsize == Wmi42_SIZE)
		{						// correct buffer length	
		pdx->TheAnswer = ((PWmi42) buffer)->TheAnswer;
		status = STATUS_SUCCESS;
		info = Wmi42_SIZE;
		}						// correct buffer length
	else
		status = STATUS_INFO_LENGTH_MISMATCH, info = 0;

	return WmiCompleteRequest(fdo, Irp, status, info, IO_NO_INCREMENT);
	}							// SetDataItem

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS ExecuteMethod(PDEVICE_OBJECT fdo, PIRP Irp, ULONG guidindex, ULONG instindex, ULONG id, ULONG cbInbuf, ULONG cbOutbuf, PUCHAR buffer)
	{							// ExecuteMethod
	return WmiCompleteRequest(fdo, Irp, STATUS_INVALID_DEVICE_REQUEST, 0, IO_NO_INCREMENT);
	}							// ExecuteMethod

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS FunctionControl(PDEVICE_OBJECT fdo, PIRP Irp, ULONG guidindex, WMIENABLEDISABLECONTROL fcn, BOOLEAN enable)
	{							// FunctionControl
	return WmiCompleteRequest(fdo, Irp, STATUS_SUCCESS, 0, IO_NO_INCREMENT);
	}							// FunctionControl
