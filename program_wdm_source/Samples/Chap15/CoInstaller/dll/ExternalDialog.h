// ExternalDialog.h -- Declaration of CExternalDialog class
// Copyright (C) 2002 by Walter Oney
// All rights reserved

// CExternalDialog is an MFC CPropertyPage built on top of an externally created property
// page. This class is useful in situations, such as a coinstaller DLL, where you need
// to add a property page to a property sheet that someone else owns.

#pragma once

///////////////////////////////////////////////////////////////////////////////

class CExternalDialog : public CPropertyPage
{								// class CExternalDialog
	DECLARE_DYNAMIC(CExternalDialog)

public:
	CExternalDialog();
	virtual ~CExternalDialog();

	virtual WNDPROC* GetSuperWndProcAddr();
	WNDPROC m_wndproc;
	AFX_MODULE_STATE* m_modstate;
};								// class CExternalDialog

BOOL CALLBACK DummyDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);

///////////////////////////////////////////////////////////////////////////////

class CExternalDialogApp : public CWinApp
	{								// class CExternalDialogApp
public:
	CExternalDialogApp() {m_theApp = this;}

#ifdef _AFXDLL
	__inline AFX_MODULE_STATE* GetModuleStatePointer() {return m_pModuleState;}
#else
	__inline AFX_MODULE_STATE* GetModuleStatePointer() {return NULL;}
#endif

	static CExternalDialogApp* m_theApp;

	DECLARE_MESSAGE_MAP()
	};								// class CExternalDialogApp