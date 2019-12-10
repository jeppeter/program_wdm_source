// FASTINST.CPP -- Fast device installer for Windows 9x
// Copyright (C) 1999 by Walter Oney
// All rights reserved

#include "stdafx.h"
#include "setup.h"
#include "progress.h"
#include "ctl3d.h"

BOOL DeviceCallback(CInf* inf, const CString& devname, const CString& instsecname, const CStringArray& devid, PVOID context, UINT& code);
LRESULT CALLBACK GenericCallback(LPGENCALLBACKINFO info, LPARAM param);
BOOL MfgCallback(CInf* inf, const CString& mfgname, const CString& modelname, PVOID context, UINT& code);

///////////////////////////////////////////////////////////////////////////////

class CFastInst : public CWinApp
{								// class CFastInst
public:
	virtual BOOL InitInstance();
};								// class CFastInst

CFastInst theApp;

BOOL CFastInst::InitInstance()
	{							// CFastInst::InitInstance
	CString infname = m_lpCmdLine;
	UINT code;

	// Open the specifid INF file

	CInf inf;
	code = inf.Open(infname, FALSE);
	if (code != 0)
		return FALSE;

	// Locate the model statement for the designated device, or for the first device
	// in the file.

	InstallStruct is;
	is.done = FALSE;

	code = inf.EnumManufacturers(MfgCallback, (PVOID) &is);
	if (code != 0)
		return FALSE;

	// Create a Device Information structure describing the device we're going to install

	LPDEVICE_INFO dip;
	code = DiCreateDeviceInfo(&dip, is.devname, 0, HKEY_LOCAL_MACHINE, NULL, inf.m_classname, GetDesktopWindow());
	if (code != 0)
		return FALSE;

	dip->atDriverPath = inf.m_atom;
	dip->Flags |= DI_ENUMSINGLEINF | DI_QUIETINSTALL;

	// Create a list of drivers in this INF file

	code = DiBuildClassDrvList(dip);
	
	if (code == 0)
		{						// install driver
		
		// Select the driver for the specified hardware id
		
		LPDRIVER_NODE dnp = dip->lpClassDrvList;
		while (dnp)
			{					// select driver
			if (dnp->lpszHardwareID == is.devid)
				break;
			dnp = dnp->lpNextDN;
			}
		
		if (!dnp)
			dnp = dip->lpClassDrvList;
		dip->lpSelectedDriver = dnp;
	
		// Change the LDID_SRCPATH path to the directory containing the INF. This is a hack
		// that makes relative pathnames in the INF work correctly.
		
		char drive[_MAX_DRIVE], dir[_MAX_DIR], infdir[_MAX_PATH];
		_splitpath(inf.m_name, drive, dir, NULL, NULL);
		_makepath(infdir, drive, dir, NULL, NULL);
		CtlSetLddPath(LDID_SRCPATH, infdir);
		
		// Display a pseudo-progress dialog to reassure the user that something
		// is actually happening
	
		Ctl3dRegister(m_hInstance);
		Ctl3dAutoSubclass(m_hInstance);	
		RegisterGauge();
		CProgressDlg progress;
		CString pattern;
		pattern.LoadString(IDS_CAPTION);
		progress.m_caption.Format(pattern, (const char*) is.devname, (const char*) inf.m_name);
		progress.DoModeless();
		progress.ShowWindow(SW_SHOW);
	
		// Install the device and the driver
		
		if (code == 0)
			{
			DiInstallClass(infname, 0);	// in case there's a custom class in the INF file
			code = DiInstallDevice(dip);
			}
			
		progress.ShowWindow(SW_HIDE);
		Ctl3dUnregister(m_hInstance);
		}						// install driver


	DiDestroyDeviceInfoList(dip);

	return FALSE;
	}							// CFastInst::InitInstance

///////////////////////////////////////////////////////////////////////////////

BOOL DeviceCallback(CInf* inf, const CString& devname, const CString& instsecname, const CStringArray& devid, PVOID context, UINT& code)
	{
	InstallStruct* isp = (InstallStruct*) context;
	int n = devid.GetSize();
	for (int i = 0; i < n; ++i)
		if (isp->devid == devid[i] || isp->devid.GetLength() == 0)
			break;

	if (i >= n)
		return TRUE;			// keep enumerating -- this isn't it

	isp->devid = devid[i];
	isp->devname = devname;
	isp->instname = instsecname;

	code = 0;					// successful conclusion
	isp->done = TRUE;
	return FALSE;				// stop enumeration
	}

///////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK GenericCallback(LPGENCALLBACKINFO info, LPARAM param)
	{							// GenericCallback
	return ERR_DI_DO_DEFAULT;
	}							// GenericCallback

///////////////////////////////////////////////////////////////////////////////

BOOL MfgCallback(CInf* inf, const CString& mfgname, const CString& modelname, PVOID context, UINT& code)
	{
	InstallStruct* isp = (InstallStruct*) context;
	isp->mfgname = mfgname;
	code = inf->EnumDevices(modelname, DeviceCallback, context);
	return code == 0 && !isp->done;
	}
