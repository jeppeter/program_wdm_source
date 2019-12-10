// StdVxd.h -- Standard declarations for C-language VxDs
// VxD Wizard generated for wdmcheck project

#ifndef STDVXD_H
#define STDVXD_H

#ifdef __cplusplus
	extern "C" {
	#define SYSCTL extern "C"
#else // not __cplusplus
	#define SYSCTL
#endif // not __cplusplus

#ifdef DEBUG
	#define DBG 1
#else
	#define DBG 0
#endif

#include <wdm.h>
#include <stdio.h>

#define WANTVXDWRAPS
#define _H2INC
#include <basedef.h>
#undef _H2INC
#undef NULL
#undef PASCAL
#undef FAR
#define NULL 0
#define FAR
#include <vmm.h>
#include <debug.h>
#include <regdef.h>
#include <vmmreg.h>
#include <vxdldr.h>

#pragma warning(disable:4229)
#include <shell.h>
#pragma warning(default:4229)

#include <vxdwraps.h>
#include <configmg.h>

#include <crs.h>
#include <stdarg.h>
#include <winerror.h>
#include <vwin32.h>

#ifdef __cplusplus
} // extern "C"
#endif
		
///////////////////////////////////////////////////////////////////////////////
// Useful general-purpose definitions

#include <string.h>
#pragma intrinsic(memcmp, memcpy, memset, strcat, strcmp, strcpy, strlen)

#undef ASSERT

#ifdef DEBUG
	#define ASSERT(e) if(!(e)){Debug_Printf("Assertion failure in " __FILE__ ", line %d: " #e "\r\n", __LINE__);\
		_asm int 1\
		}
#else
	#define ASSERT(e)
#endif

#ifndef MAKELONG
	#define MAKELONG(low, high) ((LONG)(((WORD)(low)) | (((DWORD)((WORD)(high))) << 16)))
#endif

#ifndef FIELDOFFSET
	#define FIELDOFFSET(type, field) ((DWORD)(&((type *)0)->field))
#endif

#ifndef arraysize
	#define arraysize(p) (sizeof(p)/sizeof((p)[0]))
#endif

#ifndef NAKED
	#define NAKED __declspec(NAKED)
#endif

#ifndef min
	#define min(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef max
	#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#endif // STDVXD_H
