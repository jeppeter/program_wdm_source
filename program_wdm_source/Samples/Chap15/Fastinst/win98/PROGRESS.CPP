// progress.cpp : implementation file
//

#include "stdafx.h"
#include "progress.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProgressDlg dialog


CProgressDlg::CProgressDlg()
	: CDialog()
{
	//{{AFX_DATA_INIT(CProgressDlg)
	m_caption = "";
	//}}AFX_DATA_INIT
}

void CProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProgressDlg)
	DDX_Text(pDX, IDC_CAPTION, m_caption);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CProgressDlg, CDialog)
	//{{AFX_MSG_MAP(CProgressDlg)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CProgressDlg message handlers

BOOL CProgressDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	CenterWindow();
	m_hwndGauge = ::GetDlgItem(m_hWnd, IDC_PROGRESS);
	ResetGauge(m_hwndGauge, 100);
	SetGauge(m_hwndGauge, 0);
	m_pos = 0;
	
	SetTimer(1, 100, NULL);	// update progress bar every 1/10 second
	
	return TRUE;
}

void CProgressDlg::OnTimer(UINT nIDEvent)
{
	m_pos += 5;
	if (m_pos >= 100)
		m_pos = 0;
	SetGauge(m_hwndGauge, m_pos);
}
    
BOOL CProgressDlg::DoModeless(CWnd* parent /* = NULL */)
{
	return Create(IDD, parent);
}
    