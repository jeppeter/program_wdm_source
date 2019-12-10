// Main program for whichos driver
// Copyright (C) 2001 by Walter Oney
// All rights reserved

#include "stddcls.h"
#include "driver.h"

enum OSVERSION {
	Win98Gold,				// original release of Win98
	Win98SE,				// Win98 Second Edition
	WinME,					// Windows Millenium
	Win2K,					// Windows 2000
	WinXp,					// Windows Xp
	WinNet,					// Windows .NET
	};

enum OSVERSION WhichOs(PDRIVER_OBJECT DriverObject);

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
	{							// DriverEntry
	enum OSVERSION whichos = WhichOs(DriverObject);
	static char* osnames[] = {
		"Win98Gold",
		"Win98SE",
		"WinME",
		"Win2K",
		"WinXp",
		"WinNet",
		};
	DbgPrint(DRIVERNAME " - Running under %s\n", osnames[whichos]);
	return STATUS_UNSUCCESSFUL;
	}							// DriverEntry

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

enum OSVERSION WhichOs(PDRIVER_OBJECT DriverObject)
	{							// WhichOs

	// Determine which WDM version this OS supports. This tells us most of what we want to know.

	if (IoIsWdmVersionAvailable(1, 0x30))
		return WinNet;

	else if (IoIsWdmVersionAvailable(1, 0x20))
		return WinXp;

	else if (IoIsWdmVersionAvailable(1, 0x10))
		return Win2K;

	else if (IoIsWdmVersionAvailable(1, 0x05))
		return WinME;

	// To decide between Win98SE and Win98Gold, check the ServiceKeyName field of the
	// driver object extension

	else if (DriverObject->DriverExtension->ServiceKeyName.Length)
		return Win98SE;

	else
		return Win98Gold;
	}							// WhichOs