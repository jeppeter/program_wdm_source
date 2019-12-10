// testDlg.cpp : implementation file
//

#include "stdafx.h"
#include "test.h"
#include "testDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
	{
	public:
		CAboutDlg();
		
		// Dialog Data
		//{{AFX_DATA(CAboutDlg)
		enum { IDD = IDD_ABOUTBOX };
		//}}AFX_DATA
		
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CAboutDlg)
	protected:
		virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
		//}}AFX_VIRTUAL
		
		// Implementation
	protected:
		//{{AFX_MSG(CAboutDlg)
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
	};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
	{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
	}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
	{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
	}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
//{{AFX_MSG_MAP(CAboutDlg)
// No message handlers
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTestDlg dialog

CTestDlg::CTestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTestDlg::IDD, pParent)
	{
	//{{AFX_DATA_INIT(CTestDlg)
	m_version = _T("");
	m_buttonsetting = -1;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_hDevice = INVALID_HANDLE_VALUE;
	}

CTestDlg::~CTestDlg()
	{							// CTestDlg::~CTestDlg
	if (m_hDevice != INVALID_HANDLE_VALUE)
		CloseHandle(m_hDevice);
	}							// CTestDlg::~CTestDlg

void CTestDlg::DoDataExchange(CDataExchange* pDX)
	{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTestDlg)
	DDX_Control(pDX, IDC_UP, m_ctlUp);
	DDX_Control(pDX, IDC_DOWN, m_ctlDown);
	DDX_Text(pDX, IDC_VERSION, m_version);
	DDX_Radio(pDX, IDB_DOWN, m_buttonsetting);
	//}}AFX_DATA_MAP
	}

BEGIN_MESSAGE_MAP(CTestDlg, CDialog)
//{{AFX_MSG_MAP(CTestDlg)
ON_WM_SYSCOMMAND()
ON_WM_PAINT()
ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDB_DOWN, OnDown)
	ON_BN_CLICKED(IDB_UP, OnUp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

///////////////////////////////////////////////////////////////////////////////

HANDLE CTestDlg::FindFakeDevice()
	{							// CTestDlg::FindFakeDevice

	// Enumerate HID devices

	GUID hidguid;
	HidD_GetHidGuid(&hidguid);
	CDeviceList devlist(hidguid);
	int ndevices = devlist.Initialize();

	// Search for a HIDFAKE device by opening each HID device in turn and
	// examining its attributes.

	for (int i = 0; i < ndevices; ++i)
		{						// find HIDFAKE device

		// When opening the handle to perform queries, we don't need either read
		// or write access, we must specify R/W sharing, and we can't use the
		// overlapped flag.

		HANDLE h = CreateFile(devlist.m_list[i].m_linkname, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, 0, NULL);

		if (h == INVALID_HANDLE_VALUE)
			continue;			// can't open this one -- try the next

		HIDD_ATTRIBUTES attr = {sizeof(HIDD_ATTRIBUTES)};
		BOOLEAN okay = HidD_GetAttributes(h, &attr);
		CloseHandle(h);

		if (!okay)
			continue;			// couldn't get attributes, so skip it

		if (attr.VendorID != HIDFAKE_VID || attr.ProductID != HIDFAKE_PID)
			continue;			// not our device, so skip it

		// Open a handle for reading and writing.

		return CreateFile(devlist.m_list[i].m_linkname, GENERIC_READ | GENERIC_WRITE, 0,
			NULL, OPEN_EXISTING, 0, NULL);
		}						// find HIDFAKE device

	return INVALID_HANDLE_VALUE;
	}							// CTestDlg::FindFakeDevice

///////////////////////////////////////////////////////////////////////////////

VOID CTestDlg::GetButtonState()
	{							// CTestDlg::GetButtonState
	UCHAR buffer[2];
	DWORD junk;
	ReadFile(m_hDevice, buffer, 2, &junk, NULL);
	m_ButtonDown = buffer[1];

	m_ctlDown.ShowWindow(m_ButtonDown ? SW_SHOW : SW_HIDE);
	m_ctlUp.ShowWindow(m_ButtonDown ? SW_HIDE : SW_SHOW);
	}							// CTestDlg::GetButtonState

///////////////////////////////////////////////////////////////////////////////

BOOL CTestDlg::OnInitDialog()
	{
	CDialog::OnInitDialog();
	
	// Add "About..." menu item to system menu.
	
	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);
	
	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
		{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
			{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
			}
		}
	
	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	m_hDevice = FindFakeDevice();			// locate the HIDFAKE device

	if (m_hDevice == INVALID_HANDLE_VALUE)
		AfxMessageBox(IDS_NODEVICES, MB_OK | MB_ICONHAND);
	else
		{						// initialize applet
		GetButtonState();		// get initial state
		m_buttonsetting = m_ButtonDown ? 0 : 1;

		// Issue GetFeature request to determine driver version

		FEATURE_REPORT_GET_VERSION v;
		v.id = FEATURE_CODE_GET_VERSION;

		if (HidD_GetFeature(m_hDevice, &v, sizeof(v)))
			m_version.Format("%d.%2.2d", HIWORD(v.Version), LOWORD(v.Version));

		UpdateData(FALSE);
		}						// initialize applet
	
	return TRUE;  // return TRUE  unless you set the focus to a control
	}

///////////////////////////////////////////////////////////////////////////////
// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CTestDlg::OnPaint() 
	{
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
		{
		CDialog::OnPaint();
		}
	}

///////////////////////////////////////////////////////////////////////////////
// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.

HCURSOR CTestDlg::OnQueryDragIcon()
	{
	return (HCURSOR) m_hIcon;
	}

///////////////////////////////////////////////////////////////////////////////

void CTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
	{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
		{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
		}
	else
		{
		CDialog::OnSysCommand(nID, lParam);
		}
	}

///////////////////////////////////////////////////////////////////////////////

void CTestDlg::OnDown() 
	{							// CTestDlg::OnDown
	SetButtonState(TRUE);
	}							// CTestDlg::OnDown

void CTestDlg::OnUp() 
	{							// CTestDlg::OnUp
	SetButtonState(FALSE);
	}							// CTestDlg::OnUp

///////////////////////////////////////////////////////////////////////////////

VOID CTestDlg::SetButtonState(BOOLEAN down)
	{							// CTestDlg::SetButtonState
	FEATURE_REPORT_SET_BUTTON sb;
	sb.id = FEATURE_CODE_SET_BUTTON;
	sb.ButtonDown = down;

	HidD_SetFeature(m_hDevice, &sb, sizeof(sb));

	GetButtonState();
	}							// CTestDlg::SetButtonState
