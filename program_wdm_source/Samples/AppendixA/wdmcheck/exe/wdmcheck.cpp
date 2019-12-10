// WdmCheck.cpp -- Check WDM module imports for Windows compatibility
// Copyright (C) 2000 by Walter Oney
// All rights reserved

#include "stdafx.h"
#include "ImageFile.h"
#include "WdmcheckHelper.h"
#include "..\vxd\ioctls.h"

#define arraysize(p) (sizeof(p)/sizeof((p)[0]))

BOOL CheckKernelExports(CImageFile* image, CWdmcheckHelper* helper, char* modname, PIMAGE_IMPORT_DESCRIPTOR imports, char*& misnames);
BOOL CheckOtherExports(CImageFile* image, char* modname, PIMAGE_IMPORT_DESCRIPTOR imports, char*& misnames);
DWORD ReportError(char* text, DWORD code);

///////////////////////////////////////////////////////////////////////////////

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nShow)
	{							// WinMain

	// Don't run this application in NT...

	OSVERSIONINFO verinfo = {sizeof(OSVERSIONINFO)};
	GetVersionEx(&verinfo);
	if (verinfo.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS
		|| verinfo.dwMajorVersion < 4
		|| verinfo.dwMinorVersion < 10)
		{						// wrong platform
		MessageBox(NULL, "WdmCheck is only useful on Windows 98 (or later) systems", "WdmCheck", MB_OK | MB_ICONINFORMATION);
		return 1;
		}						// wrong platform

	// Open the image file

	char filename[_MAX_PATH];
	if (*lpCmdLine == '"')
		{						// strip quotes from name
		strcpy(filename, lpCmdLine + 1);
		filename[strlen(filename) - 1] = 0;
		}						// strip quotes from name
	else
		strcpy(filename, lpCmdLine);

	CImageFile image(filename);
	if (!image.Open())
		{						// can't open
		ReportError(filename, GetLastError());
		return 1;
		}						// can't open

	// Load the VxD helper

	CWdmcheckHelper helper;
	if (!helper.Initialize(_T("WDMCHECK")))
		{
		ReportError("WDMCHECK.VXD", GetLastError());
		return 1;
		}

	// Examine each of the imports in this module to see if any of them might be
	// undefined.

	char* misnames = _strdup("Module uses the following missing functions:\n");

	PIMAGE_IMPORT_DESCRIPTOR imports = image.GetImportDescriptor();
	BOOL okay = TRUE;			// assume everything will be fine
	if (imports)
		{						// examine imports
		for ( ; imports->Name; ++imports)
			{					// for each import module
			char* modname = (char*) image.OffsetPointer(imports->Name);

			static char* kernmodules[] = {
				"ntoskrnl.exe",
				"hal.dll",
				"ndis.sys",
				"scsi.sys",
				};

			for (int i = 0; i < arraysize(kernmodules); ++i)
				if (_stricmp(modname, kernmodules[i]) == 0)
					break;

			if (i < arraysize(kernmodules))
				{				// kernel import
				if (!CheckKernelExports(&image, &helper, modname, imports, misnames))
					okay = FALSE;
				}				// kernel import
			else
				{				// other import
				if (!CheckOtherExports(&image, modname, imports, misnames))
					okay = FALSE;
				}				// other import
			}					// for each import module
		}						// examine imports

	// Report the results

	if (okay)
		MessageBox(NULL, "Module has no missing import links", "WdmCheck", MB_OK);
	else
		MessageBox(NULL, misnames, "WdmCheck", MB_OK | MB_ICONHAND);

	return NO_ERROR;
	}							// Winmain

///////////////////////////////////////////////////////////////////////////////

BOOL CheckKernelExports(CImageFile* image, CWdmcheckHelper* helper, char* modname, PIMAGE_IMPORT_DESCRIPTOR imports, char*& misnames)
	{							// CheckKernelExports
	BOOL okay = TRUE;
	PIMAGE_THUNK_DATA thunk = (PIMAGE_THUNK_DATA) image->OffsetPointer((DWORD) imports->FirstThunk);
	for ( ; thunk->u1.Function; ++thunk)
		{					// for each imported symbol
		char* name;
		if (IMAGE_SNAP_BY_ORDINAL(thunk->u1.Ordinal))
			name = (char*) IMAGE_ORDINAL(thunk->u1.Ordinal);
		else
			{					// import by name
			PIMAGE_IMPORT_BY_NAME byname = (PIMAGE_IMPORT_BY_NAME) image->OffsetPointer((DWORD) thunk->u1.AddressOfData);
			name = (char*) byname->Name;
			}					// import by name

		DWORD flags = helper->IsExported(modname, name);

		if (flags)
			{					// missing export
			okay = FALSE;
			int newlen = strlen(misnames) + 2;	// incl null terminator and newline
			char* addname;
			if (HIWORD(name))
				{				// named import
				addname = name;
				newlen += strlen(name);
				}				// named import
			else
				{				// export by ordinal
				addname = new char[strlen(modname) + 10];
				sprintf(addname, "%s#%4.4X", IMAGE_ORDINAL(thunk->u1.Ordinal));
				newlen += strlen(addname);
				}				// export by ordinal

			static char* stubdef = " (exported by WDMSTUB)";

			if (flags & WDMCHECK_STUB_DEFINED)
				newlen += strlen(stubdef);

			char* newnames = new char[newlen];
			strcpy(newnames, misnames);
			strcat(newnames, "\n");
			delete [] misnames;
			misnames = newnames;

			strcat(newnames, addname);
			if (!HIWORD(name))
				delete [] addname;
			if (flags & WDMCHECK_STUB_DEFINED)
				strcat(newnames, stubdef);
			}					// missing export
		}						// for each imported symbol
	return okay;
	}							// CheckKernelExports

///////////////////////////////////////////////////////////////////////////////

BOOL CheckOtherExports(CImageFile* image, char* modname, PIMAGE_IMPORT_DESCRIPTOR imports, char*& misnames)
	{							// CheckOtherExports

	// Open the referenced module and obtain information about its export section

	char pathname[_MAX_PATH];
	GetWindowsDirectory(pathname, sizeof(pathname));
	strcat(pathname, "\\system32\\drivers\\");
	strcat(pathname, modname);

	CImageFile lib(pathname);
	PIMAGE_EXPORT_DIRECTORY exports = NULL;
	ULONG nfunc = 0;
	PULONG functions = NULL;
	PULONG names = NULL;
	ULONG nnames = 0;
	PUSHORT ordinals = NULL;

	if (lib.Open())
		{						// get info about exports
		exports = lib.GetExportDirectory();
		nfunc = exports->NumberOfFunctions;
		functions = (PULONG) lib.OffsetPointer((ULONG) exports->AddressOfFunctions);
		names = (PULONG) lib.OffsetPointer((ULONG) exports->AddressOfNames);
		nnames = exports->NumberOfNames;
		ordinals = (PUSHORT) lib.OffsetPointer((ULONG) exports->AddressOfNameOrdinals);
		}						// get info about exports

	// Try to resolve the symbols our target driver references from this library

	BOOL okay = TRUE;			// assume everything okay
	PIMAGE_THUNK_DATA thunk = (PIMAGE_THUNK_DATA) image->OffsetPointer((DWORD) imports->FirstThunk);
	for ( ; thunk->u1.Function; ++thunk)
		{					// for each imported symbol
		ULONG address = 0;
		char* addname;
		int newlen = strlen(misnames) + 2;	// incl null terminator and newline
		
		if (IMAGE_SNAP_BY_ORDINAL(thunk->u1.Ordinal))
			{					// import by ordinal
			USHORT ordinal = (USHORT) IMAGE_ORDINAL(thunk->u1.Ordinal);
			if (ordinal >= nfunc)
				address = 0;
			else
				address = functions[ordinal];

			if (!address)
				{				// no such ordinal
				addname = new char[strlen(modname) + 10];
				sprintf(addname, "%s#%4.4X", IMAGE_ORDINAL(thunk->u1.Ordinal));
				newlen += strlen(addname);
				}				// no such ordinal
			}					// import by ordinal
		else
			{					// import by name
			PIMAGE_IMPORT_BY_NAME byname = (PIMAGE_IMPORT_BY_NAME) image->OffsetPointer((DWORD) thunk->u1.AddressOfData);
			char* name = (char*) byname->Name;
			for (ULONG i = 0; i < nnames; ++i)
				if (strcmp(name, (char*) lib.OffsetPointer(names[i])) == 0)
					{			// found it
					address = functions[ordinals[i]];
					break;
					}			// found it

			if (!address)
				{				// no such symbol
				addname = name;
				newlen += strlen(name);
				}				// no such symbol
			}					// import by name

		if (!address)
			{					// missing export
			okay = FALSE;

			char* newnames = new char[newlen];
			strcpy(newnames, misnames);
			strcat(newnames, "\n");
			delete [] misnames;
			misnames = newnames;

			strcat(newnames, addname);
			if (IMAGE_SNAP_BY_ORDINAL(thunk->u1.Ordinal))
				delete [] addname;
			}					// missing export
		}						// for each imported symbol

	return okay;
	}							// CheckOtherExports

///////////////////////////////////////////////////////////////////////////////

DWORD ReportError(char* text, DWORD code)
	{							// Report Error
	char* msg = NULL;
	char fullmsg[512];

	if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &msg, 0, NULL))

		_snprintf(fullmsg, sizeof(fullmsg), "%s: %s", text, msg);

	else
		_snprintf(fullmsg, sizeof(fullmsg), "%s: error %d", text, code);

	MessageBox(NULL, fullmsg, "WdmCheck", MB_OK | MB_ICONHAND);

	if (msg)
		LocalFree(msg);

	return code;
	}							// Report Error