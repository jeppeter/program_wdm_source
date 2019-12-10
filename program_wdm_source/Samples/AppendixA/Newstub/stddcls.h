// stddcls.h -- Precompiled headers for WDM drivers
// Copyright (C) 1999, 2000 by Walter Oney
// All rights reserved

#ifdef __cplusplus
extern "C" {
#endif

#define _BLDR_
#define _NTSYSTEM_			// to suppress import declarations
#include <ntddk.h>

#undef ExFreePool			// suppress macro to ExFreePoolWithTag

#define WANTVXDWRAPS
#define _H2INC
#include <basedef.h>
#undef _H2INC
#undef NULL
#undef PASCAL
#undef FAR
#define NULL (0)
#define FAR
#include <vmm.h>
#include <debug.h>
#include <regdef.h>
#include <vmmreg.h>

#pragma warning(disable:4229)
#include <shell.h>
#pragma warning(default:4229)
#include <vxdldr.h>
#include <ntkern.h>

#include <vxdwraps.h>
#include <configmg.h>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <winerror.h>
#include <vwin32.h>
#include <vtd.h>
#include <regstr.h>

#ifdef __cplusplus
	}
#endif

#define PAGEDCODE code_seg("PAGE")
#define LOCKEDCODE code_seg()
#define INITCODE code_seg("INIT")

#define PAGEDDATA data_seg("PAGEDATA")
#define LOCKEDDATA data_seg()
#define INITDATA data_seg("INITDATA")

#define arraysize(p) (sizeof(p)/sizeof((p)[0]))

// Override DDK definition of ASSERT so that debugger halts in the
// affected code and halts even in the unchecked OS

#if DBG && defined(_X86_)
	#undef ASSERT
	#define ASSERT(e) if(!(e)){DbgPrint("Assertion failure in "\
	__FILE__ ", line %d: " #e "\n", __LINE__);\
	_asm int 3\
  }
#endif
