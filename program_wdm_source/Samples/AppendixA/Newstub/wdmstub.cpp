// WDMSTUB.CPP -- Stubs for missing Win2K WDM service functions
// Copyright (C) 1998 by Walter Oney
// All rights reserved

#include "stddcls.h"
#include "driver.h"
#include "NonStubs.h"

#define IFSMGR_SERVICES_ONLY
#include "ifsmgr.h"

CCHAR cKeNumberProcessors = 1;
BOOLEAN bKdDebuggerEnabled = 0;

BOOLEAN PrecisionErrata = FALSE;
DWORD features = 0;

// Pentium feature flags returned in EDX from CPUID with EAX == 1

#define FEATURE_FPU				0x00000001		// FPU on chip
#define FEATURE_RDTSC			0x00000010		// RDTSC instruction supported
#define FEATURE_MACHINE_CHECK	0x00000080		// machine check exception can occur
#define FEATURE_CMPXCHG8B		0x00000100		// CMPXCHG8B instruction supported
#define FEATURE_MMX				0x00800000		// MMX instructions supported
#define FEATURE_XMMI			0x02000000

#define BCS_WANSI	0					// ifs.h generates too many compile errors if we include it

extern "C" NTSYSAPI NTSTATUS NTAPI ObReferenceObjectByName(PUNICODE_STRING name, ULONG unknown1,
    ULONG unknown2, ULONG unknown3, POBJECT_TYPE type,
    KPROCESSOR_MODE mode, ULONG unknown4, PVOID *object);
extern "C" POBJECT_TYPE* IoDriverObjectType;

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

// Names of functions stubbed by this VxD. This isn't the array we actually pass
// to PELDR -- it's the array we use when we construct a list of functions
// that aren't already implemented by NTKERN. The names must be in resident storage
// because we don't copy them when we build the export table.

static char* names[] = {
	"PoRegisterSystemState",
	"PoSetSystemState",
	"PoUnregisterSystemState",
	"IoReportTargetDeviceChangeAsynchronous",
	"ExSystemTimeToLocalTime",
	"ExLocalTimeToSystemTime",
	"IoCreateNotificationEvent",
	"IoCreateSynchronizationEvent",
	"IoAllocateWorkItem",
	"IoFreeWorkItem",
	"IoQueueWorkItem",
	"PsGetVersion",
	"RtlUshortByteSwap",
	"RtlUlongByteSwap",
	"RtlUlonglongByteSwap",
	"RtlInt64ToUnicodeString",
	"KeSetTargetProcessorDpc",
	"KeNumberProcessors",
	"KdDebuggerEnabled",
	"IoReuseIrp",
	"IoRaiseInformationalHardError",
	"HalTranslateBusAddress",
	"ExIsProcessorFeaturePresent",
	"MmGetSystemRoutineAddress",
	"ZwQueryInformationFile",
	"ZwSetInformationFile",
	"ZwQueryDefaultLocale",
	"ExFreePoolWithTag",
	"ZwLoadDriver",
	"ZwUnloadDriver",
	"SeSinglePrivilegeCheck",
	"IoSetCompletionRoutineEx",
	};

// As of SP-9, we're using C11 with the Xp options. String constants therefore end
// up in the referencing code segment, but we need to have *this* constant persist:

static char* ntoskrnl = "ntoskrnl.exe";

static char* remlocknames[] = {
	"IoAcquireRemoveLockEx",
	"IoReleaseRemoveLockEx",
	"IoReleaseRemoveLockAndWaitEx",
	"IoInitializeRemoveLockEx",
	};

// The Win98 implementation of KeEnterCriticalRegion incorrectly raises the IRQL to
// APC_LEVEL, so we'll stub it no matter what

static char* critnames[] = {
	"KeEnterCriticalRegion",
	"KeLeaveCriticalRegion",
	};

// Macros added in Xp DDK:

#undef RtlUshortByteSwap
#undef RtlUlongByteSwap
#undef RtlUlonglongByteSwap

USHORT
FASTCALL
RtlUshortByteSwap(
    IN USHORT Source
    );

ULONG
FASTCALL
RtlUlongByteSwap(
    IN ULONG Source
    );

ULONGLONG
FASTCALL
RtlUlonglongByteSwap(
    IN ULONGLONG Source
    );

NTSTATUS ZwLoadDriver(PUNICODE_STRING);
NTSTATUS ZwUnloadDriver(PUNICODE_STRING);

#pragma warning(disable:4273)	// inconsistent dll linkage

// Addresses of stub functions (must line up with names array)

NTSTATUS ZwQueryDefaultLocale(BOOLEAN thread, LCID* plocale);

static PFN addresses[] = {
	(PFN) PoRegisterSystemState,
	(PFN) PoSetSystemState,
	(PFN) PoUnregisterSystemState,
	(PFN) IoReportTargetDeviceChangeAsynchronous,
	(PFN) ExSystemTimeToLocalTime,
	(PFN) ExLocalTimeToSystemTime,
	(PFN) IoCreateNotificationEvent,
	(PFN) IoCreateSynchronizationEvent,
	(PFN) IoAllocateWorkItem,
	(PFN) IoFreeWorkItem,
	(PFN) IoQueueWorkItem,
	(PFN) PsGetVersion,
	(PFN) RtlUshortByteSwap,
	(PFN) RtlUlongByteSwap,
	(PFN) RtlUlonglongByteSwap,
	(PFN) RtlInt64ToUnicodeString,
	(PFN) KeSetTargetProcessorDpc,
	(PFN) &cKeNumberProcessors,
	(PFN) &bKdDebuggerEnabled,
	(PFN) IoReuseIrp,
	(PFN) IoRaiseInformationalHardError,
	(PFN) HalTranslateBusAddress,
	(PFN) ExIsProcessorFeaturePresent,
	(PFN) MmGetSystemRoutineAddress,
	(PFN) ZwQueryInformationFile,
	(PFN) ZwSetInformationFile,
	(PFN) ZwQueryDefaultLocale,
	(PFN) ExFreePoolWithTag,
	(PFN) ZwLoadDriver,
	(PFN) ZwUnloadDriver,
	(PFN) SeSinglePrivilegeCheck,
	(PFN) IoSetCompletionRoutineEx,
	};

static PFN remlockaddresses[] = {
	(PFN) IoAcquireRemoveLockEx,
	(PFN) IoReleaseRemoveLockEx,
	(PFN) IoReleaseRemoveLockAndWaitEx,
	(PFN) IoInitializeRemoveLockEx,
	};

static PFN critaddresses[] = {
	(PFN) KeEnterCriticalRegion,
	(PFN) KeLeaveCriticalRegion,
	};

__int64 GetZoneBias();
DWORD buildnum;

HPEEXPORTTABLE hExportTable = 0;
char** stubnames;				// names of functions we stubbed
PFN* stubaddresses;				// addresses of the stub functions
WORD* ordinals;					// ordinal table to correlate the other two tables

LCID locale;					// default locale
LCID GetSystemLocale();
DWORD atox(char* s);

NTSTATUS CompletionRoutineWrapper(PDEVICE_OBJECT fdo, PIRP Irp, PCOMPLETION_CONTEXT ctx);

///////////////////////////////////////////////////////////////////////////////

#pragma warning(disable:4035)

UINT __declspec(naked) __cdecl UniToBCS(PBYTE pBCSPath, WCHAR* pUniPath, UINT length, UINT maxLength, UINT charSet)
	{
	VxDJmp(UniToBCS)
	}

#pragma warning(default:4035)

///////////////////////////////////////////////////////////////////////////////

#pragma INITCODE

#pragma warning(disable:4035)

BOOLEAN Test_Debug_Installed()
	{
	VMMCall(Test_Debug_Installed)
	_asm setnz al
	}

#pragma warning(default:4035)

extern "C" ULONG _fltused = 0;

// Flags in CR0:

#define CR0_PG  0x80000000          // paging
#define CR0_ET  0x00000010          // extension type (80387)
#define CR0_TS  0x00000008          // task switched
#define CR0_EM  0x00000004          // emulate math coprocessor
#define CR0_MP  0x00000002          // math present
#define CR0_PE  0x00000001          // protection enable
#define CR0_CD  0x40000000          // cache disable
#define CR0_NW  0x20000000          // not write-through
#define CR0_AM  0x00040000          // alignment mask
#define CR0_WP  0x00010000          // write protect
#define CR0_NE  0x00000020          // numeric error

BOOL CheckPrecisionErrata()
	{							// CheckPrecisionErrata
	static double Dividend = 4195835.0;
	static double Divisor = 3145727.0;
	BOOL result = FALSE;

	_asm
		{
		pushfd					; save flags
		cli
		mov eax, cr0			; make sure flags are the way we need them
		mov	ecx, eax
		and eax, not (CR0_TS or CR0_MP or CR0_EM)
		mov cr0, eax

		fninit					; initialize coprocessor
		fld Dividend			; perform division to show up precision errata
		fdiv Divisor
		fmul Divisor
		fcomp Dividend
		fstsw ax				; capture floating point status
		sahf

		jc  noproblem
		jz  noproblem
		mov result, 1
	
	noproblem:
		mov cr0, ecx			; restore CR0
		popfd					; restore flags (including enable state)
		}

	return result;
	}							// CheckPrecisionErrata

BOOLEAN DefineStubs()
	{							// DefineStubs

	// Construct the name, ordinal, and address tables for functions we're going
	// to stub. In various releases of Windows 98 and Millenium, some or all of
	// these functions will be supported by the operating system, and we don't
	// want to preempt the standard implementation.

	stubnames = (char**) _HeapAllocate(sizeof(names) + sizeof(remlocknames), HEAPZEROINIT); // largest we might ever need
	if (!stubnames)
		return FALSE;

	stubaddresses = (PFN*) _HeapAllocate(sizeof(addresses) + sizeof(remlockaddresses) + sizeof(critaddresses), HEAPZEROINIT);
	if (!stubaddresses)
		{
		_HeapFree(stubnames, 0);
		return FALSE;
		}

	ordinals = (WORD*) _HeapAllocate((arraysize(names) + arraysize(remlocknames) + arraysize(critnames)) * sizeof(WORD), HEAPZEROINIT);
	if (!ordinals)
		{
		_HeapFree(stubaddresses, 0);
		_HeapFree(stubnames, 0);
		return FALSE;
		}

	int i, istub;
	for (i = 0, istub = 0; i < arraysize(names); ++i)
		{						// for each possible stub
		if (_PELDR_GetProcAddress((HPEMODULE) ntoskrnl, names[i], NULL) == 0)
			{					// stub this function
			KdPrint((DRIVERNAME " - Stubbing %s\n", names[i]));
			stubnames[istub] = names[i];
			ordinals[istub] = (WORD) istub;
			stubaddresses[istub] = addresses[i];
			++istub;
			}					// stub this function
		else
			KdPrint((DRIVERNAME " - %s already implemented - not stubbing it\n", names[i]));
		}						// for each possible stub

	// We have a special problem with the remove lock functions, in that Win98SE leaves out
	// just IoRemoveLockAndWaitEx. We'll stub all of them if we find this function missing.

	if (_PELDR_GetProcAddress((HPEMODULE) ntoskrnl, "IoReleaseRemoveLockAndWaitEx", NULL) == 0)
		{						// stub remove lock functions
		KdPrint((DRIVERNAME " - Stubbing all remove lock functions\n"));
		for (i = 0; i < arraysize(remlocknames); ++i)
			{					// for each remove lock function
			stubnames[istub] = remlocknames[i];
			ordinals[istub] = (WORD) istub;
			stubaddresses[istub] = remlockaddresses[i];
			++istub;
			}					// for each remove lock function
		}						// stub remove lock functions
	else
		KdPrint((DRIVERNAME " - Not stubbing remove lock functions\n"));

	// Always replace the critical region functions to avoid having the IRQL raised

	for (i = 0; i < arraysize(critnames); ++i)
		{						// for each critical region function
		KdPrint((DRIVERNAME " - Stubbing %s\n", critnames[i]));
		stubnames[istub] = critnames[i];
		ordinals[istub] = (WORD) istub;
		stubaddresses[istub] = critaddresses[i];
		++istub;
		}						// for each critical region function

	// Create an export table to provide for these functions

	if (_PELDR_AddExportTable(&hExportTable, ntoskrnl, istub,
		istub, 0, (PVOID*) stubnames, ordinals, stubaddresses, NULL) != 0)
		{						// can't define exports
		KdPrint((DRIVERNAME " -- Unable to define export table\n"));
		_HeapFree(ordinals, 0);
		_HeapFree(stubaddresses, 0);
		_HeapFree(stubnames, 0);
		return FALSE;
		}						// can't define exports

	// See if debugger running

	bKdDebuggerEnabled = Test_Debug_Installed();

	// Get build number from HKLM\Software\Microsoft\Windows\CurrentVersion[VersionNumber]

	VMMHKEY hkey;
	if (_RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_SETUP, &hkey) == 0)
		{						// get value
		char value[64];
		DWORD size = sizeof(value);
		if (_RegQueryValueEx(hkey, "VersionNumber", NULL, NULL, (PBYTE) value, &size) == 0)
			{					// parse value
			char* p = value;
			atoi(p);	// major version number
			if (*p == '.')
				++p;
			atoi(p);	// minor version number
			if (*p == '.')
				++p;
			buildnum = atoi(p);	// build number
			}					// parse value
		_RegCloseKey(hkey);
		}						// get value

	// Determine CPU feature flags

	DWORD machineinfo;
	#undef Get_Machine_Info
	VMMCall(Get_Machine_Info)
	_asm mov machineinfo, ebx	; capture machine type flags
	if (machineinfo & GMIF_CPUID)
		{						// cpuid supported
		_asm mov eax, 1			; EAX = 1 => request for feature flags
		_asm cpuid
		_asm mov features, edx	; save feature flags
		}						// cpuid supported

	if (features & FEATURE_FPU)
		PrecisionErrata = (BOOLEAN) CheckPrecisionErrata();

	// Determine the default locale for ZwQueryDefaultLocale

	locale = GetSystemLocale();

	return TRUE;
	}							// DefineStubs

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID UndefineStubs()
	{							// UndefineStubs
	_PELDR_RemoveExportTable(hExportTable, NULL);
	_HeapFree(ordinals, 0);
	_HeapFree(stubaddresses, 0);
	_HeapFree(stubnames, 0);
	}							// UndefineStubs

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

PVOID PoRegisterSystemState(PVOID hstate, ULONG flags)
	{							// PoRegisterSystemState
	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
	return NULL;
	}							// PoRegisterSystemState

VOID PoSetSystemState(ULONG flags)
	{							// PoSetSystemState
	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
	}							// PoSetSystemState

VOID PoUnregisterSystemState(PVOID hstate)
	{							// PoUnregisterSystemState
	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
	}							// PoUnregisterSystemState

NTSTATUS IoReportTargetDeviceChangeAsynchronous(PDEVICE_OBJECT pdo, PVOID NfyStruct, PDEVICE_CHANGE_COMPLETE_CALLBACK Callback, PVOID Context)
	{							// IoReportTargetDeviceChangeAsynchronous
	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
	return STATUS_NOT_IMPLEMENTED;
	}							// IoReportTargetDeviceChangeAsynchronous

VOID ExSystemTimeToLocalTime(PLARGE_INTEGER systime, PLARGE_INTEGER localtime)
	{							// ExSystemTimeToLocalTime
	localtime->QuadPart = systime->QuadPart - GetZoneBias();
	}							// ExSystemTimeToLocalTime

VOID ExLocalTimeToSystemTime(PLARGE_INTEGER localtime, PLARGE_INTEGER systime)
	{							// ExLocalTimeToSystemTime
	systime->QuadPart = localtime->QuadPart + GetZoneBias();
	}							// ExLocalTimeToSystemTime

PKEVENT IoCreateNotificationEvent(PUNICODE_STRING EventName, PHANDLE EventHandle)
	{							// IoCreateNotificationEvent
	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
	return NULL;
	}							// IoCreateNotificationEvent

PKEVENT IoCreateSynchronizationEvent(PUNICODE_STRING EventName, PHANDLE EventHandle)
	{							// IoCreateSynchronizationEvent
	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
	return NULL;
	}							// IoCreateSynchronizationEvent

PIO_WORKITEM IoAllocateWorkItem(PDEVICE_OBJECT DeviceObject)
	{							// IoAllocateWorkItem
	return AllocateWorkItem(DeviceObject);
	}							// IoAllocateWorkItem

VOID IoFreeWorkItem(PIO_WORKITEM item)
	{							// IoFreeWorkItem
	FreeWorkItem(item);
	}							// IoFreeWorkItem

VOID IoQueueWorkItem(PIO_WORKITEM item, PIO_WORKITEM_ROUTINE Routine, WORK_QUEUE_TYPE QueueType, PVOID Context)
	{							// IoQueueWorkItem
	QueueWorkItem(item, Routine, QueueType, Context);
	}							// IoQueueWorkItem

BOOLEAN PsGetVersion(PULONG MajorVersion, PULONG MinorVersion, PULONG BuildNumber, PUNICODE_STRING spnum)
	{							// PsGetVersion
	#undef Get_VMM_Version
	USHORT version;
	ULONG dbgver;
	VMMCall(Get_VMM_Version);
	_asm mov version, ax
	_asm mov dbgver, ecx

	if (MajorVersion)
		*MajorVersion = version >> 8;
	if (MinorVersion)
		*MinorVersion = version & 255;
	if (BuildNumber)
		*BuildNumber = buildnum;
	if (spnum)
		spnum->Length = 0;

	return dbgver != 0;
	}							// PsGetVersion

NTSTATUS IoAcquireRemoveLockEx(PIO_REMOVE_LOCK p, PVOID tag, PCSTR file, ULONG line, ULONG size)
	{							// IoAcquireRemoveLockEx
	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
	return AcquireRemoveLock(p, tag, file, line, size);
	}							// IoAcquireRemoveLockEx

VOID IoReleaseRemoveLockEx(PIO_REMOVE_LOCK p, PVOID tag, ULONG size)
	{							// IoReleaseRemoveLockEx
	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
	ReleaseRemoveLock(p, tag, size);
	}							// IoReleaseRemoveLockEx

VOID IoReleaseRemoveLockAndWaitEx(PIO_REMOVE_LOCK p, PVOID tag, ULONG size)
	{							// IoReleaseRemoveLockAndWaitEx
	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
	ReleaseRemoveLockAndWait(p, tag, size);
	}							// IoReleaseRemoveLockAndWaitEx

VOID IoInitializeRemoveLockEx(PIO_REMOVE_LOCK p, ULONG tag, ULONG maxminutes, ULONG hwm, ULONG size)
	{							// IoInitializeRemoveLockEx
	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
	InitializeRemoveLock(p, tag, maxminutes, hwm, size);
	}							// IoInitializeRemoveLockEx

#pragma warning(disable:4035)

USHORT FASTCALL RtlUshortByteSwap(USHORT source)
	{							// RtlUshortByteSwap
	_asm movzx eax, cx
	_asm xchg ah, al
	}							// RtlUshortByteSwap

ULONG FASTCALL RtlUlongByteSwap(ULONG source)
	{							// RtlUlongByteSwap
	_asm mov eax, ecx
	_asm bswap eax
	}							// RtlUlongByteSwap

ULONGLONG FASTCALL RtlUlonglongByteSwap(ULONGLONG source)
	{							// RtlUlonglongByteSwap
	_asm mov eax, ecx
	_asm xchg eax, edx
	_asm bswap eax
	_asm bswap edx
	}							// RtlUlonglongByteSwap

#pragma warning(default:4035)

NTSTATUS RtlInt64ToUnicodeString(ULONGLONG value, ULONG base, PUNICODE_STRING string)
	{							// RtlInt64ToUnicodeString
	return STATUS_NOT_IMPLEMENTED;
	}							// RtlInt64ToUnicodeString

VOID KeEnterCriticalRegion()
	{							// KeEnterCriticalRegion
	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
	}							// KeEnterCriticalRegion

VOID KeLeaveCriticalRegion()
	{							// KeLeaveCriticalRegion
	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
	}							// KeLeaveCriticalRegion

VOID KeSetTargetProcessorDpc(PRKDPC dpc, CCHAR number)
	{							// KeSetTargetProcessorDpc
	}							// KeSetTargetProcessorDpc

VOID KeSetImportanceDpc(PRKDPC dpc, KDPC_IMPORTANCE importance)
	{							// KeSetImportanceDpc
	}							// KeSetImportanceDpc

VOID IoReuseIrp(PIRP Irp, NTSTATUS status)
	{							// IoReuseIrp
	ReuseIrp(Irp, status);
	}							// IoReuseIrp

void __cdecl MessageCallback(char* msg);

BOOLEAN IoRaiseInformationalHardError(NTSTATUS status, PUNICODE_STRING string, PKTHREAD thread)
	{							// IoRaiseInformationalHardError
	char* msg = (char*) _HeapAllocate(1024, 0);
	if (!msg)
		return FALSE;
	if (string)
		{						// convert string data
		UINT unilen = string->Length;
		UINT ansilen = unilen / 2;
		char* ansistring = (char*) _HeapAllocate(ansilen + 1, 0);
		if (!ansistring)
			{
			_HeapFree(msg, 0);
			return FALSE;
			}

		#define BCS_OEM 1
		ansistring[UniToBCS((PBYTE) ansistring, string->Buffer, unilen, ansilen, BCS_OEM)] = 0;
		_Sprintf(msg, "An operation failed with status %8.8lX. The operation relates to '%s'", status, ansistring);
		_HeapFree(ansistring, 0);
		}						// convert string data
	else
		_Sprintf(msg, "An operation failed with status %8.8lX", status);

	BOOL okay;
	okay = _SHELL_CallAtAppyTime((APPY_CALLBACK) MessageCallback, (DWORD) msg, 0) != 0;
	if (!okay)
		{						// can't queue regular message box
		SHELL_SYSMODAL_Message(Get_Sys_VM_Handle(), MB_OK | MB_ICONHAND | MB_SYSTEMMODAL, msg, "Windows - Operation Failed");
		_HeapFree(msg, 0);
		okay = TRUE;
		}						// can't queue regular message box

	return (BOOLEAN) okay;
	}							// IoRaiseInformationalHardError

BOOLEAN HalTranslateBusAddress(INTERFACE_TYPE InterfaceType, ULONG BusNumber, PHYSICAL_ADDRESS BusAddress,
	PULONG AddressSpace, PPHYSICAL_ADDRESS TranslatedAddress)
	{							// HalTranslateBusAddress
	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
	*TranslatedAddress = BusAddress;
	return TRUE;
	}							// HalTranslateBusAddress

BOOLEAN ExIsProcessorFeaturePresent(ULONG feature)
	{							// ExIsProcessorFeaturePresent
	BOOLEAN result = FALSE;

	switch (feature)
		{						// determine if feature present
	case PF_FLOATING_POINT_PRECISION_ERRATA:
		result = PrecisionErrata;
		break;

	case PF_FLOATING_POINT_EMULATED:
		result = (features & FEATURE_FPU) == 0;
		break;

	case PF_COMPARE_EXCHANGE_DOUBLE:
		result = (features & FEATURE_CMPXCHG8B) != 0;
		break;

	case PF_MMX_INSTRUCTIONS_AVAILABLE:
		result = (features & FEATURE_MMX) != 0;

	case PF_PPC_MOVEMEM_64BIT_OK:
	case PF_ALPHA_BYTE_INSTRUCTIONS:
		break;					// non-x86 features

	case PF_XMMI_INSTRUCTIONS_AVAILABLE:
		result = (features & FEATURE_XMMI) != 0;
		break;

	case PF_3DNOW_INSTRUCTIONS_AVAILABLE:
		break;					// assume false until somebody needs it to work...

	case PF_RDTSC_INSTRUCTION_AVAILABLE:
		result = (features & FEATURE_RDTSC) != 0;
		break;

	case PF_PAE_ENABLED:
		break;					// Win98 doesn't run on phys addr extension machines
		}						// determine if feature present

	return result;
	}							// ExIsProcessorFeaturePresent

PVOID MmGetSystemRoutineAddress(PUNICODE_STRING name)
	{							// MmGetSystemRoutineAddress
	char ansiname[256];
	ansiname[UniToBCS((PBYTE) ansiname, name->Buffer, name->Length, 255, BCS_WANSI)] = 0;
	return _PELDR_GetProcAddress((HPEMODULE) ntoskrnl, ansiname, NULL);
	}							// MmGetSystemRoutineAddress

NTSTATUS ZwQueryInformationFile(HANDLE h, PIO_STATUS_BLOCK pstatus, PVOID fi,
	ULONG length, FILE_INFORMATION_CLASS c)
	{							// ZwQueryInformationFile
	pstatus->Status = STATUS_NOT_IMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
	}							// ZwQueryInformationFile

NTSTATUS ZwSetInformationFile(HANDLE h, PIO_STATUS_BLOCK pstatus, PVOID fi,
	ULONG length, FILE_INFORMATION_CLASS c)		
	{							// ZwSetInformationFile
	pstatus->Status = STATUS_NOT_IMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
	}							// ZwSetInformationFile

NTSTATUS ZwQueryDefaultLocale(BOOLEAN thread, LCID* plocale)
	{							// ZwQueryDefaultLocale
	*plocale = locale;
	return STATUS_SUCCESS;
	}							// ZwQueryDefaultLocale

VOID ExFreePoolWithTag(PVOID p, ULONG tag)
	{							// ExFreePoolWithTag
	ExFreePool(p);
	}							// ExFreePoolWithTag

NTSTATUS ZwLoadDriver(PUNICODE_STRING ServiceKey)
	{							// ZwLoadDriver
	return _NtKernLoadDriver(ServiceKey);
	}							// ZwLoadDriver

NTSTATUS ZwUnloadDriver(PUNICODE_STRING ServiceKey)
	{							// ZwUnloadDriver

	// Isolate the service name from the end of the key.
	// This will be the name of the driver object

	for (int i = ServiceKey->Length / sizeof(WCHAR) - 1; i >= 0; --i)
		if (ServiceKey->Buffer[i] == L'\\')
			break;
	++i;						// skip backslash
	int lname = ServiceKey->Length / sizeof(WCHAR) - i;

	WCHAR objname[128];
	wcscpy(objname, L"\\Driver\\");
	memcpy(objname + 8, ServiceKey->Buffer + i, 2 * lname);
	objname[lname + 8] = 0;
	UNICODE_STRING oname;
	RtlInitUnicodeString(&oname, objname);

	// Find the driver object

	PDRIVER_OBJECT DriverObject;
	NTSTATUS status = ObReferenceObjectByName(&oname, 0, 0, 0, *IoDriverObjectType, KernelMode, 0, (PVOID*) &DriverObject);
	if (!NT_SUCCESS(status))
		{
		KdPrint((DRIVERNAME " - Can't locate driver object %ws - %X\n", objname, status));
		return status;
		}
	ObDereferenceObject(DriverObject);	// remove reference from ObReferenceObjectByName

	// Determine the module name by reading the registry key, backscanning
	// to the last backslash, and converting to ANSI

	OBJECT_ATTRIBUTES oa;
	InitializeObjectAttributes(&oa, ServiceKey, OBJ_KERNEL_HANDLE, NULL, NULL);

	HANDLE hkey;
	status = ZwOpenKey(&hkey, KEY_READ, &oa);
	if (!NT_SUCCESS(status))
		{
		KdPrint((DRIVERNAME " - ZwOpenKey(%ws) failed - %X\n", ServiceKey->Buffer, status));
		return status;
		}

	struct foo : public _KEY_VALUE_PARTIAL_INFORMATION
		{
		UCHAR buffer[511];
		} value;
	UNICODE_STRING valname;
	ULONG size;
	RtlInitUnicodeString(&valname, L"ImagePath");
	status = ZwQueryValueKey(hkey, &valname, KeyValuePartialInformation,
		&value, sizeof(value), &size);
	ZwClose(hkey);

	if (!NT_SUCCESS(status))
		{
		KdPrint((DRIVERNAME " - ZwQueryValueKey failed - %X\n", status));
		return status;
		}

	for (i = value.DataLength / sizeof(WCHAR) - 1; i >= 0; --i)
		if (((PWCHAR) (value.Data))[i] == L'\\')
			break;
	++i;
	lname = value.DataLength / sizeof(WCHAR) - i;
	memcpy(objname, value.Data + i * 2, lname * 2);
	objname[lname] = 0;

	BYTE modname[64];
	modname[UniToBCS(modname, objname, lname * 2, sizeof(modname), BCS_WANSI)] = 0;
	HPEMODULE hmod = _PELDR_GetModuleHandle((PSTR) modname);

	if (!hmod)
		{
		KdPrint((DRIVERNAME " - Can't get module handle to %s\n", modname));
		return STATUS_UNSUCCESSFUL;
		}

	// At long last, unload the module and release the extra references to
	// the device object

	if (DriverObject->DriverUnload)
		(*DriverObject->DriverUnload)(DriverObject);
	ObDereferenceObject(DriverObject);
	ObDereferenceObject(DriverObject);
	_PELDR_FreeModule(hmod, NULL);

	return STATUS_SUCCESS;
	}							// ZwUnloadDriver

BOOLEAN SeSinglePrivilegeCheck(LUID PrivilegeValue, KPROCESSOR_MODE PreviousMode)
	{							// SeSinglePrivilegeCheck
	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
	ASSERT(PreviousMode == UserMode || PreviousMode == KernelMode);
	return TRUE;
	}							// SeSinglePrivilegeCheck

NTSTATUS IoSetCompletionRoutineEx(PDEVICE_OBJECT fdo, PIRP Irp, PIO_COMPLETION_ROUTINE CompletionRoutine,
	PVOID Context, BOOLEAN success, BOOLEAN failure, BOOLEAN cancel)		
	{							// IoSetCompletionRoutineEx
	ASSERT(fdo);
	ASSERT(Irp);
	ASSERT(CompletionRoutine);
	ASSERT(success || failure || cancel);

	PCOMPLETION_CONTEXT ctx = (PCOMPLETION_CONTEXT) _HeapAllocate(sizeof(COMPLETION_CONTEXT), 0);
	if (!ctx)
		return STATUS_INSUFFICIENT_RESOURCES;
	ctx->CompletionRoutine = CompletionRoutine;
	ctx->Context = Context;
	ctx->fdo = fdo;

	IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE) CompletionRoutineWrapper, (PVOID) ctx,
		success, failure, cancel);

	return STATUS_SUCCESS;
	}							// IoSetCompletionRoutineEx

///////////////////////////////////////////////////////////////////////////////

void __cdecl MessageCallback(char* msg)
	{							// MessageCallback
#pragma pack(2)
	struct {					// MessageBox parameters
		WORD fuStyle;			// style flags
		DWORD lpszTitle;		// title string
		DWORD lpszText;			// message text
		WORD hwndParent;		// parent window handle
		} parms;				// MessageBox parameters
#pragma pack()

	// Construct parameter list for call to MessageBox

	parms.fuStyle = MB_OK | MB_ICONHAND;
	parms.lpszTitle = _SHELL_LocalAllocEx(LMEM_STRING, 0, "Windows - Operation Failed");
	parms.lpszText = _SHELL_LocalAllocEx(LMEM_STRING, 0, msg);
	parms.hwndParent = 0;
	
	// Invoke MessageBox to display the message

	if (parms.lpszTitle && parms.lpszText)
		_SHELL_CallDll("USER", "MESSAGEBOX", sizeof(parms), &parms);

	// Cleanup

	if (parms.lpszText)
		_SHELL_LocalFree(parms.lpszText);
	if (parms.lpszTitle)
		_SHELL_LocalFree(parms.lpszTitle);
	_HeapFree(msg, 0);
	}							// MessageCallback

///////////////////////////////////////////////////////////////////////////////

__int64 GetZoneBias()
	{							// GetZoneBias
	DWORD hkey;
	if (_RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_TIMEZONE, &hkey) != 0)
		return 0;

	__int64 bias;
	LONG actbias;
	DWORD size = sizeof(actbias);

	if (_RegQueryValueEx(hkey, REGSTR_VAL_TZACTBIAS, NULL, NULL, &actbias, &size) == 0)
		{						// compute 100-ns bias
		actbias *= 60;			// minutes to seconds
		_asm mov eax, actbias
		_asm mov ecx, 10000000	// seconds to 100-ns units
		_asm imul ecx
		_asm mov dword ptr bias, eax
		_asm mov dword ptr bias+4, edx

		}						// compute 100-ns bias
	else
		bias = 0;

	_RegCloseKey(hkey);
	return bias;
	}							// GetZoneBias

///////////////////////////////////////////////////////////////////////////////

DWORD atox(char* s)
	{							// atox
	DWORD result = 0;
	char ch;
	while ((ch = *s++))
		{						// convert hex number
		BYTE hexit;
		if (ch >= '0' && ch <= '9')
			hexit = ch - '0';
		else if (ch >= 'A' && ch <= 'F')
			hexit = ch - ('A' - 10);
		else if ( ch >= 'a' && ch <= 'f')
			hexit = ch - ('a' - 10);
		else
			break;				// invalid hexit -- ignore

		result <<= 4;
		result += hexit;
		}						// convert hex number

	return result;
	}							// atox

///////////////////////////////////////////////////////////////////////////////

LCID GetSystemLocale()
	{							// GetSystemLocale
	LCID lcid = LOCALE_SYSTEM_DEFAULT;	// in case nothing found
	#define REGSTR_PATH_LOCALE TEXT("System\\CurrentControlSet\\Control\\Nls\\Locale")

	// Query the registry to determine the default locale

	DWORD hKey;
	if (_RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_LOCALE, &hKey) != 0)
		return lcid;

	char szLocale[9];
	DWORD size = sizeof(szLocale);
	DWORD type;

	if (_RegQueryValueEx(hKey, NULL, NULL, &type, (PBYTE) szLocale, &size) == 0 && type == REG_SZ)
		lcid = atox(szLocale);

	_RegCloseKey(hKey);
	return lcid;
	}							// GetSystemLocale

///////////////////////////////////////////////////////////////////////////////

#pragma warning(disable:4035)

LONG __stdcall Get_System_Time()
	{							// Get_System_Time
	VMMCall(Get_System_Time)
	}							// Get_System_Time

#pragma warning(default:4035)

///////////////////////////////////////////////////////////////////////////////

NTSTATUS CompletionRoutineWrapper(PDEVICE_OBJECT junk, PIRP Irp, PCOMPLETION_CONTEXT ctx)
	{							// CompletionRoutineWrapper
	ObReferenceObject(ctx->fdo);
	NTSTATUS status = (*ctx->CompletionRoutine)(junk, Irp, ctx->Context);
	ObDereferenceObject(ctx->fdo);
	_HeapFree((PVOID) ctx, 0);
	return status;
	}							// CompletionRoutineWrapper