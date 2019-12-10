// Coinstaller.h -- Declaration of base classes for coinstaller DLLs
// Copyright (C) 2002 by Walter Oney
// All rights reserved

// To use these base classes, create an MFC DLL project.Add the Coinstaller and
// ExternalDialog cpp and h files, and cifunc.h, to your project. Alter the main application
// class to be derived from CCoinstaller and provide overrides for any desired
// virtual functions.
//
// After creating a dialog template for a property page in the resource editor,
// it's easiest to use Class Wizard to define a class derived from CPropertyPage
// (since CCoinstallerDialog won't show up.) Afterwards:
//
//	1.	Edit the dialog class header file to change the base class from
//		CPropertyPage to CCoinstallerDialog,
//
//	2.	Edit the dialog class implementation file to change all references to
//		CPropertyPage to CCoinstallerDialog
//
//	3.	Use operator new to create an instance of the class. If you put a pointer to
//		the page in your CCoinstaller object, you can easily destroy it in your
//		destructor.
//
//	Note: in designing a device manager property page, define the following controls,
//	which are initialized by CCoinstallerDialog::OnInitDialog
//		ICON            "",IDC_CLASSICON,7,7,20,20
//		LTEXT           "Name of sample device",IDC_DEVNAME,37,9,174,16


#pragma once

#include "ExternalDialog.h"
#include <setupapi.h>

///////////////////////////////////////////////////////////////////////////////

class CCoinstallerDialog : public CExternalDialog
{							// class CCoinstallerDialog
	DECLARE_DYNAMIC(CCoinstallerDialog)

public:
	CCoinstallerDialog(UINT id, UINT caption = 0, UINT title = 0, UINT subtitle = 0);

	HDEVINFO m_infoset;
	PSP_DEVINFO_DATA m_did;

	UINT m_id;
	UINT m_caption;
	UINT m_title;
	UINT m_subtitle;

protected:
	virtual BOOL OnInitDialog();
};							// class CCoinstallerDialog

///////////////////////////////////////////////////////////////////////////////

class CCoinstaller : public CExternalDialogApp
{								// class CCoinstaller
public:
	CCoinstaller() {}

	BOOL AddPropertyPage(HDEVINFO infoset, PSP_DEVINFO_DATA did, CCoinstallerDialog* page);

	// Virtual functions for handling each relevant install function

	#define CIFUNC(f, vf) virtual DWORD vf(HDEVINFO infoset, PSP_DEVINFO_DATA did, PVOID& PostContext); 
	#include "cifunc.h"

	#define CIFUNC(f, vf) virtual DWORD vf##Post(HDEVINFO infoset, PSP_DEVINFO_DATA did, PVOID PostContext, DWORD InstallResult);
	#include "cifunc.h"

	// The exported coinstaller procedure:

	static DWORD CoinstallerProc(DI_FUNCTION dif, HDEVINFO infoset, PSP_DEVINFO_DATA did, PCOINSTALLER_CONTEXT_DATA ctx);

	DECLARE_MESSAGE_MAP()
};								// class CCoinstaller