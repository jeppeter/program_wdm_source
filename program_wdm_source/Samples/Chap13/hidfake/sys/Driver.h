// Declarations for hidfake driver
// Copyright (C) 1999, 2000 by Walter Oney
// All rights reserved

#pragma once

#define DRIVERNAME "HIDFAKE"				// for use in messages
#define LDRIVERNAME L"HIDFAKE"				// for use in UNICODE string constants
#include "generic.h"

///////////////////////////////////////////////////////////////////////////////
// Device extension structure

typedef struct _DEVICE_EXTENSION {
	PGENERIC_EXTENSION pgx;					// device extension for GENERIC.SYS
	IO_REMOVE_LOCK RemoveLock;				// for guarding LDO from early removal
	NTSTATUS AddDeviceStatus;				// status returned from AddDevice
	DEVICE_POWER_STATE devpower;			// current power state of device

	BOOLEAN ButtonDown;						// true if button should be considered down
	} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

///////////////////////////////////////////////////////////////////////////////
// Accessing macros for fields kept in HIDCLASS's device extension, which is
// the "real" one you get to from the FDO

#define PDX(fdo) ((PDEVICE_EXTENSION) ((PHID_DEVICE_EXTENSION) ((fdo)->DeviceExtension))->MiniDeviceExtension)
#define PDO(fdo) (((PHID_DEVICE_EXTENSION) ((fdo)->DeviceExtension))->PhysicalDeviceObject)
#define LDO(fdo) (((PHID_DEVICE_EXTENSION) ((fdo)->DeviceExtension))->NextDeviceObject)

///////////////////////////////////////////////////////////////////////////////
// Layout of button report returned by the driver

typedef struct _BUTTON_REPORT {
	UCHAR id;								// report id
	UCHAR Buttons;							// which buttons are down
	} BUTTON_REPORT, *PBUTTON_REPORT;

///////////////////////////////////////////////////////////////////////////////
// Global functions

VOID RemoveDevice(IN PDEVICE_OBJECT fdo);
NTSTATUS CompleteRequest(IN PIRP Irp, IN NTSTATUS status, IN ULONG_PTR info);
NTSTATUS StartDevice(PDEVICE_OBJECT fdo, PCM_PARTIAL_RESOURCE_LIST raw, PCM_PARTIAL_RESOURCE_LIST translated);
VOID StopDevice(PDEVICE_OBJECT fdo, BOOLEAN oktouch = FALSE);

// HIDCLASS callbacks (disguised as IRP dispatch routines)

NTSTATUS DispatchInternalControl(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchPower(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchWmi(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchPnp(PDEVICE_OBJECT fdo, PIRP Irp);

extern BOOLEAN win98;
