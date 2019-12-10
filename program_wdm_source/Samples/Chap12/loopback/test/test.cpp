// TEST.CPP -- test program for LOOPBACK.SYS
// Copyright (C) 1999 by Walter Oney
// All rights reserved

#include "stdafx.h"

VOID DumpData(char* s, PUCHAR p, ULONG len);

int main(int argc, char* argv[])
	{							// main

	// Determine size of test. Default is 64 bytes (one endpoint's worth), but this
	// can be overridden from the command line

	int testsize = 16;			// i.e., 64 bytes
	if (argc > 1)
		testsize = atoi(argv[1]);
	if (testsize > 1024)
		{
		puts("Maximum buffer size for this device is 1024 dwords (4096 bytes)");
		return 1;
		}

	PDWORD data = (PDWORD) malloc(testsize * sizeof(DWORD));
	if (!data)
		return 4;

	HANDLE hdevice = CreateFile("\\\\.\\LOOPBACK", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hdevice == INVALID_HANDLE_VALUE)
		{
		printf("Unable to open LOOPBACK device - error %d\n", GetLastError());
		return 1;
		}

	DWORD retcode = 0;

	for (int i = 0; i < testsize; ++i)
		data[i] = i;

	DWORD junk;
	if (WriteFile(hdevice, data, testsize * sizeof(DWORD), &junk, NULL))
		printf("Write transferred %d bytes okay\n", junk);
	else
		printf("Error %d trying to write data\n", GetLastError());

	memset(data, 0, sizeof(data));

	if (ReadFile(hdevice, data, testsize * sizeof(DWORD), &junk, NULL))
		{
		for (i = 0; i < testsize; ++i)
			if (data[i] != (DWORD) i)
				break;
		if (i >= testsize)
			printf("Read transferred %d bytes correctly\n", junk);
		else
			{
			printf("Read claimed to transfer %d bytes okay, but %d'th DWORD is incorrect\n", junk, i);
			DumpData("IN ", (PUCHAR) data, junk);
			retcode = 2;
			}
		}
	else
		{
		printf("Error %d trying to read data\n", GetLastError());
		retcode = 3;
		}
	
	CloseHandle(hdevice);
	return retcode;
	}							// main

VOID DumpData(char* s, PUCHAR p, ULONG len)
	{							// DumpData
	ULONG offset = 0;			// offset of current line of data
	static char* hexit = "0123456789ABCDEF";
	char buffer[76];			// output line buffer

	while (len)
		{						// for each line of output
		#define HEXBUF (buffer + 9)		// where hex data goes
		#define XLBUF (buffer + 58)		// where translated data goes

		ULONG nbytes = len > 16 ? 16 : len;	// up to 16 bytes each line
		sprintf(buffer, "%3s %4.4X:", s, offset);
		memset(HEXBUF, ' ', 3 * 16 + 1 + 16);
		buffer[74] = 0;

		for (ULONG i = 0; i < nbytes; ++i)
			{					// for each byte on line
			char ch = p[i];
			HEXBUF[3 * i + 1] = hexit[(ch >> 4) & 15];
			HEXBUF[3 * i + 2] = hexit[ch & 15];

			XLBUF[i] = (ch >= ' ' && ch < 128) ? ch : '.';
			}					// for each byte on line
		
		puts(buffer);

		p += nbytes;
		offset += nbytes;
		len -= nbytes;
		}						// for each line of output
	}							// DumpData
