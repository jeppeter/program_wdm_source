// WdmcheckHelper.cpp -- Implementation of CWdmcheckHelper class
// Copyright (C) 2000 by Walter Oney
// All rights reserved

#include "stdafx.h"
#include "WdmcheckHelper.h"
#include "..\vxd\ioctls.h"

///////////////////////////////////////////////////////////////////////////////

CWdmcheckHelper::CWdmcheckHelper() : CVxdHelper()
	{							// CWdmcheckHelper::CWdmcheckHelper 
	}							// CWdmcheckHelper::CWdmcheckHelper 

CWdmcheckHelper::~CWdmcheckHelper()
	{							// CWdmcheckHelper::~CWdmcheckHelper 
	}							// CWdmcheckHelper::~CWdmcheckHelper 

///////////////////////////////////////////////////////////////////////////////

DWORD CWdmcheckHelper::IsExported(char* modname, char* fcn)
	{							// CWdmcheckHelper::IsExported
	EXPORT_DEFINED_PARMS parms = {modname, strlen(modname) + 1, fcn};
	DWORD junk;
	DWORD exported;

	if (HIWORD(fcn))
		parms.fcnlen = strlen(fcn) + 1;
	
	if (!DeviceIoControl(m_hdriver, IOCTL_EXPORT_DEFINED, &parms, sizeof(parms), &exported, sizeof(exported), &junk, NULL))
		exported = WDMCHECK_NOT_DEFINED;

	return exported;
	}							// CWdmcheckHelper::IsExported
