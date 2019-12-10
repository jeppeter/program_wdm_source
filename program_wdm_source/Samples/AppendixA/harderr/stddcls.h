// stddcls.h -- Precompiled headers for WDM drivers
// Copyright (C) 1999 by Walter Oney
// All rights reserved

#ifdef __cplusplus
extern "C" {
#endif

#pragma warning(disable:4201)	// nameless struct/union
#define DEPRECATE_DDK_FUNCTIONS 1

#include <ntddk.h>
#include <stdio.h>

#ifdef __cplusplus
	}
#endif

#define PAGEDCODE code_seg("page")
#define LOCKEDCODE code_seg()
#define INITCODE code_seg("init")

#define PAGEDDATA data_seg("page")
#define LOCKEDDATA data_seg()
#define INITDATA data_seg("init")

#define arraysize(p) (sizeof(p)/sizeof((p)[0]))
