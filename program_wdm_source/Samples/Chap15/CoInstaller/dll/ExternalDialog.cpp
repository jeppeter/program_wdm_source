// ExternalDialog.cpp -- Implementation of CExternalDialog class
// Copyright (C) 2002 by Walter Oney
// All rights reserved

#include "stdafx.h"
#include "ExternalDialog.h"

CExternalDialogApp* CExternalDialogApp::m_theApp;

BEGIN_MESSAGE_MAP(CExternalDialogApp, CWinApp)
END_MESSAGE_MAP()

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CExternalDialog, CPropertyPage)

CExternalDialog::CExternalDialog() : CPropertyPage()
	{							// CExternalDialog::CExternalDialog
	m_wndproc = NULL;

	// Capture the module state pointer. This is a protected member of CCmdTarget
	// and therefore not directly available to our DummyDlgProc.

	m_modstate = CExternalDialogApp::m_theApp->GetModuleStatePointer();
	}							// CExternalDialog::CExternalDialog

CExternalDialog::~CExternalDialog()
	{							// CExternalDialog::~CExternalDialog
	}							// CExternalDialog::~CExternalDialog

// GetSuperWndProcAddr must be overridden because it's a protected member of
// CWnd and therefore unavailable to our DummyDlgProc

WNDPROC* CExternalDialog::GetSuperWndProcAddr()
	{							// CExternalDialog::GetSuperWndProcAddr
	return &m_wndproc;
	}							// CExternalDialog::GetSuperWndProcAddr

///////////////////////////////////////////////////////////////////////////////
// This dummy dialog procedure is in place just long enough to handle WM_INITDIALOG.
// Its purpose is to subclass the dialog window the same way MFC normally does when
// you create a dialog derived from CPropertyPageEx.

BOOL CALLBACK AfxDlgProc(HWND hWnd, UINT message, WPARAM, LPARAM);

BOOL CALLBACK DummyDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
	{							// DummyDlgProc
	if (!lParam)
		return FALSE;			// not WM_INITDIALOG yet

	// Subclass the window so as to switch to AfxWndProc as the window procedure. This
	// duplicates what _AfxCbtFilterHook does during the creation of the dialog window

	CExternalDialog* dlg = (CExternalDialog*) ((LPPROPSHEETPAGE) lParam)->lParam;
	AFX_MANAGE_STATE(dlg->m_modstate);
	dlg->Attach(hdlg);
	dlg->PreSubclassWindow();
	WNDPROC* pOldWndProc = dlg->GetSuperWndProcAddr();
	WNDPROC afxWndProc = AfxGetAfxWndProc();
	*pOldWndProc = (WNDPROC) SetWindowLong(hdlg, GWL_WNDPROC, (DWORD) afxWndProc);

	// Replace the dialog proc pointer with AfxDlgProc

	SetWindowLong(hdlg, DWL_DLGPROC, (DWORD) AfxDlgProc);

	// Now let MFC process this message

	return CallWindowProc(afxWndProc, hdlg, msg, wParam, NULL);
	}							// DummyDlgProc
