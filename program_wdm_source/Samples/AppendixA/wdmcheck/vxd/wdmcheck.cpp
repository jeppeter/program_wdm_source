// wdmcheck.c -- Implementation of WDMCHECK.VXD
// Copyright (C) 2000 by Walter Oney
// All rights reserved

#include "stdvxd.h"
#include "wdmcheck.h"
#include "ioctls.h"
#include "version.h"

VOID GetStubInfo();
VOID DeleteStubInfo();
BOOL StringOkay(char* s, DWORD len);
DWORD IsExportDefined(char* module, char* fcn);

#define IOCTL_INTERNAL_WDMSTUB_GET_VERSION CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_NEITHER, FILE_ANY_ACCESS)

///////////////////////////////////////////////////////////////////////////////
// Overrides for library new and delete operators.

void* __cdecl ::operator new(unsigned int size)
	{							// operator new
	return _HeapAllocate(size, 0);
	}							// operator new

void __cdecl ::operator delete(void* p)
	{							// operator delete
	if (p)
		_HeapFree(p, 0);
	}							// operator delete

///////////////////////////////////////////////////////////////////////////////
// Global data areas:

#pragma VxD_PAGEABLE_DATA_SEG	// data which can be paged

CStubVersion* CStubVersion::Anchor = NULL;

///////////////////////////////////////////////////////////////////////////////

WCHAR* wcscpy(WCHAR* target, WCHAR* source)
	{							// wcscpy
	WCHAR* result = target;
	WCHAR ch;
	do	{
		ch = *source++;
		*target++ = ch;
		}
	while (ch);

	return result;
	}							// wcscpy

int wcslen(WCHAR* s)
	{							// wcslen
	int len = 0;
	while (*s++)
		++len;
	return len;
	}							// wcslen

WCHAR* wcscat(WCHAR* target, WCHAR* source)
	{							// wcscat
	WCHAR* result = target;
	wcscpy(target + wcslen(target), source);
	return result;
	}							// wcscat

#pragma warning(disable:4035)

int  __declspec(naked) __cdecl _lstrcmpi(const char* left, const char* right)
	{							// _lstrcmpi
	VMMJmp(_lstrcmpi)
	}							// _lstrcmpi

Device_Location_List* VMM_GetVxDLocationList(DWORD& nvxds, DWORD& tabsize)
	{							// VMM_GetVxDLocationList
	VMMCall(VMM_GetVxDLocationList)
	_asm
		{
		jz		error
		push	eax
		mov		eax, nvxds
		mov		[eax], edx		; EDX = count of VxDs
		mov		eax, tabsize
		mov		[eax], ecx		; ECX = size of table
		pop		eax
	error:
		}
	}							// VMM_GetVxDLocationList

#pragma warning(default:4035)

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// System control message handlers
///////////////////////////////////////////////////////////////////////////////

#pragma VxD_INIT_CODE_SEG

SYSCTL BOOL __cdecl OnSysDynamicDeviceInit(void)
	{							// OnSysDynamicDeviceInit
	GetStubInfo();
	return TRUE;
	}							// OnSysDynamicDeviceInit

///////////////////////////////////////////////////////////////////////////////

#pragma VxD_PAGEABLE_CODE_SEG

SYSCTL BOOL __cdecl OnSysDynamicDeviceExit(void)
	{							// OnSysDynamicDeviceExit
	DeleteStubInfo();
	return TRUE;
	}							// OnSysDynamicDeviceExit

///////////////////////////////////////////////////////////////////////////////

#pragma VxD_PAGEABLE_CODE_SEG

SYSCTL DWORD __cdecl OnW32DeviceIoControl(PDIOCPARAMETERS p)
	{							// OnW32DeviceIoControl
	switch (p->dwIoControlCode)
		{						// process IOCTL operation

	case DIOC_OPEN:
	case DIOC_CLOSEHANDLE:
		return 0;

	case IOCTL_GET_VERSION:
		if (p->cbOutBuffer < sizeof(DWORD))
			return ERROR_INVALID_PARAMETER;
		*(PDWORD) p->lpvOutBuffer = (VERMAJOR << 16) | VERMINOR;
		if (p->lpcbBytesReturned)
			*(PDWORD) p->lpcbBytesReturned = sizeof(DWORD);
		return 0;

	case IOCTL_EXPORT_DEFINED:
		{						// IOCTL_EXPORT_DEFINE
		if (p->cbInBuffer < sizeof(EXPORT_DEFINED_PARMS))
			return ERROR_INVALID_PARAMETER;
		if (p->cbOutBuffer < sizeof(DWORD))
			return ERROR_INVALID_PARAMETER;

		PEXPORT_DEFINED_PARMS parms = (PEXPORT_DEFINED_PARMS) p->lpvInBuffer;

		if (!StringOkay(parms->modname, parms->modlen))
			return ERROR_INVALID_PARAMETER;
		if (HIWORD(parms->fcn))
			{					// function name is a string
			if (!StringOkay(parms->fcn, parms->fcnlen))
				return ERROR_INVALID_PARAMETER;
			}					// function name is a string

		*(PDWORD) p->lpvOutBuffer = IsExportDefined(parms->modname, parms->fcn);
		if (p->lpcbBytesReturned)
			*(PDWORD) p->lpcbBytesReturned = sizeof(DWORD);
		return 0;
		}						// IOCTL_EXPORT_DEFINED

	default:
		return ERROR_INVALID_FUNCTION;
		}						// process IOCTL operation
	}							// OnW32DeviceIoControl

///////////////////////////////////////////////////////////////////////////////

VOID GetStubInfo()
	{							// GetStubInfo

	// Look for a VxD version of WDMSTUB

	PVMMDDB ddb = Get_DDB(0, "WDMSTUB ");
	if (ddb)
		new CStubVersion(ddb);

	// Look for WDMSTUB.SYS and OEM clones thereof

	for (ULONG index = 0; index < 100; ++index)
		{						// for each possible driver index
		WCHAR namebuf[32];
		UNICODE_STRING devname;

		// Note: _snwprintf not exported in wdmvxd.clb

		wcscpy(namebuf, L"\\Device\\WDMSTUB");
		char indexbuf[3];
		WCHAR windexbuf[3];
		_Sprintf(indexbuf, "%d", index);
		windexbuf[0] = indexbuf[0];
		windexbuf[1] = indexbuf[1];
		windexbuf[2] = indexbuf[2];
		wcscat(namebuf, windexbuf);
		RtlInitUnicodeString(&devname, namebuf);

		PFILE_OBJECT fop;
		PDEVICE_OBJECT clone;
		NTSTATUS status = IoGetDeviceObjectPointer(&devname, FILE_READ_DATA, &fop, &clone);

		if (NT_SUCCESS(status))
			{					// found another version
			new CStubVersion(clone);
			ObDereferenceObject(fop);
			}					// found another version
		}						// for each possible driver index

	if (!CStubVersion::Anchor)
		return;					// no WDMSTUB versions loaded, so just return

	// Run through the list of all loaded drivers (which is presumably much longer
	// than the list of WDMSTUBs) to determine the addresses of the sections in
	// these drivers. We have to run VXDLDR's and VMM's lists of drivers to get all
	// of this info

	for (DeviceInfo* dip = VXDLDR_GetDeviceList(); dip; dip = dip->DI_Next)
		{						// for each device
		char drvname[256];
		_Sprintf(drvname, "\\Driver\\%s.sys", dip->DI_ModuleName);
		for (CStubVersion* stub = CStubVersion::Anchor; stub; stub = stub->m_next)
			{					// for each WDMSTUB
			if (stub->m_ddb == dip->DI_DDB
				|| stub->m_name &&_lstrcmpi(stub->m_name, drvname) == 0)
				stub->GetSectionInfo(dip);
			}					// for each WDMSTUB
		}						// for each device

	DWORD nvxds;
	DWORD size;

	Device_Location_List* loclist = VMM_GetVxDLocationList(nvxds, size);
	for (DWORD ivxd = 0; ivxd < nvxds; ++ivxd)
		{						// for each VxD
		for (CStubVersion* stub = CStubVersion::Anchor; stub; stub = stub->m_next)
			{					// for each WDMSTUB
			if (stub->m_ddb == (PVMMDDB) loclist->DLL_DDB)
				stub->GetSectionInfo(loclist);
			}					// for each WDMSTUB
		loclist = (Device_Location_List*) (loclist->DLL_ObjLocation + loclist->DLL_NumObjects);
		}						// for each VxD
	}							// GetStubInfo

///////////////////////////////////////////////////////////////////////////////

VOID DeleteStubInfo()
	{							// DeleteStubInfo
	while (CStubVersion::Anchor)
		delete CStubVersion::Anchor;
	}							// DeleteStubInfo

///////////////////////////////////////////////////////////////////////////////
// Verify that a string pointer is valid and that null terminator is within
// the specified length

#pragma VxD_PAGEABLE_CODE_SEG

BOOL StringOkay(char* s, DWORD len)
	{							// StringOkay
	if (!_Assert_Range(s, len, 0, 0, 0))
		return FALSE;			// pointer is invalid in some way

	while (len--)
		if (!*s++)
			return TRUE;

	return FALSE;				// exhausted length before finding NUL
	}							// StringOkay

///////////////////////////////////////////////////////////////////////////////

#pragma VxD_PAGEABLE_CODE_SEG

DWORD IsExportDefined(char* module, char* fcn)
	{							// IsExportDefined
	DWORD addr = (DWORD) _PELDR_GetProcAddress((HPEMODULE) module, fcn, NULL);
	if (!addr)
		return WDMCHECK_NOT_DEFINED;

	// This symbol is defined. See if the definition is inside WDMSTUB

	for (CStubVersion* stub = CStubVersion::Anchor; stub; stub = stub->m_next)
		{						// for each stub
		if (!stub->m_sections)
			continue;			// we never found this guy's sections
		for (ULONG i = 0; i < stub->m_nsections; ++i)
			if (addr >= stub->m_sections[i].startaddr && addr < stub->m_sections[i].endaddr)
				return WDMCHECK_STUB_DEFINED;
		}						// for each stub

	// The symbol must be exported by the regular kernel

	return 0;
	}							// IsExportDefined

///////////////////////////////////////////////////////////////////////////////

CStubVersion::CStubVersion(PVMMDDB ddb)
	{							// CStubVersion::CStubVersion
	Initialize();
	m_ddb = ddb;
	}							// CStubVersion::CStubVersion

CStubVersion::CStubVersion(PDEVICE_OBJECT DeviceObject)
	{							// CStubVersion::CStubVersion
	Initialize();
	PDRIVER_OBJECT DriverObject = DeviceObject->DriverObject;
	ANSI_STRING as = {0};
	RtlUnicodeStringToAnsiString(&as, &DriverObject->DriverName, TRUE);
	m_name = as.Buffer;
	}							// CStubVersion::CStubVersion

CStubVersion::~CStubVersion()
	{							// CStubVersion::~CStubVersion
	if (m_next)
		m_next->m_prev = m_prev;
	if (m_prev)
		m_prev->m_next = m_next;
	else
		Anchor = m_next;
		
	if (m_name)
		_HeapFree(m_name, 0);
	if (m_nsections)
		delete [] m_sections;
	}							// CStubVersion::~CStubVersion

void CStubVersion::GetSectionInfo(DeviceInfo* dip)
	{							// CStubVersion::GetSectionInfo
	m_nsections = dip->DI_ObjCount;
	m_sections = new SECTION_INFO[m_nsections];
	for (ULONG i = 0; i < m_nsections; ++i)
		{						// for each section
		m_sections[i].startaddr = dip->DI_ObjInfo[i].OI_LinearAddress;
		m_sections[i].endaddr = dip->DI_ObjInfo[i].OI_LinearAddress + dip->DI_ObjInfo[i].OI_Size;
		}						// for each section
	}							// CStubVersion::GetSectionInfo

void CStubVersion::GetSectionInfo(Device_Location_List* dll)
	{							// CStubVersion::GetSectionInfo
	m_nsections = dll->DLL_NumObjects;
	m_sections = new SECTION_INFO[m_nsections];
	for (ULONG i = 0; i < m_nsections; ++i)
		{						// for each section
		m_sections[i].startaddr = dll->DLL_ObjLocation[i].OL_LinearAddr;
		m_sections[i].endaddr = dll->DLL_ObjLocation[i].OL_LinearAddr + dll->DLL_ObjLocation[i].OL_Size;
		}						// for each section
	}							// CStubVersion::GetSectionInfo

void CStubVersion::Initialize()
	{							// CStubVersion::Initialize
	m_next = Anchor;
	m_prev = NULL;
	if (Anchor)
		Anchor->m_prev = this;
	Anchor = this;

	m_ddb = NULL;
	m_name = NULL;
	m_nsections = 0;
	m_sections = NULL;
	}							// CStubVersion::Initialize