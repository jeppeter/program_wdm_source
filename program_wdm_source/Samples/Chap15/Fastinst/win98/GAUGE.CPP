// gauge.cpp : implementation of progress gauge window
//

#include "stdafx.h"
#include "gauge.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

static DWORD bgColor;			// background color
static DWORD fgColor;			// foreground color
static BOOL fMono;				// true if monochrome monitor

#define BAR_POS 0				// window word for bar position
#define BAR_FULL 2				// ditto for full bar


///////////////////////////////////////////////////////////////////////////////
// Window function

LRESULT EXPORT WINAPI GaugeWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{							// GaugeWndProc
	
	switch (msg)
		{						// select on message
		
	case WM_CREATE:
		ResetGauge(hwnd, 100);
		return 0;
		
	case WM_PAINT:
		{						// WM_PAINT
		CPaintDC dc(CWnd::FromHandle(hwnd));
		CRect rc;
		
		GetClientRect(hwnd, &rc);
		
		// Draw black frame around the progress bar

		CBrush BlackBrush;
		BlackBrush.CreateStockObject(BLACK_BRUSH);
		dc.FrameRect(rc, &BlackBrush);
		
		// Determine current and full positions; calculate percent full
		
		WORD pos = GetWindowWord(hwnd, BAR_POS);
		WORD full = GetWindowWord(hwnd, BAR_FULL);
		if (full == 0)
			full = 1;			// prevent divide by zero
		if (pos > full)
			pos = full;			// don't go past 100%
#if 1
		char* percent = "";
#else
		char percent[32];
		wsprintf(percent, "%3d%%", (WORD) ((DWORD) pos * 100 / full));
#endif
		CSize size = dc.GetTextExtent(percent, strlen(percent));
		
		// Calculate the done and undone rectangles
		
		rc.InflateRect(-1, -1);
		int dx = rc.right - rc.left;
		int dy = rc.bottom - rc.top;
		int x = (int) ((LONG) pos * dx / (LONG) full);
		CRect rcDone = rc;
		CRect rcUndone = rc;
		rcDone.right = rcUndone.left = rc.left + x;
		
		// Display the progress bar & percent-complete figures
		
		dc.SetBkColor(bgColor);
		dc.SetTextColor(fgColor);
		dc.ExtTextOut((dx - size.cx)/2, (dy - size.cy)/2, ETO_OPAQUE | ETO_CLIPPED, &rcDone, percent, strlen(percent), NULL);

		dc.SetBkColor(fgColor);
		dc.SetTextColor(bgColor);
		dc.ExtTextOut((dx - size.cx)/2, (dy - size.cy)/2, ETO_OPAQUE | ETO_CLIPPED, &rcUndone, percent, strlen(percent), NULL);
		
		return 0;
		}						// WM_PAINT
		
		}						// select on message
		
	return DefWindowProc(hwnd, msg, wParam, lParam);
	}							// GaugeWndProc
	
///////////////////////////////////////////////////////////////////////////////
// ResetGauge resets the gauge to register empty and to remember a new 100% value

void ResetGauge(HWND hwnd, WORD full)
	{							// ResetGauge
	SetWindowWord(hwnd, BAR_FULL, full);
	SetGauge(hwnd, 0);
	}							// ResetGauge

///////////////////////////////////////////////////////////////////////////////
// SetGauge adjusts the current position of the gauge

void SetGauge(HWND hwnd, WORD pos)
	{							// SetGauge
	SetWindowWord(hwnd, BAR_POS, pos);
	InvalidateRect(hwnd, NULL, FALSE);
	UpdateWindow(hwnd);			// force immediate repaint
	}							// SetGauge

///////////////////////////////////////////////////////////////////////////////
// RegisterGauge registers the GAUGE window class

BOOL RegisterGauge()
	{							// RegisterGauge
	WNDCLASS wc;
	
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "GAUGE";
	wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
	wc.hInstance = AfxGetInstanceHandle();
	wc.style = 0;
	wc.lpfnWndProc = GaugeWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 2 * sizeof(WORD);
	
	if (!RegisterClass(&wc))
		return FALSE;
	
	CClientDC dc(NULL);
	fMono = dc.GetDeviceCaps(NUMCOLORS) == 2;
	if (fMono)
		bgColor = RGB(0, 0, 0), fgColor = RGB(255, 255, 255);
	else
		bgColor = RGB(0, 0, 255), fgColor = RGB(255, 255, 255);
		
	return TRUE;
	}							// RegisterGauge
	