// ImageFile -- Declaration of CImageFile class
// Copyright (C) 2000 by Walter Oney
// All rights reserved

#ifndef IMAGEFILE_H
#define IMAGEFILE_H

#include "MappedFile.h"
													  
///////////////////////////////////////////////////////////////////////////////

class CImageFile : public CMappedFile
{								// class CImageFile
public:
	CImageFile(LPCTSTR filename);
	virtual ~CImageFile();

	PIMAGE_EXPORT_DIRECTORY GetExportDirectory();
	PIMAGE_IMPORT_DESCRIPTOR GetImportDescriptor();
	virtual BOOL Open();

protected:
	PIMAGE_NT_HEADERS	m_hdr;		// PE file header
	DWORD				m_hdroff;	// offset of PE header in mapped image
};								// class CImageFile

#endif