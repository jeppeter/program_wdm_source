// SUBALLOC.H - Resource suballocation interface for non-standard multifunction devices
// Copyright (C) 1999 by Walter Oney
// All rights reserved

#ifndef SUBALLOC_H
#define SUBALLOC_H

// {0AA04540-6FD1-11d3-81B5-00C04FA330A6}
DEFINE_GUID(GUID_RESOURCE_SUBALLOCATE_STANDARD, 0xaa04540, 0x6fd1, 0x11d3, 0x81, 0xb5, 0x0, 0xc0, 0x4f, 0xa3, 0x30, 0xa6);

typedef NTSTATUS (*PGETRESOURCES)(PVOID context, PDEVICE_OBJECT pdo, PCM_RESOURCE_LIST* raw, PCM_RESOURCE_LIST* translated);

struct _RESOURCE_SUBALLOCATE_STANDARD : public INTERFACE {
	PGETRESOURCES GetResources;	// get resources for given child PDO
	};
typedef struct _RESOURCE_SUBALLOCATE_STANDARD RESOURCE_SUBALLOCATE_STANDARD, *PRESOURCE_SUBALLOCATE_STANDARD;

#define RESOURCE_SUBALLOCATE_STANDARD_VERSION 1		// current version of interface structure

#endif