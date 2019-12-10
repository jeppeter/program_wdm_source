// SampleCoinstaller.h : main header file for the SAMPLECOINSTALLER DLL
//

#if !defined(AFX_SAMPLECOINSTALLER_H__FCE7888A_4AD2_440E_9112_A564A416A302__INCLUDED_)
#define AFX_SAMPLECOINSTALLER_H__FCE7888A_4AD2_440E_9112_A564A416A302__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
#include "Coinstaller.h"

#include "ShoeSize.h"			// shoe size dialog for install wizard
#include "ShoeSizeProperty.h"	// shoe size dialog for device mgr property sheet

/////////////////////////////////////////////////////////////////////////////
// CSampleCoinstallerApp
// See SampleCoinstaller.cpp for the implementation of this class
//

class CSampleCoinstallerApp : public CCoinstaller
{
public:
	CSampleCoinstallerApp();
	virtual ~CSampleCoinstallerApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSampleCoinstallerApp)
	//}}AFX_VIRTUAL

public:
	virtual DWORD AddPropertyPages(HDEVINFO infoset, PSP_DEVINFO_DATA did, PVOID& PostContext); 
	virtual DWORD FinishInstall(HDEVINFO infoset, PSP_DEVINFO_DATA did, PVOID& PostContext); 

	CShoeSize* m_shoesize;		// shoe size dialog page
	CShoeSizeProperty* m_shoesizeprop;

	//{{AFX_MSG(CSampleCoinstallerApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CSampleCoinstallerApp theApp;

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SAMPLECOINSTALLER_H__FCE7888A_4AD2_440E_9112_A564A416A302__INCLUDED_)
