// WdmcheckHelper.h -- Declaration of CWdmcheckHelper class
// Copyright (C) 2000 by Walter Oney
// All rights reserved

#ifndef WDMCHECKHELPER_H
#define WDMCHECKHELPER_H

#include "VxdHelper.h"

///////////////////////////////////////////////////////////////////////////////

class CWdmcheckHelper : public CVxdHelper
{								// class CWdmcheckHelper
public:
	CWdmcheckHelper();
	virtual ~CWdmcheckHelper();

	DWORD	IsExported(char* modname, char* fcn);
};								// class CWdmcheckHelper


#endif
