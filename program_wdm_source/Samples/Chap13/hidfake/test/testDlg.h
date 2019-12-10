// testDlg.h : header file
//

#if !defined(AFX_TESTDLG_H__B66FDBD9_AACB_470B_9519_890A3D632F50__INCLUDED_)
#define AFX_TESTDLG_H__B66FDBD9_AACB_470B_9519_890A3D632F50__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "DeviceList.h"

extern "C" {			

#include <hidsdi.h>					// from the DDK
	}
#pragma comment(lib, "hid")

#include "..\sys\FeatureReports.h"	// from the driver project

/////////////////////////////////////////////////////////////////////////////
// CTestDlg dialog

class CTestDlg : public CDialog
{
// Construction
public:
	CTestDlg(CWnd* pParent = NULL);	// standard constructor
	~CTestDlg();

// Dialog Data
	//{{AFX_DATA(CTestDlg)
	enum { IDD = IDD_TEST_DIALOG };
	CStatic	m_ctlUp;
	CStatic	m_ctlDown;
	CString	m_version;
	int		m_buttonsetting;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTestDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;
	HANDLE m_hDevice;
	UCHAR m_ButtonDown;

	HANDLE FindFakeDevice();
	VOID GetButtonState();
	VOID SetButtonState(BOOLEAN down);

	// Generated message map functions
	//{{AFX_MSG(CTestDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnDown();
	afx_msg void OnUp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TESTDLG_H__B66FDBD9_AACB_470B_9519_890A3D632F50__INCLUDED_)
