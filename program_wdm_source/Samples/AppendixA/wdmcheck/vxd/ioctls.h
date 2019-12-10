// IOCTLS.H -- IOCTL operations used in WDMCHECK 
// Copyright (C) 2000 by Walter Oney
// All rights reserved

#ifndef IOCTLS_WDMCHECK_H
#define IOCTLS_WDMCHECK_H

#define IOCTL_GET_VERSION		1	// get version #
#define IOCTL_EXPORT_DEFINED	2	// is specified symbol exported by kernel

typedef struct _EXPORT_DEFINED_PARMS {
	char* modname;				// name of module
	DWORD modlen;				// length of module name
	char* fcn;					// function name or ordinal
	DWORD fcnlen;				// length of function name, or zero if given an ordinal
	} EXPORT_DEFINED_PARMS, *PEXPORT_DEFINED_PARMS;

// Flags in DWORD return value from IOCTL_EXPORT_DEFINED

#define WDMCHECK_NOT_DEFINED		1		// symbol is not defined
#define WDMCHECK_STUB_DEFINED		2		// symbol is defined in WDMSTUB

#endif