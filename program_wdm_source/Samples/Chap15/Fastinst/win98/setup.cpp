// Setup.cpp -- Helper classes for FASTINST
// Copyright (C) 1999 by Walter Oney
// All rights reserved

#include "stdafx.h"
#include "setup.h"

UINT GetField(HINF hinf, HINFLINE hLine, int index, CString& value);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

CInf::CInf()
	{							// CInf::CInf
	m_hinf = (HINF) HFILE_ERROR;
	m_atom = (ATOM) NULL;
	}							// CInf::CInf

CInf::~CInf()
	{							// CInf::~CInf
	if (m_hinf != (HINF) HFILE_ERROR)
		IpClose(m_hinf);
	if (m_atom)
		GlobalDeleteAtom(m_atom);
	}							// CInf::~CInf

///////////////////////////////////////////////////////////////////////////////

UINT CInf::EnumDevices(const CString& modelname, PENUMDEVCALLBACK callback, PVOID context)
	{							// CInf::EnumDevices
	HINFLINE hLine;
	UINT code;

	if ((code = IpFindFirstLine(m_hinf, modelname, NULL, &hLine)) != 0)
		return code;

	do	{						// for each model
		CString devname, instname, id;
		CStringArray idlist;

		if ((code = GetField(m_hinf, hLine, 0, devname)) != 0
			|| (code = GetField(m_hinf, hLine, 1, instname)) != 0)
			return code;

		int nfields;
		if ((code = IpGetFieldCount(m_hinf, hLine, &nfields)) != 0)
			return code;

		for (int i = 2; i <= nfields; ++i)
			if (GetField(m_hinf, hLine, i, id) == 0)
				idlist.Add(id);
			else
				break;

		if (idlist.GetSize() == 0)
			idlist.Add(_T(""));

		if (!(*callback)(this, devname, instname, idlist, context, code))
			return code;
		}						// for each model
	while (IpFindNextLine(m_hinf, &hLine) == 0);

	return 0;
	}							// CInf::EnumDevices

///////////////////////////////////////////////////////////////////////////////

UINT CInf::EnumManufacturers(PENUMMFGCALLBACK callback, PVOID context)
	{							// CInf::EnumManufacturers
	HINFLINE mfg;				// context for manufacturer section scan
	UINT code;

	if ((code = IpFindFirstLine(m_hinf, _T("Manufacturer"), NULL, &mfg)) != 0)
		return code;

	// Loop throught the manufacturer section, calling the specified callback
	// function for each one

	do	{						// for each manufacturer
		CString mfgname, modelname;
		if ((code = GetField(m_hinf, mfg, 0, mfgname)) != 0
			|| (code = GetField(m_hinf, mfg, 1, modelname)) != 0)
			return code;

		// Invoke callback function. It returns TRUE if we should continue
		// the enumeration or FALSE if we should stop.
		
		if (!(*callback)(this, mfgname, modelname, context, code))
			return code;
		}						// for each manufacturer
	while (IpFindNextLine(m_hinf, &mfg) == 0);

	return 0;
	}							// CInf::EnumManufacturers

///////////////////////////////////////////////////////////////////////////////

UINT CInf::Open(PCTSTR name, BOOL defsearch /* = TRUE */)
	{							// CInf::Open

	CString infname(name);
	if (infname[0] == '"')
		infname = infname.Mid(2, infname.GetLength() - 2); // remove quotes from name

	// If we're not to use the default search path for INF files (namely, directories
	// named in HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\DevicePath [i.e.,
	// the INF directory], make sure we present a pathname rather than just a
	// name to setup functions

	if (!defsearch && _tcschr(infname, _T('\\')) == NULL)
		m_name = _T(".\\");
	m_name += infname;

	UINT code = IpOpen(m_name, &m_hinf);
	if (code != 0)
		return code;

	// Create a global atom to hold the filename.

	m_atom = GlobalAddAtom(m_name);

	// Determine the class name for this INF

	char classname[256];
	code = DiGetINFClass((LPSTR)(LPCSTR) m_name, INFTYPE_TEXT, classname, sizeof(classname));
	if (code != 0)
		return code;
	m_classname = classname;
	return code;
	}							// CInf::Open

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UINT GetField(HINF hinf, HINFLINE hLine, int index, CString& value)
	{							// GetField
	int need;
	UINT code;
	char buffer[256];

	if ((code = IpGetStringField(hinf, hLine, index, buffer, arraysize(buffer), &need)) != 0)
		return code;

	if (buffer[0] == '%')
		{						// possible string reference
		CString stringname = buffer;
		stringname = stringname.Mid(1, stringname.GetLength() - 2);
		HINFLINE hString;
		code = IpFindFirstLine(hinf, "Strings", stringname, &hString);
		if (code == 0)
			code = IpGetStringField(hinf, hString, 1, buffer, arraysize(buffer), &need);
		if (code != 0)
			return code;
		}						// possible string reference

	value = buffer;
	return code;
	}							// GetField
