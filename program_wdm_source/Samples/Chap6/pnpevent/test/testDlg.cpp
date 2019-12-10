// testDlg.cpp : implementation file
// Copyright (C) 1999 by Walter Oney
// All rights reserved

// This is the interesting program in the PNPEVENT sample, wherein I illustrate how
// a user-mode application tracks the PnP events associated with arrival and departure
// of devices. The key points here are these:
//
//	1.	At WM_INITDIALOG time, we register for notification of events involving an
//		interface GUID that our hardware (actually the PNPEVENT sample driver) uses.
//		We also call EnumerateExistingDevices to locate all existing PNPEVENT devices
//		so we can treat them all as having just arrived. (In general, this is what you'd
//		want to do. However, to keep this sample as simple as possible, we actually
//		ignore the 2d and subsequent instances of the PNPEVENT device.)
//
//	2.	We handle the WM_DEVICECHANGE message, which is how the system packages notifications
//		about hardware devices. We handle the various notifications as follows (this is the
//		order in which they'd occur when you run the test suggested in PNPEVENT.HTM):
//
//		a.	DBT_DEVICEARRIVAL for the interface: call OnNewDevice to display a message and
//			open a handle to the device. In addition, register to receive notifications
//			about this handle.
//
//		b.	DBT_DEVIQEQUERYREMOVE for the handle: ask permission to remove the device. If
//			this fails, we return a code that is supposed to block removal of the device
//			(but doesn't actually, in my experience).
//
//		c.	DBT_DEVICEREMOVECOMPLETE and DBT_DEVICEREMOVEPENDING for the handle: In
//			Windows 2000 or XP, unregister the handle notification. Doing this here would
//			destabilize Windows 98 and maybe 98SE or ME. Then close the handle
//
//		d.	DBT_DEVICEREMOVECOMPLETE for the interface: display a message.

#include "stdafx.h"
#include "test.h"
#include "testDlg.h"

typedef DWORD ULONG_PTR;
typedef DWORD DWORD_PTR;
#include "setupapi.h"
#pragma comment(lib, "setupapi")

#define _SYS_GUID_OPERATORS_
#include <initguid.h>
#include "..\sys\guids.h"
#include <winioctl.h>
#include "..\sys\ioctls.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL win98;

///////////////////////////////////////////////////////////////////////////////
// CTestDlg dialog

CTestDlg::CTestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTestDlg::IDD, pParent)
	{							// CTestDlg::CTestDlg
	//{{AFX_DATA_INIT(CTestDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_hInterfaceNotification = NULL;
	m_hHandleNotification = NULL;
	m_hDevice = INVALID_HANDLE_VALUE;

	OSVERSIONINFO vi = {sizeof(OSVERSIONINFO)};
	GetVersionEx(&vi);
	win98 = vi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS;
	}							// CTestDlg::CTestDlg

CTestDlg::~CTestDlg()
	{							// CTestDlg::CTestDlg
	if (m_hDevice != INVALID_HANDLE_VALUE)
		CloseHandle(m_hDevice);
	}							// CTestDlg::CTestDlg

void CTestDlg::DoDataExchange(CDataExchange* pDX)
	{							// CTestDlg::DoDataExchange
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTestDlg)
	DDX_Control(pDX, IDB_SENDEVENT, m_ctlSendevent);
	DDX_Control(pDX, IDC_EVENTS, m_ctlEvents);
	//}}AFX_DATA_MAP
	}							// CTestDlg::DoDataExchange

BEGIN_MESSAGE_MAP(CTestDlg, CDialog)
	//{{AFX_MSG_MAP(CTestDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDB_SENDEVENT, OnSendevent)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_WM_DEVICECHANGE()
END_MESSAGE_MAP()

///////////////////////////////////////////////////////////////////////////////

VOID CTestDlg::EnumerateExistingDevices(const GUID* guid)
	{							// CTestDlg::EnumerateExistingDevices
	HDEVINFO info = SetupDiGetClassDevs((GUID*) guid, NULL, NULL, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);
	if (info == INVALID_HANDLE_VALUE)
		return;

	SP_INTERFACE_DEVICE_DATA ifdata;
	ifdata.cbSize = sizeof(ifdata);
	DWORD devindex;
	for (devindex = 0; SetupDiEnumDeviceInterfaces(info, NULL, (GUID*) guid, devindex, &ifdata); ++devindex)
		{						// for each device
		DWORD needed;
		SetupDiGetDeviceInterfaceDetail(info, &ifdata, NULL, 0, &needed, NULL);

		PSP_INTERFACE_DEVICE_DETAIL_DATA detail = (PSP_INTERFACE_DEVICE_DETAIL_DATA) malloc(needed);
		if (!detail)
			continue;
		detail->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
		if (!SetupDiGetDeviceInterfaceDetail(info, &ifdata, detail, needed, NULL, NULL))
			{						// can't get detail info
			free((PVOID) detail);
			continue;
			}						// can't get detail info

		CString devname = detail->DevicePath;
		free((PVOID) detail);
		OnNewDevice(devname, guid);
		}						// for each device

	SetupDiDestroyDeviceInfoList(info);
	}							// CTestDlg::EnumerateExistingDevices

///////////////////////////////////////////////////////////////////////////////

BOOL CTestDlg::HandleDeviceChange(DWORD evtype, PDEV_BROADCAST_DEVICEINTERFACE dip)
	{							// CTestDlg::HandleDeviceChange
	CString devname = dip->dbcc_name;

	switch (evtype)
		{						// process device interface notification

	case DBT_DEVICEARRIVAL:
		OnNewDevice(devname, &dip->dbcc_classguid);
		break;

	case DBT_DEVICEREMOVECOMPLETE:
		{						// DBT_DEVICEREMOVECOMPLETE
		CString msg;
		msg.Format(_T("Device %s removed"), (LPCTSTR) devname);
		m_ctlEvents.AddString(msg);
		break;
		}						// DBT_DEVICEREMOVECOMPLETE

		}						// process device interface notification

	return TRUE;
	}							// CTestDlg::HandleDeviceChange

///////////////////////////////////////////////////////////////////////////////


BOOL CTestDlg::HandleDeviceChange(DWORD evtype, PDEV_BROADCAST_HANDLE dhp)
	{							// CTestDlg::HandleDeviceChange

	if (dhp->dbch_handle != m_hDevice)
		return TRUE;			// notification for some other handle

	switch (evtype)
		{						// process handle notification

	case DBT_DEVICEQUERYREMOVE:
		if (MessageBox(_T("Okay to remove the device?"), _T("Removal Query"), MB_YESNO) != IDYES)
			return BROADCAST_QUERY_DENY;

		// Note that we fall through there because we may not get additional notifications

	case DBT_DEVICEREMOVECOMPLETE:
	case DBT_DEVICEREMOVEPENDING:
		MessageBox(_T("Closing Handle"));

		// Deregistering the notification handle here will destablize Win98. Reader
		// Fred Chumaceiro discovered that it's okay to deregister outside the context
		// of a WM_DEVICECHANGE that refers to the same notification handle, though.

		if (m_hHandleNotification && !win98)
			{					// deregister notification handle
			UnregisterDeviceNotification(m_hHandleNotification);
			m_hHandleNotification = NULL;
			}					// deregister notification handle

		if (m_hDevice != INVALID_HANDLE_VALUE)
			CloseHandle(m_hDevice);
		m_hDevice = INVALID_HANDLE_VALUE;
		
		m_ctlSendevent.EnableWindow(FALSE);
		break;
		
		}						// process handle notification

	return TRUE;
	}							// CTestDlg::HandleDeviceChange

///////////////////////////////////////////////////////////////////////////////

void CTestDlg::OnDestroy() 
	{							// CTestDlg::OnDestroy
	// Per Fred Chumaceiro: it's safe to deregister the notification handles in Win98
	// while the window still exists.

	if (m_hInterfaceNotification)
		UnregisterDeviceNotification(m_hInterfaceNotification);
	if (m_hHandleNotification)
		UnregisterDeviceNotification(m_hHandleNotification);

	CDialog::OnDestroy();
	}							// CTestDlg::OnDestroy

///////////////////////////////////////////////////////////////////////////////

BOOL CTestDlg::OnDeviceChange(UINT nEventType, DWORD dwData)
	{							// CTestDlg::OnDeviceChange
	if (!dwData)
		return TRUE;

	_DEV_BROADCAST_HEADER* p = (_DEV_BROADCAST_HEADER*) dwData;

	if (p->dbcd_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
		return HandleDeviceChange(nEventType, (PDEV_BROADCAST_DEVICEINTERFACE) p);
	else if (p->dbcd_devicetype == DBT_DEVTYP_HANDLE)
		return HandleDeviceChange(nEventType, (PDEV_BROADCAST_HANDLE) p);
	else
		return TRUE;
	}							// CTestDlg::OnDeviceChange

///////////////////////////////////////////////////////////////////////////////

BOOL CTestDlg::OnInitDialog()
	{							// CTestDlg::OnInitDialog
	CDialog::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	DEV_BROADCAST_DEVICEINTERFACE filter;
	filter.dbcc_size = sizeof(filter);
	filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	filter.dbcc_classguid = GUID_DEVINTERFACE_PNPEVENT;
	m_hInterfaceNotification = RegisterDeviceNotification(m_hWnd, &filter, 0);

	EnumerateExistingDevices(&GUID_DEVINTERFACE_PNPEVENT);
	
	return TRUE;  // return TRUE  unless you set the focus to a control
	}							// CTestDlg::OnInitDialog

///////////////////////////////////////////////////////////////////////////////

VOID CTestDlg::OnNewDevice(const CString& devname, const GUID* guid)
	{							// CTestDlg::OnNewDevice
	CString msg;

	if (m_hDevice != INVALID_HANDLE_VALUE)
		{						// only supports 1 instance
		msg.Format(_T("This test program can only handle one instance of PNPEVENT at a time.\n"
			"Therefore, the arrival of a new instance named \n%s is being ignored."),
			(LPCTSTR) devname);
		MessageBox(msg, "Warning", MB_OK | MB_ICONEXCLAMATION);
		return;
		}						// only supports 1 instance

	msg.Format(_T("Device %s arrived"), (LPCTSTR) devname);
	m_ctlEvents.AddString(msg);

	msg = devname;

	msg.SetAt(2, _T('.'));

	m_hDevice = CreateFile(msg, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (m_hDevice == INVALID_HANDLE_VALUE)
		{
		MessageBox("Unable to open handle to device", "Error", MB_OK | MB_ICONHAND);
		return;
		}

	// Per Fred Chumaceiro: it should be safe to deregister a stale notification handle
	// now, since the current WM_DEVICECHANGE doesn't pertain to it.

	if (m_hHandleNotification)
		UnregisterDeviceNotification(m_hHandleNotification);

	DEV_BROADCAST_HANDLE filter = {0};
	filter.dbch_size = sizeof(filter);
	filter.dbch_devicetype = DBT_DEVTYP_HANDLE;
	filter.dbch_handle = m_hDevice;
	m_hHandleNotification = RegisterDeviceNotification(m_hWnd, &filter, 0);
	
	m_ctlSendevent.EnableWindow(TRUE);
	}							// CTestDlg::OnNewDevice

///////////////////////////////////////////////////////////////////////////////
// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CTestDlg::OnPaint() 
	{							// CTestDlg::OnPaint
	if (IsIconic())
		{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
		}
	else
		CDialog::OnPaint();
	}							// CTestDlg::OnPaint

///////////////////////////////////////////////////////////////////////////////

HCURSOR CTestDlg::OnQueryDragIcon()
	{							// CTestDlg::OnQueryDragIcon
	return (HCURSOR) m_hIcon;
	}							// CTestDlg::OnQueryDragIcon

///////////////////////////////////////////////////////////////////////////////

void CTestDlg::OnSendevent() 
	{							// CTestDlg::OnSendevent
	if (m_hDevice == INVALID_HANDLE_VALUE)
		return;
	DWORD junk;
	DeviceIoControl(m_hDevice, IOCTL_GENERATE_EVENT, NULL, 0, NULL, 0, &junk, NULL);
	}							// CTestDlg::OnSendevent
