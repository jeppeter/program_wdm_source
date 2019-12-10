// Declarations for mulfunc driver
// Copyright (C) 1999 by Walter Oney
// All rights reserved

#ifndef DRIVER_H
#define DRIVER_H

#define DRIVERNAME "MULFUNC"				// for use in messages
#define LDRIVERNAME L"MULFUNC"				// for use in UNICODE string constants

///////////////////////////////////////////////////////////////////////////////
// Device extension structure

enum DEVSTATE {
	STOPPED,								// device stopped
	WORKING,								// started and working
	PENDINGSTOP,							// stop pending
	PENDINGREMOVE,							// remove pending
	SURPRISEREMOVED,						// removed by surprise
	REMOVED,								// removed
	};

// Common beginning for FDO and PDO device extension structures

typedef struct _COMMON_EXTENSION {
	ULONG flags;							// flags
	} COMMON_EXTENSION, *PCOMMON_EXTENSION;

#define ISPDO			0x00000001			// this is a PDO for a child device
#define CHILDTYPEA		0x00000002			// this is the "A" child
#define CHILDTYPEB		0x00000004			// this is the "B" child
#define FDOLOCKED		0x00000008			// we locked the FDO when this PDO started
#define PDODELETED		0x00000010			// PDO was already deleted once

// Device extension for main device FDO

typedef struct _DEVICE_EXTENSION {
	ULONG flags;							// flags
	PDEVICE_OBJECT DeviceObject;			// device object this extension belongs to
	PDEVICE_OBJECT LowerDeviceObject;		// next lower driver in same stack
	PDEVICE_OBJECT Pdo;						// the PDO
	PDRIVER_OBJECT DriverObject;			// our own driver object
	IO_REMOVE_LOCK RemoveLock;				// removal control locking structure
	UNICODE_STRING devname;					// name of this device
	DEVSTATE state;							// current state of device
	DEVSTATE prevstate;						// state prior to removal query
	DEVICE_POWER_STATE devpower;			// current device power state
	SYSTEM_POWER_STATE syspower;			// current system power state
	DEVICE_CAPABILITIES devcaps;			// copy of most recent device capabilities

	// This sample device has two child devices at all times. I called them
	// A and B 

	PDEVICE_OBJECT ChildA;					// one child device
	PDEVICE_OBJECT ChildB;					// the other child device

	// This parent device suballocates its I/O resources by exporting
	// a direct-call interface to the SUBALLOC upper filter. These are
	// the resources that will be assigned to the child devices:

	LONG sarefcnt;									// reference count for SUBALLOC interfaces
	CM_PARTIAL_RESOURCE_DESCRIPTOR PortARaw;		// port resource for child A
	CM_PARTIAL_RESOURCE_DESCRIPTOR PortATranslated;
	CM_PARTIAL_RESOURCE_DESCRIPTOR PortBRaw;		// port resource for child B
	CM_PARTIAL_RESOURCE_DESCRIPTOR PortBTranslated;
	} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

// Device extension for child device PDO

typedef struct _PDO_EXTENSION {
	ULONG flags;							// flags
	PDEVICE_OBJECT DeviceObject;			// the PDO for which this is the extension
	PDEVICE_OBJECT Fdo;						// the FDO for the parent device
	} PDO_EXTENSION, *PPDO_EXTENSION;

///////////////////////////////////////////////////////////////////////////////
// Global functions

VOID RemoveDevice(IN PDEVICE_OBJECT fdo);
NTSTATUS CompleteRequest(IN PIRP Irp, IN NTSTATUS status, IN ULONG_PTR info);
NTSTATUS CompleteRequest(IN PIRP Irp, IN NTSTATUS status);
NTSTATUS ForwardAndWait(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS SendDeviceSetPower(IN PDEVICE_OBJECT fdo, IN DEVICE_POWER_STATE state, BOOLEAN wait = FALSE);

// I/O request handlers

NTSTATUS DispatchPowerFdo(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchPnpFdo(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchWmiFdo(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchPowerPdo(PDEVICE_OBJECT pdo, PIRP Irp);
NTSTATUS DispatchPnpPdo(PDEVICE_OBJECT pdo, PIRP Irp);
NTSTATUS DispatchWmiPdo(PDEVICE_OBJECT fdo, PIRP Irp);

extern BOOLEAN win98;
extern UNICODE_STRING servkey;

#endif // DRIVER_H
