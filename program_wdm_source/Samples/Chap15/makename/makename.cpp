// MAKENAME.CPP -- Inject or remove filter driver for a specified device
// Copyright (C) 1999 by Walter Oney
// All rights reserved


#include "stdafx.h"

LPTSTR* CommandLineToArgv(LPTSTR cmdline, int* pargc);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
	{							// DllMain
    return TRUE;
	}							// DllMain

///////////////////////////////////////////////////////////////////////////////

extern "C" void WINAPI MakeName(HWND hwnd, HINSTANCE hInst, LPCTSTR rawline, int nshow)
	{							// MakeName
	HDEVINFO hinfoset = INVALID_HANDLE_VALUE;

	// Determine which platform we're running on. You'll see why a bit later...

	OSVERSIONINFO vi = {sizeof(OSVERSIONINFO)};
	GetVersionEx(&vi);
	BOOL win98 = vi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS;

	// Parse arguments. We expect to be called this way:
	// rundll32 MAKENAME,MakeName devname "FriendlyNameTemplate"
	//
	// E.g.: rundll32 makename,makename "Acme USB Toaster" "Acme Toaster (%d)"

	int argc;
	LPTSTR cmdline = _tcsdup(rawline);
	LPTSTR* argv = CommandLineToArgv(cmdline, &argc);
	if (!argv)
		{
		free(cmdline);
		return;
		}

	LPTSTR devname = NULL;
	LPTSTR friendlyname = NULL;

	for (int iarg = 0; iarg < argc; ++iarg)
		{						// inspect arguments
		LPTSTR arg = argv[iarg];

		if (!devname)
			devname = arg;
		else if (!friendlyname)
			friendlyname = arg;
		else
			goto badargs;
		}						// inspect arguments

	if (!devname || !friendlyname)
		goto badargs;			// didn't get both required parameters

	// Use SetupDi functions to locate each instance of the specified device

	hinfoset = SetupDiGetClassDevs(NULL, NULL, hwnd, DIGCF_PRESENT | DIGCF_ALLCLASSES);
	if (hinfoset == INVALID_HANDLE_VALUE)
		goto error;

	{							// local scope
	SP_DEVINFO_DATA devinfo = {sizeof(SP_DEVINFO_DATA)};
	SP_DEVINFO_DATA targinfo;
	BOOL found = FALSE;
	int index = 1;

	for (DWORD devindex = 0; SetupDiEnumDeviceInfo(hinfoset, devindex, &devinfo); ++devindex)
		{						// for each device
		DWORD junk;
		TCHAR value[512];
		if (!SetupDiGetDeviceRegistryProperty(hinfoset, &devinfo, SPDRP_DEVICEDESC, &junk, (PBYTE) value, sizeof(value), NULL)
			|| _tcsicmp((LPCTSTR) value, devname) != 0)
			continue;			// not this device

		// If device already has a friendly name, count it so as to generate a unique index. Otherwise, remember it as the one
		// whose name we're eventually going to set.

		if (SetupDiGetDeviceRegistryProperty(hinfoset, &devinfo, SPDRP_FRIENDLYNAME, &junk, (PBYTE) value, sizeof(value), NULL))
			++index;
		else
			targinfo = devinfo, found = TRUE;
		}						// for each device

	// Compose a new friendly name and install it in the registry

	if (found)
		{						// found target device
		size_t nchars = _tcslen(friendlyname) + 20;
		LPTSTR newname = (LPTSTR) malloc(nchars * sizeof(WCHAR));
		if (!newname)
			goto error;
		_sntprintf(newname, nchars, friendlyname, index);
		nchars = _tcslen(newname);
		SetupDiSetDeviceRegistryProperty(hinfoset, &targinfo, SPDRP_FRIENDLYNAME, (PBYTE) newname, nchars * sizeof(WCHAR));
		free(newname);
		}						// found target device
	}							// local scope

cleanup:
	if (hinfoset != INVALID_HANDLE_VALUE)
		SetupDiDestroyDeviceInfoList(hinfoset);
	free(argv);
	free(cmdline);
	return;

badargs:
	OutputDebugString(_T("MakeName - invalid arguments\n"));
	goto cleanup;

error:
	OutputDebugString(_T("MakeName - error trying to define new friendly name\n"));
	goto cleanup;
	}							// MakeName

///////////////////////////////////////////////////////////////////////////////
// Note -- CommandLineToArgvW exists already, but not the ANSI equivalent @&*%!

LPTSTR* CommandLineToArgv(LPTSTR cmdline, int* pargc)
	{							// CommandLineToArgv
	int argc = 0;
	while (*cmdline == _T(' ') || *cmdline == _T('\t'))
		++cmdline;				// skip leading blanks

	LPTSTR arg = cmdline;
	while (*arg)
		{						// count arguments
		TCHAR ch;
		while ((ch = *arg) == _T(' ') || ch == _T('\t'))
			*arg++ = _T(' ');	// skip leading blanks, replace tabs with blanks
		if (!ch)
			break;				// end of command line
		
		++argc;
		
		if (ch == _T('\"'))
			{					// quoted arg
			*arg++ = _T(' ');	// replace leading quote with space
			while ((ch = *arg) && ch != _T('\"'))
				++arg;			// find trailing quote
			}					// quoted arg
		else
			{					// unquoted arg
			while ((ch = *arg) && ch != _T(' ') && ch != _T('\t'))
				++arg;			// find space after arg
			}					// unquoted arg

		if (ch)
			*arg++ = 0;			// change ending delimiter to nul
		}						// count arguments

	if (!argc)
		return NULL;			// no arguments

	LPTSTR* argv = (LPTSTR*) malloc(argc * sizeof(LPTSTR));
	if (!argv)
		return NULL;			// can't allocate memory

	arg = cmdline;
	for (int iarg = 0; iarg < argc; ++iarg)
		{						// fill in argv array
		TCHAR ch;
		while ((ch = *arg) == _T(' '))
			++arg;				// skip to start of arg
		argv[iarg] = arg;
		arg += _tcslen(arg) + 1;	// skip to end of arg
		}						// fill in argv array

	*pargc = argc;
	return argv;
	}							// CommandLineToArgv
