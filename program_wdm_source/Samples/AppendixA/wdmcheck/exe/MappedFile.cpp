// MappedFile.cpp -- Definition of CMappedFile class
// Copyright (C) 2000 by Walter Oney
// All rights reserved

#include "stdafx.h"
#include "MappedFile.h"

///////////////////////////////////////////////////////////////////////////////
// @mfunc <c CMappedFile> Constructor
// @parm Name of file to be accessed

CMappedFile::CMappedFile(LPCTSTR filename)
	{							// CMappedFile::CMappedFile
	m_filename = _tcsdup(filename);
	m_hFile = INVALID_HANDLE_VALUE;
	m_hMapping = NULL;
	}							// CMappedFile::CMappedFile

// @mfunc <c CMappedFile> Destructor

CMappedFile::~CMappedFile()
	{							// CMappedFile::~CMappedFile
	if (m_filename)
		free((LPVOID) m_filename);
	Close();
	}							// CMappedFile::~CMappedFile

///////////////////////////////////////////////////////////////////////////////
// @mfunc Close the file

void CMappedFile::Close()
	{							// CMappedFile::Close
	if (m_data)
		UnmapViewOfFile(m_data);
	m_data = NULL;

	if (m_hMapping)
		CloseHandle(m_hMapping);
	m_hMapping = NULL;

	if (m_hFile != INVALID_HANDLE_VALUE)
		CloseHandle(m_hFile);
	m_hFile = INVALID_HANDLE_VALUE;
	}							// CMappedFile::Close

///////////////////////////////////////////////////////////////////////////////
// @mfunc Get data location
// @rdesc Starting address of mapped data file

LPCVOID CMappedFile::GetData() const
	{							// CMappedFile::GetData
	return m_data;
	}							// CMappedFile::GetData

///////////////////////////////////////////////////////////////////////////////
// @mfunc Get ending location of data in file
// @rdesc Ending address of data

LPCVOID CMappedFile::GetDataEnd() const
	{							// CMappedFile::GetDataEnd
	return m_end;
	}							// CMappedFile::GetDataEnd

///////////////////////////////////////////////////////////////////////////////
// @mfunc Get name of underlying file
// @rdesc Name of the file

LPCTSTR CMappedFile::GetName() const
	{							// CMappedFile::GetName
	return m_filename;
	}							// CMappedFile::GetName

///////////////////////////////////////////////////////////////////////////////
// @mfunc Convert offset to pointer
// @parm Offset value
// @rdesc Address of given location in mapped file, or NULL if out of bounds

LPVOID CMappedFile::OffsetPointer(DWORD offset)
	{							// CMappedFile::OffsetPointer
	if (offset >= m_size)
		return NULL;
	return (LPVOID) ((PUCHAR) m_data + offset);
	}							// CMappedFile::OffsetPointer

///////////////////////////////////////////////////////////////////////////////
// @mfunc Open a mapped file
// @rdesc TRUE if mapping established okay, FALSE otherwise

BOOL CMappedFile::Open()
	{							// CMappedFile::Open
	m_hFile = CreateFile(m_filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (m_hFile == INVALID_HANDLE_VALUE)
		return FALSE;
	m_hMapping = CreateFileMapping(m_hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (!m_hMapping)
		return FALSE;
	m_data = MapViewOfFile(m_hMapping, FILE_MAP_READ, 0, 0, 0);
	if (!m_data)
		return FALSE;

	m_size = GetFileSize(m_hFile, NULL);
	m_end = (LPCVOID) ((LPBYTE) m_data + m_size);

	return TRUE;
	}							// CMappedFile::Open
