// SAMCLS16.CPP -- 16-bit Property Page Provider DLL for WDM samples (Win98 version)
// Copyright (C) 1999 by Walter Oney
// All rights reserved
               
extern "C" {
           
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <setupx.h>
#include <string.h>

#define EXPORT __export

#define Not_VxD   // to get ring-3 dcls
#include <vmm.h>
#define MIDL_PASS  // suppress 32-bit only #pragma pack(push)
#include <configmg.h>

}

#include "resource.h"

BOOL WINAPI EXPORT PageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);
UINT CALLBACK PageCallbackProc(HWND junk, UINT msg, LPPROPSHEETPAGE p);

typedef struct {UINT id; DWORD flag;} IDFLAG;

struct SETUPSTUFF {
	LPDEVICE_INFO pdi;
	DEVNODE devnode;
	char infopath[MAX_PATH];
	};

typedef SETUPSTUFF FAR* LPSETUPSTUFF;

///////////////////////////////////////////////////////////////

extern "C" BOOL WINAPI EXPORT EnumPropPages(LPDEVICE_INFO pdi, LPFNADDPROPSHEETPAGE AddPage, LPARAM lParam)
    {    // EnumPropPages
    PROPSHEETPAGE page; // status property page
    HPROPSHEETPAGE hpage;

	// Initialize a property page

    page.dwSize = sizeof(PROPSHEETPAGE);
    _asm mov page.hInstance, ds
    page.pszTemplate = MAKEINTRESOURCE(IDD_SAMPAGE);
    page.hIcon = NULL;
    page.pfnDlgProc = (DLGPROC) PageDlgProc;

	// Create an auxiliary data structure to pass important information
	// to our WM_INITDIALOG handler. Arrange for a callback when the page
	// is destroyed (the only event we actually care about) to delete this
	// structure

	LPSETUPSTUFF stuff = new SETUPSTUFF;
	if (!stuff)
		return TRUE;			// i.e., display the property sheet

	stuff->devnode = pdi->dnDevnode;
	stuff->pdi = pdi;

    page.lParam = (LPARAM) stuff;
    page.pfnCallback = PageCallbackProc;
	page.dwFlags = PSP_USECALLBACK;

	// Create a property page and add it to the device manager's property sheet

    hpage = CreatePropertySheetPage(&page);
    if (!hpage)
		{
		delete stuff;
		return TRUE;
		}

    if (!(*AddPage)(hpage, lParam))
        DestroyPropertySheetPage(hpage);
    
	return TRUE;
    }                           // EnumPropPages

///////////////////////////////////////////////////////////////

BOOL WINAPI EXPORT PageDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
    {                           // PageDlgProc
	LPSETUPSTUFF stuff = (LPSETUPSTUFF) GetWindowLong(hdlg, DWL_USER);

    switch (msg)
    	{						// process message
    	
    case WM_INITDIALOG:
    	{						// WM_INITDIALOG 
		stuff = (LPSETUPSTUFF) ((LPPROPSHEETPAGE) lParam)->lParam;
    	DEVNODE devnode = stuff->devnode;
    	SetWindowLong(hdlg, DWL_USER, (LONG) stuff);
    	CONFIGRET cr;
    	char name[256];                                                                        
    	ULONG length = sizeof(name);
    	
    	// Determine the friendly name or description for this device
		    	                                                                        
    	cr = CM_Read_Registry_Value(devnode, NULL, "FriendlyName", REG_SZ, name, &length, CM_REGISTRY_HARDWARE);
    	if (cr != CR_SUCCESS)
			{
			length = sizeof(name);
    		cr = CM_Read_Registry_Value(devnode, NULL, "DeviceDesc", REG_SZ, name, &length, CM_REGISTRY_HARDWARE);
			}

    	if (cr != CR_SUCCESS)
    		name[0] = 0;
    	
    	SetDlgItemText(hdlg, IDC_SAMNAME, name);

		// Set the class icon

		HWND hClassIcon = GetDlgItem(hdlg, IDC_CLASSICON);
		HICON hIcon;
		if (DiLoadClassIcon(stuff->pdi->szClassName, &hIcon, NULL) == 0)
			SendMessage(hClassIcon, STM_SETICON, (WPARAM) (HANDLE) hIcon, 0);
		else
			ShowWindow(hClassIcon, SW_HIDE);
    	
    	// Look for a SampleInfo entry in the registry
    	
    	length = sizeof(name);
    	cr = CM_Read_Registry_Value(devnode, NULL, "SampleInfo", REG_SZ, name, &length, CM_REGISTRY_HARDWARE);
    	if (cr == CR_SUCCESS)
    		{					// save SampleInfo
    		DoEnvironmentSubst(name, sizeof(name));
			int len = lstrlen(name);
			if (len >= sizeof(stuff->infopath))
				len = sizeof(stuff->infopath) - 1;
			memcpy(stuff->infopath, name, len);
			stuff->infopath[len] = 0;
    		}					// save SampleInfo
		else
			{					// no SampleInfo
			stuff->infopath[0] = 0;
			ShowWindow(GetDlgItem(hdlg, IDC_INFOLABEL), SW_HIDE);
			ShowWindow(GetDlgItem(hdlg, IDB_MOREINFO), SW_HIDE);
			}					// no SampleInfo
			    		
    	break;
    	}						// WM_INITDIALOG
    	
    case WM_COMMAND:
    	switch (LOWORD(wParam))
    		{					// process command notification
    		
    	case IDB_MOREINFO:
    		{					// IDB_MOREINFO 
    		ShellExecute(hdlg, NULL, stuff->infopath, NULL, NULL, SW_SHOWNORMAL);
    		return TRUE;
    		}					// IDB_MOREINFO
    		
    		}					// process command notification
		break;
    	
    	}						// process message
    	
    return FALSE;
    }                           // StatusDlgProc

///////////////////////////////////////////////////////////////////////////////

UINT CALLBACK PageCallbackProc(HWND junk, UINT msg, LPPROPSHEETPAGE p)
	{							// PageCallbackProc
	if (msg == PSPCB_RELEASE && p->lParam)
		{
		LPSETUPSTUFF stuff = (LPSETUPSTUFF) p->lParam;
		delete stuff;
		}
	return TRUE;				// matters only for PSPCB_CREATE
	}							// PageCallbackProc
        