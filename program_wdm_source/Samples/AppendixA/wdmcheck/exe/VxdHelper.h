// VxdHelper.h -- Declaration of CVxdHelper class
// Copyright (C) 2000 by Walter Oney
// All rights reserved

#ifndef VXDHELPER_H
#define VXDHELPER_H

///////////////////////////////////////////////////////////////////////////////

class CVxdHelper
{								// class CVxdHelper
public:
	CVxdHelper();
	virtual ~CVxdHelper();

	virtual BOOL Initialize(LPCTSTR name);

protected:
	HANDLE		m_hdriver;		// handle to device object
};								// class CVxdHelper

#endif