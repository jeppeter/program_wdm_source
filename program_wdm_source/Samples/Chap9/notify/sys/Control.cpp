// Control.cpp -- IOCTL handlers for notify driver
// Copyright (C) 1999, 2000 by Walter Oney
// All rights reserved

#include "stddcls.h"
#include "driver.h"
#include "ioctls.h"

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS DispatchControl(PDEVICE_OBJECT fdo, PIRP Irp)
	{							// DispatchControl
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	NTSTATUS status = STATUS_SUCCESS;
	ULONG info = 0;

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG cbin = stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG cbout = stack->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;

	switch (code)
		{						// process request

	case IOCTL_WAIT_NOTIFY:				// code == 0x800
		{						// IOCTL_WAIT_NOTIFY
		if (cbout < sizeof(ULONG))
			status = STATUS_INVALID_PARAMETER;
		else
			status = GenericCacheControlRequest(pdx->pgx, Irp, &pdx->NotifyIrp);
		break;
		}						// IOCTL_WAIT_NOTIFY

	case IOCTL_GENERATE_EVENT:				// code == 0x801
		{						// IOCTL_GENERATE_EVENT
		if (cbin < sizeof(ULONG))
			{					// buffer too small
			status = STATUS_INVALID_PARAMETER;
			break;
			}					// buffer too small

		PIRP nfyirp = GenericUncacheControlRequest(pdx->pgx, &pdx->NotifyIrp);
		if (nfyirp)
			{					// complete notification IRP
			*(PULONG) nfyirp->AssociatedIrp.SystemBuffer = *(PULONG) Irp->AssociatedIrp.SystemBuffer;
			CompleteRequest(nfyirp, STATUS_SUCCESS, sizeof(ULONG));
			}					// complete notification IRP
		else
			status = STATUS_UNSUCCESSFUL;

		break;
		}						// IOCTL_GENERATE_EVENT

	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;

		}						// process request

	return status == STATUS_PENDING ? status : CompleteRequest(Irp, status, info);
	}							// DispatchControl

#pragma LOCKEDCODE				// force inline functions into nonpaged code

