// VxdHelper.cpp -- Implementation of CVxdHelper class
// Copyright (C) 1999 by Walter Oney
// All rights reserved

#include "stdafx.h"
#include "VxdHelper.h"

///////////////////////////////////////////////////////////////////////////////

CVxdHelper::CVxdHelper()
	{							// CVxdHelper::CVxdHelper
	m_hdriver = INVALID_HANDLE_VALUE;
	}							// CVxdHelper::CVxdHelper

CVxdHelper::~CVxdHelper()
	{							// CVxdHelper::~CVxdHelper
	if (m_hdriver != INVALID_HANDLE_VALUE)
		{						// close handle to driver
		CloseHandle(m_hdriver);
		m_hdriver = INVALID_HANDLE_VALUE;
		}						// close handle to driver
	}							// CVxdHelper::~CVxdHelper

///////////////////////////////////////////////////////////////////////////////

BOOL CVxdHelper::Initialize(LPCTSTR name)
	{							// CVxdHelper::Initialize
	TCHAR devname[_MAX_PATH];
	_tcscpy(devname, _T("\\\\.\\"));
	_tcscat(devname, name);

	m_hdriver = CreateFile(devname, GENERIC_READ | GENERIC_WRITE, 0,
		NULL, OPEN_EXISTING, 0, NULL);

	if (m_hdriver == INVALID_HANDLE_VALUE)
		{						// try to dynamically load VxD
		_tcscat(devname, _T(".vxd"));
		m_hdriver = CreateFile(devname, 0, 0, NULL, 0, FILE_FLAG_DELETE_ON_CLOSE, NULL);
		}						// try to dynamically load VxD

	if (m_hdriver == INVALID_HANDLE_VALUE)
		return FALSE;

	return TRUE;
	}							// CVxdHelper::Initialize
