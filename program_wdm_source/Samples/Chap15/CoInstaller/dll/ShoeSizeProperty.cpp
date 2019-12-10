// ShoeSizeProperty.cpp : implementation file
//

#include "stdafx.h"
#include "SampleCoinstaller.h"
#include "ShoeSizeProperty.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
// CShoeSizeProperty property page

IMPLEMENT_DYNAMIC(CShoeSizeProperty, CCoinstallerDialog)

CShoeSizeProperty::CShoeSizeProperty() : CCoinstallerDialog(IDD)
	{							// CShoeSizeProperty::CShoeSizeProperty
	//{{AFX_DATA_INIT(CShoeSizeProperty)
	m_shoesize = 1;
	m_restart = FALSE;
	//}}AFX_DATA_INIT
	}							// CShoeSizeProperty::CShoeSizeProperty

CShoeSizeProperty::~CShoeSizeProperty()
	{							// CShoeSizeProperty::~CShoeSizeProperty
	}							// CShoeSizeProperty::~CShoeSizeProperty

void CShoeSizeProperty::DoDataExchange(CDataExchange* pDX)
	{							// CShoeSizeProperty::DoDataExchange
	CCoinstallerDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CShoeSizeProperty)
	DDX_Text(pDX, IDC_SHOESIZE, m_shoesize);
	DDV_MinMaxInt(pDX, m_shoesize, 1, 99);
	DDX_Check(pDX, IDB_RESTART, m_restart);
	//}}AFX_DATA_MAP
	}							// CShoeSizeProperty::DoDataExchange


BEGIN_MESSAGE_MAP(CShoeSizeProperty, CCoinstallerDialog)
	//{{AFX_MSG_MAP(CShoeSizeProperty)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

///////////////////////////////////////////////////////////////////////////////
// CShoeSizeProperty message handlers

BOOL CShoeSizeProperty::OnInitDialog() 
	{							// CShoeSizeProperty::OnInitDialog

	// Determine the current setting of the registry property before doing
	// the UpdateData call inside the base class.

	HKEY hkey = SetupDiOpenDevRegKey(m_infoset, m_did, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
	if (hkey)
		{						// key exists
		DWORD size = sizeof(m_shoesize);
		RegQueryValueEx(hkey, "ProgrammersShoeSize", NULL, NULL, (LPBYTE) &m_shoesize, &size);
		RegCloseKey(hkey);
		}						// key exists

	CCoinstallerDialog::OnInitDialog();
	return TRUE;
	}							// CShoeSizeProperty::OnInitDialog

BOOL CShoeSizeProperty::OnApply() 
	{							// CShoeSizeProperty::OnApply

	// Set the ProgrammersShoeSize parameter

	UpdateData();
	
	HKEY hkey = SetupDiOpenDevRegKey(m_infoset, m_did, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_ALL_ACCESS);
	if (hkey)
		{						// key exists
		RegSetValueEx(hkey, "ProgrammersShoeSize", NULL, REG_DWORD, (LPBYTE) &m_shoesize, sizeof(m_shoesize));
		RegCloseKey(hkey);
		}						// key exists

	// If the "restart" button is checked, arrange for a restart.

	if (m_restart)
		{						// arrange to restart device
		SP_DEVINSTALL_PARAMS dip = {sizeof(SP_DEVINSTALL_PARAMS)};
		if (SetupDiGetDeviceInstallParams(m_infoset, m_did, &dip))
			{					// change parameters
			dip.FlagsEx |= DI_FLAGSEX_PROPCHANGE_PENDING;
			//dip.Flags |= DI_PROPERTIES_CHANGE | DI_NEEDREBOOT;	// if your device needs a reboot at this point
			SetupDiSetDeviceInstallParams(m_infoset, m_did, &dip);
			}					// change parameters
		}						// arrange to restart device

	return CCoinstallerDialog::OnApply();
	}							// CShoeSizeProperty::OnApply
