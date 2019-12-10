// ShoeSize.cpp : implementation file
//

#include "stdafx.h"
#include "SampleCoinstaller.h"
#include "ShoeSize.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
// CShoeSize property page

IMPLEMENT_DYNAMIC(CShoeSize, CCoinstallerDialog)

CShoeSize::CShoeSize() : CCoinstallerDialog(IDD, 0, IDS_SHOETITLE, IDS_SHOESUBTITLE)
	{							// CShoeSize::CShoeSize
	//{{AFX_DATA_INIT(CShoeSize)
	m_shoesize = 1;
	//}}AFX_DATA_INIT
	}							// CShoeSize::CShoeSize

CShoeSize::~CShoeSize()
	{							// CShoeSize::~CShoeSize
	}							// CShoeSize::~CShoeSize

void CShoeSize::DoDataExchange(CDataExchange* pDX)
	{							// CShoeSize::DoDataExchange
	CCoinstallerDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CShoeSize)
	DDX_Text(pDX, IDC_SHOESIZE, m_shoesize);
	DDV_MinMaxInt(pDX, m_shoesize, 1, 99);
	//}}AFX_DATA_MAP
	}							// CShoeSize::DoDataExchange


BEGIN_MESSAGE_MAP(CShoeSize, CCoinstallerDialog)
	//{{AFX_MSG_MAP(CShoeSize)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

///////////////////////////////////////////////////////////////////////////////
// CShoeSize message handlers. Note that we don't get the wizard-finish message
// for some reason, so we have to do our updating work at wizard-next time

BOOL CShoeSize::OnSetActive() 
	{							// CShoeSize::OnSetActive

	// If the device exists already, determine the current setting of
	// the ProgrammersShoeSize parameter

	HKEY hkey = SetupDiOpenDevRegKey(m_infoset, m_did, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
	if (hkey)
		{						// key exists
		DWORD size = sizeof(m_shoesize);
		RegQueryValueEx(hkey, "ProgrammersShoeSize", NULL, NULL, (LPBYTE) &m_shoesize, &size);
		RegCloseKey(hkey);
		UpdateData(FALSE);
		}						// key exists

	return CCoinstallerDialog::OnSetActive();
	}							// CShoeSize::OnSetActive

LRESULT CShoeSize::OnWizardNext() 
	{							// CShoeSize::OnWizardNext

	// Set the ProgrammersShoeSize parameter

	UpdateData();
	
	HKEY hkey = SetupDiOpenDevRegKey(m_infoset, m_did, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_ALL_ACCESS);
	if (hkey)
		{						// key exists
		RegSetValueEx(hkey, "ProgrammersShoeSize", NULL, REG_DWORD, (LPBYTE) &m_shoesize, sizeof(m_shoesize));
		RegCloseKey(hkey);
		}						// key exists
	
	return CCoinstallerDialog::OnWizardNext();
	}							// CShoeSize::OnWizardNext
