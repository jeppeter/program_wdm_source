// Declarations for powtrace driver
// Copyright (C) 1999, 2000 by Walter Oney
// All rights reserved

#pragma once

#define DRIVERNAME "POWTRACE"				// for use in messages
#define LDRIVERNAME L"POWTRACE"				// for use in UNICODE string constants

///////////////////////////////////////////////////////////////////////////////
// Device extension structure

typedef struct _DEVICE_EXTENSION {
	PDEVICE_OBJECT DeviceObject;			// device object this extension belongs to
	PDEVICE_OBJECT LowerDeviceObject;		// next lower driver in same stack
	PDEVICE_OBJECT Pdo;						// the PDO
	IO_REMOVE_LOCK RemoveLock;				// removal control locking structure
	BOOLEAN CapabilitiesDumped;				// true if capabilities ever dumped for this device
	BOOLEAN filetrace;						// TRUE if want to dump messages to a file too
	} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

///////////////////////////////////////////////////////////////////////////////
// Context structure for PowerTraceWorkitem function

typedef struct _POWTRACE_CONTEXT {
	PIO_WORKITEM item;
	char msg[1];
	} POWTRACE_CONTEXT, *PPOWTRACE_CONTEXT;

///////////////////////////////////////////////////////////////////////////////
// Global functions

VOID RemoveDevice(IN PDEVICE_OBJECT fdo);
NTSTATUS CompleteRequest(IN PIRP Irp, IN NTSTATUS status, IN ULONG_PTR info);
