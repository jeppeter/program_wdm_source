// FeatureReports.h -- Declaration of feature report structures for HIDFAKE driver
// Copyright (C) 2002 by Walter Oney
// All rights reserved

// The HIDFAKE sample driver uses feature reports for out-of-bad communication
// between an application and the driver. This file, which is shared between
// the user-mode test applet and the driver, defines the format of the feature
// reports.

#pragma once

#include <pshpack1.h>

// Dummy VID/PID values to simplify test applet's scan. You *must* use your own
// unique values here, or else you'll likely collide with some other vendor's
// fake HID device.

#define HIDFAKE_VID		0x1234
#define HIDFAKE_PID		0x5678

///////////////////////////////////////////////////////////////////////////////

typedef struct _FEATURE_REPORT_GET_VERSION {
	UCHAR id;					// feature id (FEATURE_CODE_GET_VERSION)
	ULONG Version;				// driver revision level
	} FEATURE_REPORT_GET_VERSION, *PFEATURE_REPORT_GET_VERSION;

#define FEATURE_CODE_GET_VERSION	2

///////////////////////////////////////////////////////////////////////////////

typedef struct _FEATURE_REPORT_SET_BUTTON {
	UCHAR id;					// feature id (FEATURE_CODE_SET_BUTTON)
	UCHAR ButtonDown;			// TRUE if button 1 considered down, FALSE otherwise
	} FEATURE_REPORT_SET_BUTTON, *PFEATURE_REPORT_SET_BUTTON;

#define FEATURE_CODE_SET_BUTTON		3

#include <poppack.h>