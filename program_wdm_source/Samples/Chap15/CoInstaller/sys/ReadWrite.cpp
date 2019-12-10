// Read/Write request processors for SampleCoinstallerDummy driver
// Copyright (C) 1999, 2000 by Walter Oney
// All rights reserved

#include "stddcls.h"
#include "driver.h"

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS StartDevice(PDEVICE_OBJECT fdo, PCM_PARTIAL_RESOURCE_LIST raw, PCM_PARTIAL_RESOURCE_LIST translated)
	{							// StartDevice
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	// Fetch and print the dummy parameter ProgrammersShoeSize

	HANDLE hkey;
	NTSTATUS status = IoOpenDeviceRegistryKey(pdx->Pdo, PLUGPLAY_REGKEY_DEVICE, KEY_READ, &hkey);
	if (!NT_SUCCESS(status))
		{
		KdPrint((DRIVERNAME " - IoOpenDeviceRegistryKey failed - %X\n", status));
		return status;
		}

	UNICODE_STRING valname;
	RtlInitUnicodeString(&valname, L"ProgrammersShoeSize");

	KEY_VALUE_PARTIAL_INFORMATION value;
	ULONG junk;
	status = ZwQueryValueKey(hkey, &valname, KeyValuePartialInformation, &value, sizeof(value), &junk);
	if (NT_SUCCESS(status))
		{
		ULONG size =*(PULONG) value.Data;
		KdPrint((DRIVERNAME " - ProgrammersShoeSize is %d\n", size));
		}

	ZwClose(hkey);

	return STATUS_SUCCESS;
	}							// StartDevice

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID StopDevice(IN PDEVICE_OBJECT fdo, BOOLEAN oktouch /* = FALSE */)
	{							// StopDevice
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	}							// StopDevice
