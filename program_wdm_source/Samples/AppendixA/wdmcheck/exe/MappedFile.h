// MappedFile.h -- Declaration of CMappedFile class
// Copyright (C) 2000 by Walter Oney
// All rights reserved

#ifndef MAPPEDFILE_H
#define MAPPEDFILE_H
													  
///////////////////////////////////////////////////////////////////////////////

class CMappedFile
{								// class CMappedFile

public:
	CMappedFile(LPCTSTR filename);
	virtual ~CMappedFile();

	void	Close();
	LPCVOID GetData() const;
	LPCVOID GetDataEnd() const;
	LPCTSTR	GetName() const;
	LPVOID	OffsetPointer(DWORD offset);
	virtual BOOL Open();

protected:
	LPCTSTR	m_filename;
	HANDLE	m_hFile;
	HANDLE	m_hMapping;
	LPCVOID	m_data;
	LPCVOID	m_end;
	DWORD	m_size;
};								// class CMappedFile

#endif