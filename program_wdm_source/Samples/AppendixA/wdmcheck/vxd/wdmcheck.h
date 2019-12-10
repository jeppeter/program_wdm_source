// wdmcheck.h -- Declarations for WDMCHECK.VXD

#ifndef WDMCHECK_H
#define WDMCHECK_H

#define WDMCHECK_VXD_ID Undefined_Device_ID
#define DRIVERNAME "WDMCHECK"

///////////////////////////////////////////////////////////////////////////////

typedef struct _SECTION_INFO {
	DWORD startaddr;
	DWORD endaddr;
	} SECTION_INFO, *PSECTION_INFO;

///////////////////////////////////////////////////////////////////////////////

class CStubVersion
{								// class CStubVersion
public:
	CStubVersion(PVMMDDB ddb);
	CStubVersion(PDEVICE_OBJECT DeviceObject);
	~CStubVersion();

	CStubVersion* m_next;
	CStubVersion* m_prev;

	PVMMDDB m_ddb;			// DDB address for VxD
	char* m_name;			// name of WDM driver
	ULONG m_nsections;		// number of sections
	PSECTION_INFO m_sections;	// list of sectionds

	void Initialize();
	void GetSectionInfo(DeviceInfo* dip);
	void GetSectionInfo(Device_Location_List* dll);

	static CStubVersion*	Anchor;	// anchor of global list
};								// class CStubVersion

#endif // WDMCHECK_H
