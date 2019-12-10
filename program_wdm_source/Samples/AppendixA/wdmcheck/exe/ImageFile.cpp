// ImageFile.cpp -- Implemention of CImageFile class
// Copyright (C) 2000 by Walter Oney
// All rights reserved

#include "stdafx.h"
#include "ImageFile.h"
													  
///////////////////////////////////////////////////////////////////////////////

CImageFile::CImageFile(LPCTSTR filename) : CMappedFile(filename)
	{							// CImageFile::CImageFile
	}							// CImageFile::CImageFile

CImageFile::~CImageFile()
	{							// CImageFile::~CImageFile
	}							// CImageFile::~CImageFile
													  
///////////////////////////////////////////////////////////////////////////////

PIMAGE_EXPORT_DIRECTORY CImageFile::GetExportDirectory()
	{							// CImageFile::GetExportDirectory
	PIMAGE_DATA_DIRECTORY dir = &m_hdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	if (dir->VirtualAddress)
		return (PIMAGE_EXPORT_DIRECTORY) OffsetPointer(dir->VirtualAddress);
	else
		return NULL;
	}							// CImageFile::GetExportDirectory
													  
///////////////////////////////////////////////////////////////////////////////

PIMAGE_IMPORT_DESCRIPTOR CImageFile::GetImportDescriptor()
	{							// CImageFile::GetImportDescriptor
	PIMAGE_DATA_DIRECTORY dir = &m_hdr->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	if (dir->VirtualAddress)
		return (PIMAGE_IMPORT_DESCRIPTOR) OffsetPointer(dir->VirtualAddress);
	else
		return NULL;
	}							// CImageFile::GetImportDescriptor
													  
///////////////////////////////////////////////////////////////////////////////

BOOL CImageFile::Open()
	{							// CImageFile::Open
	if (!CMappedFile::Open())
		return FALSE;
	
	// Locate the PE header

	PIMAGE_DOS_HEADER exehdr = (PIMAGE_DOS_HEADER) m_data;
	if (exehdr->e_magic != IMAGE_DOS_SIGNATURE || exehdr->e_lfarlc <= FIELD_OFFSET(IMAGE_DOS_HEADER, e_lfanew))
		{						// invalid EXE
		Close();
		return FALSE;
		}						// invalid EXE

	m_hdroff = exehdr->e_lfanew;
	m_hdr = (PIMAGE_NT_HEADERS) OffsetPointer(m_hdroff);
	if (m_hdr->Signature != IMAGE_NT_SIGNATURE)
		{						// not a PE module
		Close();
		return FALSE;
		}						// not a PE module

	return TRUE;
	}							// CImageFile::Open