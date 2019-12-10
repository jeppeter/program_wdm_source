// Declarations for wdmstub driver
// Copyright (C) 1999, 2000 by Walter Oney
// All rights reserved

#ifndef DRIVER_H
#define DRIVER_H

#define DRIVERNAME "WDMSTUB"				// for use in messages
#define LDRIVERNAME L"WDMSTUB"				// for use in UNICODE string constants

///////////////////////////////////////////////////////////////////////////////
// Device extension structure

typedef struct _DEVICE_EXTENSION {
	ULONG flags;							// flags common to dummy device extension
	PDEVICE_OBJECT DeviceObject;			// device object this extension belongs to
	PDEVICE_OBJECT LowerDeviceObject;		// next lower driver in same stack
	PDEVICE_OBJECT Pdo;						// the PDO
	} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _DUMMY_DEVICE_EXTENSION {
	ULONG flags;							// flags common to layered device object
	} DUMMY_DEVICE_EXTENSION, *PDUMMY_DEVICE_EXTENSION;

#define DUMMY_DEVICE_FLAG		0x00000001	// TRUE if this is the dummy device object

///////////////////////////////////////////////////////////////////////////////

typedef struct _COMPLETION_CONTEXT {
	PIO_COMPLETION_ROUTINE CompletionRoutine;
	PVOID Context;
	PDEVICE_OBJECT fdo;
	} COMPLETION_CONTEXT, *PCOMPLETION_CONTEXT;

///////////////////////////////////////////////////////////////////////////////
// This internal IOCTL is used for version checking between clones of WDMSTUB:

#define IOCTL_INTERNAL_WDMSTUB_GET_VERSION CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_NEITHER, FILE_ANY_ACCESS)

///////////////////////////////////////////////////////////////////////////////
// Global functions

VOID RemoveDevice(IN PDEVICE_OBJECT fdo);
NTSTATUS CompleteRequest(IN PIRP Irp, IN NTSTATUS status, IN ULONG_PTR info);
BOOLEAN DefineStubs();
VOID UndefineStubs();

#endif // DRIVER_H
