// Coinstaller.cpp -- Implementation of CCoinstaller class
// Copyright (C) 2002 by Walter Oney
// All rights reserved

#include "stdafx.h"
#include "Coinstaller.h"
#pragma comment(lib, "setupapi")
#include "resource.h"

BEGIN_MESSAGE_MAP(CCoinstaller, CExternalDialogApp)
END_MESSAGE_MAP()

///////////////////////////////////////////////////////////////////////////////
// This is the exported coinstaller procedure for this DLL. Note that this function
// has to be exported under the name "CoinstallerProc" in the .DEF file because of
// the __stdcall decoration.

extern "C" DWORD __stdcall CoinstallerProc(DI_FUNCTION dif, HDEVINFO infoset, PSP_DEVINFO_DATA did, PCOINSTALLER_CONTEXT_DATA ctx)
	{							// CoinstallerProc

	// Handle MFC plumbing. This will actually do something only if you link the
	// DLL dynamically with the MFC libraries, which isn't a very good idea
	// because it requires the end user to have the appropriate DLLs installed.

	CCoinstaller* pThis = (CCoinstaller*) CExternalDialogApp::m_theApp;
	AFX_MANAGE_STATE(pThis->GetModuleStatePointer());

	// Invoke a virtual function to handle the specified install function. We have
	// one set of functions for pre-processing and another for post-processing,
	// and they have slightly different calling sequences.

	DWORD result;

	if (!ctx->PostProcessing)
		{						// pre-processing
		PVOID PostContext = NULL;
		result = NO_ERROR;		// default return code

		#define CIFUNC(f,vf) \
			case f: \
				result = pThis->vf(infoset, did, PostContext); \
				break;
		
		switch(dif)
			{					// process install function
			#include "cifunc.h"
			}					// process install function

		ASSERT(result != ERROR_DI_DO_DEFAULT);	// not allowed for a coinstaller

		if (result == ERROR_DI_POSTPROCESSING_REQUIRED)
			ctx->PrivateData = PostContext;		// set context for post-processing callback
		}						// pre-processing

	else
		{						// post-processing
		result = ctx->InstallResult;	// default is to propagate same result

		#define CIFUNC(f, vf) \
			case f: \
				result = pThis->vf##Post(infoset, did, ctx->PrivateData, ctx->InstallResult); \
				break;
		
		switch(dif)
			{					// process install function
			#include "cifunc.h"
			}					// process install function

		ASSERT(result != ERROR_DI_DO_DEFAULT);	// not allowed for a coinstaller
		}						// post-processing

	return result;
	}							// CoinstallerProc

///////////////////////////////////////////////////////////////////////////////
// AddPropertyPage adds a property page to the Device Manager or install wizard. 
// This function would be called by a derived class AddPropertyPages or
// FinishInstall function

BOOL CCoinstaller::AddPropertyPage(HDEVINFO infoset, PSP_DEVINFO_DATA did, CCoinstallerDialog* page)
	{							// CCoinstaller::AddPropertyPage

	// Get the class install parameters for the current function. This is an odd sort
	// of API. It knows somehow what install function we're handling, and
	// so it "knows" that we want an SP_ADDPROPERTYPAGE_DATA structure as output.

	SP_ADDPROPERTYPAGE_DATA ppd = {{sizeof(SP_CLASSINSTALL_HEADER)}};
	if (!SetupDiGetClassInstallParams(infoset, did, &ppd.ClassInstallHeader, sizeof(ppd), NULL))
		return FALSE;			// can't get class install parameters for this device

	// Based on which DIF code we're handling, decide if this is a wizard page or not

	BOOL iswiz = ppd.ClassInstallHeader.InstallFunction != DIF_ADDPROPERTYPAGE_ADVANCED;

	// Make sure we won't exceed the maximum number of property pages

	if (ppd.NumDynamicPages >= MAX_INSTALLWIZARD_DYNAPAGES)
		return FALSE;

	// Save information about the device in question for use by members of the
	// dialog class.

	page->m_infoset = infoset;
	page->m_did = did;

	// Create a property page

	PROPSHEETPAGE proppage = {sizeof(PROPSHEETPAGE)};

	proppage.pfnDlgProc = DummyDlgProc;			// to force window to get subclassed safely
	proppage.lParam = (LPARAM) page;

	proppage.hInstance = AfxGetResourceHandle();
	proppage.pszTemplate = MAKEINTRESOURCE(page->m_id);
	
	if (page->m_caption)
		{						// has a caption
		proppage.pszTitle = MAKEINTRESOURCE(page->m_caption);
		proppage.dwFlags |= PSP_USETITLE;
		}						// has a caption
	
	if (page->m_title)
		{						// has a title
		ASSERT(iswiz);
		proppage.dwFlags |= PSP_USEHEADERTITLE;
		proppage.pszHeaderTitle = MAKEINTRESOURCE(page->m_title);
		}						// has a title
	
	if (page->m_subtitle)
		{						// has a subtitle
		ASSERT(iswiz);
		proppage.dwFlags |= PSP_USEHEADERSUBTITLE;
		proppage.pszHeaderSubTitle = MAKEINTRESOURCE(page->m_subtitle);
		}						// has a subtitle

	HPROPSHEETPAGE hpage = CreatePropertySheetPage(&proppage);
	if (!hpage)
		return FALSE;			// can't create the page

	// Add the page to the wizard 

	ppd.DynamicPages[ppd.NumDynamicPages++] = hpage;
	SetupDiSetClassInstallParams(infoset, did, &ppd.ClassInstallHeader, sizeof(ppd));

	return TRUE;

	}							// CCoinstaller::AddPropertyPage

///////////////////////////////////////////////////////////////////////////////
// Base class functions. These all return the default status

#define CIFUNC(f, vf) DWORD CCoinstaller::vf(HDEVINFO infoset, PSP_DEVINFO_DATA did, PVOID& PostContext) {return NO_ERROR;}
#include "cifunc.h"

#define CIFUNC(f, vf) DWORD CCoinstaller::vf##Post(HDEVINFO infoset, PSP_DEVINFO_DATA did, PVOID PostContext, DWORD InstallResult) {return InstallResult;}
#include "cifunc.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CCoinstallerDialog, CExternalDialog)

CCoinstallerDialog::CCoinstallerDialog(UINT id, UINT caption, UINT title, UINT subtitle)
	: CExternalDialog()
	{							// CCoinstallerDialog::CCoinstallerDialog
	m_id = id;
	m_caption = caption;
	m_title = title;
	m_subtitle = subtitle;
	}							// CCoinstallerDialog::CCoinstallerDialog

///////////////////////////////////////////////////////////////////////////////
// This override for OnInitDialog initializes the icon and device title controls
// for a device manager property page

BOOL CCoinstallerDialog::OnInitDialog()
	{							// CCoinstallerDialog::OnInitDialog
	HWND hwnd;
	GetDlgItem(IDC_CLASSICON, &hwnd);
	if (hwnd)
		{						// set icon
		HICON hIcon;
		if (SetupDiLoadClassIcon(&m_did->ClassGuid, &hIcon, NULL))
			::SendMessage(hwnd, STM_SETICON, (WPARAM) (HANDLE) hIcon, 0);
		else
			::ShowWindow(hwnd, SW_HIDE);
		}						// set icon

	GetDlgItem(IDC_DEVNAME, &hwnd);
	if (hwnd)
		{						// set device name
		TCHAR name[256];
		if (!SetupDiGetDeviceRegistryProperty(m_infoset, m_did, SPDRP_FRIENDLYNAME, NULL, (PBYTE) name, sizeof(name), NULL)
			&& !SetupDiGetDeviceRegistryProperty(m_infoset, m_did, SPDRP_DEVICEDESC, NULL, (PBYTE) name, sizeof(name), NULL))
			name[0] = 0;

		::SetWindowText(hwnd, name);
		}						// set device name

	return CExternalDialog::OnInitDialog();
	}							// CCoinstallerDialog::OnInitDialog