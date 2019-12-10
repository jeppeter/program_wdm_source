// WMI.CPP -- Windows Management Instrumentation handlers for wakeup driver
// Copyright (C) 1999, 2000 by Walter Oney
// All rights reserved

#include "stddcls.h"
#include <wmilib.h>
#include "driver.h"

#include <initguid.h>
#include <wmidata.h>

NTSTATUS QueryRegInfo(PDEVICE_OBJECT fdo, PULONG flags, PUNICODE_STRING instname, PUNICODE_STRING* regpath, PUNICODE_STRING resname, PDEVICE_OBJECT* pdo);
NTSTATUS QueryDataBlock(PDEVICE_OBJECT fdo, PIRP Irp, ULONG guidindex, ULONG instindex, ULONG instcount, PULONG instlength, ULONG bufsize, PUCHAR buffer);
NTSTATUS SetDataBlock(PDEVICE_OBJECT fdo, PIRP Irp, ULONG guidindex, ULONG instindex, ULONG bufsize, PUCHAR buffer);
NTSTATUS SetDataItem(PDEVICE_OBJECT fdo, PIRP Irp, ULONG guidindex, ULONG instindex, ULONG id, ULONG bufsize, PUCHAR buffer);
NTSTATUS ExecuteMethod(PDEVICE_OBJECT fdo, PIRP Irp, ULONG guidindex, ULONG instindex, ULONG id, ULONG cbInbuf, ULONG cbOutbuf, PUCHAR buffer);
NTSTATUS FunctionControl(PDEVICE_OBJECT fdo, PIRP Irp, ULONG guidindex, WMIENABLEDISABLECONTROL fcn, BOOLEAN enable);

WMIGUIDREGINFO guidlist[] = {
	{&MSPower_DeviceWakeEnable_GUID, 1, WMIREG_FLAG_INSTANCE_PDO},
	{&MSPower_DeviceEnable_GUID, 1, WMIREG_FLAG_INSTANCE_PDO},
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

NTSTATUS DispatchWmi(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// DispatchWmi
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	NTSTATUS status = IoAcquireRemoveLock(&pdx->RemoveLock, Irp);
	if (!NT_SUCCESS(status))
		return CompleteRequest(Irp, status, 0);

	// Delegate processing to the WMILIB helper library

	SYSCTL_IRP_DISPOSITION disposition;
	status = WmiSystemControl(&libinfo, fdo, Irp, &disposition);

	switch (disposition)
		{						// finish handling IRP

	case IrpProcessed:
		break;

	case IrpNotCompleted:
		IoCompleteRequest(Irp, IO_NO_INCREMENT); // WMILIB already filled in IoStatus fields
		break;

	default:
		IoSkipCurrentIrpStackLocation(Irp);
		status = IoCallDriver(pdx->LowerDeviceObject, Irp);
		break;
		}						// finish handling IRP

	IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	return status;
	}							// DispatchWmi

///////////////////////////////////////////////////////////////////////////////

void WmiInitialize(PDEVICE_OBJECT fdo)
	{							// WmiInitialize
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	NTSTATUS status = IoWMIRegistrationControl(fdo, WMIREG_ACTION_REGISTER);
	if (!NT_SUCCESS(status))
		KdPrint((DRIVERNAME " - IoWMIRegistrationControl failed - %X\n", status));
	}							// WmiInitialize

///////////////////////////////////////////////////////////////////////////////

void WmiTerminate(PDEVICE_OBJECT fdo)
	{							// WmiTerminate
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	IoWMIRegistrationControl(fdo, WMIREG_ACTION_DEREGISTER);
	}							// WmiTerminate

///////////////////////////////////////////////////////////////////////////////

NTSTATUS QueryRegInfo(PDEVICE_OBJECT fdo, PULONG flags, PUNICODE_STRING instname, 
	PUNICODE_STRING* regpath, PUNICODE_STRING resname, PDEVICE_OBJECT* pdo)
	{							// QueryRegInfo
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	*flags = WMIREG_FLAG_INSTANCE_PDO;
	*pdo = pdx->Pdo;

	// Note that WMILIB will crash if regpath isn't set, even though
	// this driver doesn't have a private schema in a MOF resource.

	*regpath = &servkey;
	RtlInitUnicodeString(resname, L"MofResource");
	return STATUS_SUCCESS;
	}							// QueryRegInfo

///////////////////////////////////////////////////////////////////////////////

NTSTATUS QueryDataBlock(PDEVICE_OBJECT fdo, PIRP Irp, ULONG guidindex, 
	ULONG instindex, ULONG instcount, PULONG instlength, ULONG bufavail, PUCHAR buffer)
	{							// QueryDataBlock
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	ULONG bufsize = 0;
	NTSTATUS status = STATUS_SUCCESS;

	switch (guidindex)
		{						// provide indicated data

	case 0:
		{						// MSPower_DeviceWakeEnable
		bufsize = sizeof(MSPower_DeviceWakeEnable);
		if (instindex != 0 || instcount != 1)
			status = STATUS_WMI_INSTANCE_NOT_FOUND;
		else if (!instlength || bufavail < bufsize)
			status = STATUS_BUFFER_TOO_SMALL;
		else
			{					// provide data
			((PMSPower_DeviceWakeEnable) buffer)->Enable = pdx->wakeup;
			instlength[0] = bufsize;
			}					// provide data
			
		break;
		}						// MSPower_DeviceWakeEnable

	case 1:
		{						// MSPower_DeviceEnable
		bufsize = sizeof(MSPower_DeviceEnable);
		if (instindex != 0 || instcount != 1)
			status = STATUS_WMI_INSTANCE_NOT_FOUND;
		else if (!instlength || bufavail < bufsize)
			status = STATUS_BUFFER_TOO_SMALL;
		else
			{					// provide data
			((PMSPower_DeviceEnable) buffer)->Enable = pdx->autopower;
			instlength[0] = bufsize;
			}					// provide data
			
		break;
		}						// MSPower_DeviceWakeEnable

	default:
		status = STATUS_WMI_GUID_NOT_FOUND;
		}						// provide indicated data

	return WmiCompleteRequest(fdo, Irp, status, bufsize, IO_NO_INCREMENT);
	}							// QueryDataBlock

///////////////////////////////////////////////////////////////////////////////

NTSTATUS SetDataBlock(PDEVICE_OBJECT fdo, PIRP Irp, ULONG guidindex, ULONG instindex, ULONG bufavail, PUCHAR buffer)
	{							// SetDataBlock
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	NTSTATUS status = STATUS_SUCCESS;
	ULONG bufsize = 0;

	switch (guidindex)
		{						// provide indicated data

	case 0:
		{						// MSPower_DeviceWakeEnable
		bufsize = sizeof(MSPower_DeviceWakeEnable);
		if (instindex != 0)
			status = STATUS_WMI_INSTANCE_NOT_FOUND;
		else if (bufavail != bufsize)
			status = STATUS_INFO_LENGTH_MISMATCH, bufsize = 0;
		else
			ChangeWakeupEnable(pdx, ((PMSPower_DeviceWakeEnable) buffer)->Enable);
		break;
		}						// MSPower_DeviceWakeEnable

	case 1:
		{						// MSPower_DeviceEnable
		bufsize = sizeof(MSPower_DeviceEnable);
		if (instindex != 0)
			status = STATUS_WMI_INSTANCE_NOT_FOUND;
		else if (bufavail != bufsize)
			status = STATUS_INFO_LENGTH_MISMATCH, bufsize = 0;
		else
			ChangeAutoPower(pdx, ((PMSPower_DeviceEnable) buffer)->Enable);
		break;
		}						// MSPower_DeviceEnable

	default:
		status = STATUS_WMI_GUID_NOT_FOUND;
		}						// provide indicated data

	return WmiCompleteRequest(fdo, Irp, status, bufsize, IO_NO_INCREMENT);
	}							// SetDataBlock

///////////////////////////////////////////////////////////////////////////////

NTSTATUS SetDataItem(PDEVICE_OBJECT fdo, PIRP Irp, ULONG guidindex, ULONG instindex, ULONG id, ULONG bufavail, PUCHAR buffer)
	{							// SetDataItem
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	NTSTATUS status;
	ULONG bufsize = 0;

	switch (guidindex)
		{						// provide indicated data

	case 0:
		{						// MSPower_DeviceWakeEnable
		bufsize = sizeof(MSPower_DeviceWakeEnable);
		if (instindex != 0)
			status = STATUS_WMI_INSTANCE_NOT_FOUND;
		else if (id != MSPower_DeviceWakeEnable_Enable_ID)
			status = STATUS_WMI_ITEMID_NOT_FOUND;
		else if (bufavail != bufsize)
			status = STATUS_INFO_LENGTH_MISMATCH, bufsize = 0;
		else
			ChangeWakeupEnable(pdx, ((PMSPower_DeviceWakeEnable) buffer)->Enable);
		break;
		}						// MSPower_DeviceWakeEnable

	case 1:
		{						// MSPower_DeviceEnable
		bufsize = sizeof(MSPower_DeviceEnable);
		if (instindex != 0)
			status = STATUS_WMI_INSTANCE_NOT_FOUND;
		else if (id != MSPower_DeviceEnable_Enable_ID)
			status = STATUS_WMI_ITEMID_NOT_FOUND;
		else if (bufavail != bufsize)
			status = STATUS_INFO_LENGTH_MISMATCH, bufsize = 0;
		else
			ChangeAutoPower(pdx, ((PMSPower_DeviceEnable) buffer)->Enable);
		break;
		}						// MSPower_DeviceEnable

	default:
		status = STATUS_WMI_GUID_NOT_FOUND;
		}						// provide indicated data

	return WmiCompleteRequest(fdo, Irp, status, bufsize, IO_NO_INCREMENT);
	}							// SetDataItem

///////////////////////////////////////////////////////////////////////////////

NTSTATUS ExecuteMethod(PDEVICE_OBJECT fdo, PIRP Irp, ULONG guidindex, ULONG instindex, ULONG id, ULONG cbInbuf, ULONG cbOutbuf, PUCHAR buffer)
	{							// ExecuteMethod
	return WmiCompleteRequest(fdo, Irp, STATUS_INVALID_DEVICE_REQUEST, 0, IO_NO_INCREMENT);
	}							// ExecuteMethod

///////////////////////////////////////////////////////////////////////////////

NTSTATUS FunctionControl(PDEVICE_OBJECT fdo, PIRP Irp, ULONG guidindex, WMIENABLEDISABLECONTROL fcn, BOOLEAN enable)
	{							// FunctionControl
	return WmiCompleteRequest(fdo, Irp, STATUS_INVALID_DEVICE_REQUEST, 0, IO_NO_INCREMENT);
	}							// FunctionControl
