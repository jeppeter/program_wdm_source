// SampleCoinstaller.cpp -- A sample device co-installer DLL
// Copyright (C) 2002 by Walter Oney
// All rights reserved

#include "stdafx.h"
#include "SampleCoinstaller.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
// CSampleCoinstallerApp

BEGIN_MESSAGE_MAP(CSampleCoinstallerApp, CExternalDialogApp)
	//{{AFX_MSG_MAP(CSampleCoinstallerApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

///////////////////////////////////////////////////////////////////////////////
// CSampleCoinstallerApp construction

CSampleCoinstallerApp::CSampleCoinstallerApp()
	{							// CSampleCoinstallerApp::CSampleCoinstallerApp
	m_shoesize = NULL;
	m_shoesizeprop = NULL;
	}							// CSampleCoinstallerApp::CSampleCoinstallerApp

CSampleCoinstallerApp::~CSampleCoinstallerApp()
	{							// CSampleCoinstallerApp::~CSampleCoinstallerApp
	if (m_shoesize)
		delete m_shoesize;
	if (m_shoesizeprop)
		delete m_shoesizeprop;
	}							// CSampleCoinstallerApp::~CSampleCoinstallerApp

///////////////////////////////////////////////////////////////////////////////
// The one and only CSampleCoinstallerApp object

CSampleCoinstallerApp theApp;

///////////////////////////////////////////////////////////////////////////////
// this override of the base class AddPropertyPages function adds a custom
// page to the device manager propety sheet.

DWORD CSampleCoinstallerApp::AddPropertyPages(HDEVINFO infoset, PSP_DEVINFO_DATA did, PVOID& PostContext)
	{							// CSampleCoinstallerApp::AddPropertyPages
	m_shoesizeprop = new CShoeSizeProperty;
	AddPropertyPage(infoset, did, m_shoesizeprop);
	return NO_ERROR;
	}							// CSampleCoinstallerApp::AddPropertyPages

///////////////////////////////////////////////////////////////////////////////
// This override of the base class FinishInstall function adds a custom wizard
// page to the setup wizard for our dummy device. The page allows the end user
// to pre-specify the value of the whimsical ProgrammersShoeSize parameter.

DWORD CSampleCoinstallerApp::FinishInstall(HDEVINFO infoset, PSP_DEVINFO_DATA did, PVOID& PostContext)
	{							// CSampleCoinstallerApp::FinishInstall
	m_shoesize = new CShoeSize;
	AddPropertyPage(infoset, did, m_shoesize);
	return NO_ERROR;
	}							// CSampleCoinstallerApp::FinishInstall
