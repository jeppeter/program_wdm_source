// stddcls.h -- Precompiled headers for WDM drivers
// Copyright (C) 1999, 2000 by Walter Oney
// All rights reserved

#ifdef __cplusplus
extern "C" {
#endif

#include <wdm.h>
#include <stdio.h>

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

#if !DBG
#pragma warning(disable:4390)	// empty controlled stmt [e.g., if() KdPrint();]
#endif

#if DBG && defined(_X86_)
	#undef ASSERT
	#define ASSERT(e) if(!(e)){DbgPrint("Assertion failure in "\
	__FILE__ ", line %d: " #e "\n", __LINE__);\
	_asm int 3\
  }
#endif
