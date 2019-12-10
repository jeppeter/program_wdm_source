// Read/Write request processors for hidfake driver
// Copyright (C) 1999, 2000 by Walter Oney
// All rights reserved

#include "stddcls.h"
#include "driver.h"

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS StartDevice(PDEVICE_OBJECT fdo, PCM_PARTIAL_RESOURCE_LIST raw, PCM_PARTIAL_RESOURCE_LIST translated)
	{							// StartDevice
	PDEVICE_EXTENSION pdx = PDX(fdo);

	// Since HIDCLASS ignored the return code from AddDevice, this is our
	// only opportunity to abort the initialization of this driver...

	if (!NT_SUCCESS(pdx->AddDeviceStatus))
		return pdx->AddDeviceStatus;	// punt -- AddDevice failed

	// In a real device, this is where you would do the regular StartDevice
	// processing for the device. This driver has no intialization to do,
	// however.

	return STATUS_SUCCESS;
	}							// StartDevice

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID StopDevice(IN PDEVICE_OBJECT fdo, BOOLEAN oktouch /* = FALSE */)
	{							// StopDevice
	PDEVICE_EXTENSION pdx = PDX(fdo);
	}							// StopDevice
