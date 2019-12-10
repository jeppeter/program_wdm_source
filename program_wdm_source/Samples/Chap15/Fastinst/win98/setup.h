// Setup.h -- Declaration of setup helper classes
// Copyright (C) 1999 by Walter Oney
// All rights reserved

#ifndef SETUP_H
#define SETUP_H

typedef void* PVOID;
typedef const char* PCTSTR;
typedef char* PTSTR;

#ifndef arraysize
	#define arraysize(p) (sizeof(p)/sizeof((p)[0]))
#endif

///////////////////////////////////////////////////////////////////////////////

struct InstallStruct {
	CString devid;				// target device id
	CString mfgname;			// name of manufacturer
	CString devname;			// name of device
	CString instname;			// install section name
	BOOL done;					// TRUE when time to stop enumeration
	};

///////////////////////////////////////////////////////////////////////////////

class CInf;
typedef BOOL (*PENUMDEVCALLBACK)(CInf* inf, const CString& devname, const CString& instsecname, const CStringArray& devid, PVOID context, UINT& code);
typedef BOOL (*PENUMMFGCALLBACK)(CInf* inf, const CString& mfgname, const CString& modelname, PVOID context, UINT& code);

///////////////////////////////////////////////////////////////////////////////

class CInf
{								// class CInf
public:
	CInf();
	~CInf();

	CString		m_name;			// name of file
	CString		m_classname;	// name of device class
	ATOM		m_atom;			// INF name atom
	HINF		m_hinf;			// open handle

	UINT		EnumDevices(const CString& modelname, PENUMDEVCALLBACK callback, PVOID context);
	UINT		EnumManufacturers(PENUMMFGCALLBACK callback, PVOID context);
	UINT		Open(PCTSTR name, BOOL defsearch = TRUE);
};								// class CInf


#endif
