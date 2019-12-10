#if !defined(AFX_SHOESIZEPROPERTY_H__225884F2_F797_49BB_87F8_AEB8589E6E04__INCLUDED_)
#define AFX_SHOESIZEPROPERTY_H__225884F2_F797_49BB_87F8_AEB8589E6E04__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ShoeSizeProperty.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CShoeSizeProperty dialog

class CShoeSizeProperty : public CCoinstallerDialog
{
	DECLARE_DYNAMIC(CShoeSizeProperty)

// Construction
public:
	CShoeSizeProperty();
	~CShoeSizeProperty();

// Dialog Data
	//{{AFX_DATA(CShoeSizeProperty)
	enum { IDD = IDD_SHOESIZEPROP };
	int		m_shoesize;
	BOOL	m_restart;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CShoeSizeProperty)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CShoeSizeProperty)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SHOESIZEPROPERTY_H__225884F2_F797_49BB_87F8_AEB8589E6E04__INCLUDED_)
