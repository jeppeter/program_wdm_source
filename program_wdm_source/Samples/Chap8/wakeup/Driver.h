// Declarations for wakeup driver
// Copyright (C) 1999, 2000 by Walter Oney
// All rights reserved

#pragma once

#define DRIVERNAME "WAKEUP"				// for use in messages
#define LDRIVERNAME L"WAKEUP"				// for use in UNICODE string constants
#include "generic.h"

///////////////////////////////////////////////////////////////////////////////
// Device extension structure

typedef struct _DEVICE_EXTENSION {
	PDEVICE_OBJECT DeviceObject;			// device object this extension belongs to
	PDEVICE_OBJECT LowerDeviceObject;		// next lower driver in same stack
	PDEVICE_OBJECT Pdo;						// the PDO
	IO_REMOVE_LOCK RemoveLock;				// removal control locking structure
	UNICODE_STRING devname;
	PGENERIC_EXTENSION pgx;					// device extension for GENERIC.SYS

	PIRP SuspendIrp;						// our selective suspend IRP
	LONG SuspendIrpCancelled;				// flag to control cancelation of selective suspend
	FAST_MUTEX SuspendMutex;				// used to synchronize calls to IssueSelectiveSuspendRequest

	USB_IDLE_CALLBACK_INFO cbinfo;			// callback info for selective suspend

	BOOLEAN wakeup;							// TRUE if wakeup should be enabled
	BOOLEAN autopower;						// TRUE if device should be powered down when not in use

	} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

///////////////////////////////////////////////////////////////////////////////
// Global functions

VOID ChangeAutoPower(PDEVICE_EXTENSION pdx, BOOLEAN enable);
VOID ChangeWakeupEnable(PDEVICE_EXTENSION pdx, BOOLEAN enable);
VOID RemoveDevice(IN PDEVICE_OBJECT fdo);
NTSTATUS CompleteRequest(IN PIRP Irp, IN NTSTATUS status, IN ULONG_PTR info);
DEVICE_POWER_STATE GetDevicePowerState(PDEVICE_OBJECT fdo, SYSTEM_POWER_STATE sstate, DEVICE_POWER_STATE dstate);
VOID RestoreDeviceContext(PDEVICE_OBJECT fdo, DEVICE_POWER_STATE oldstate, DEVICE_POWER_STATE newstate, PVOID context);
NTSTATUS StartDevice(PDEVICE_OBJECT fdo, PCM_PARTIAL_RESOURCE_LIST raw, PCM_PARTIAL_RESOURCE_LIST translated);
VOID StopDevice(PDEVICE_OBJECT fdo, BOOLEAN oktouch = FALSE);
VOID WmiInitialize(IN PDEVICE_OBJECT fdo);
VOID WmiTerminate(IN PDEVICE_OBJECT fdo);

// I/O request handlers

NTSTATUS DispatchPower(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchWmi(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchPnp(PDEVICE_OBJECT fdo, PIRP Irp);

extern BOOLEAN win98;
extern UNICODE_STRING servkey;
