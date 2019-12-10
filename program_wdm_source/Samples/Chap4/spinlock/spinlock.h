// spinlock.h -- Declarations for platform-portable spinlock package
// Copyright (C) 2002 by Walter Oney
// All rights reserved

// This file declares platform-portable routines for acquiring and releasing
// spin locks. In Windows XP and later, it uses an in-stack queued spin lock.
// In earlier versions of Windows, it uses a regular spin lock. To use this
// package of routines:
//
//	1.	Call InitializeSpinLockFunctionPointers from your DriverEntry routine.
//	2.	Initialize KSPIN_LOCK objects in the usual way (by calling
//		KeInitializeSpinLock).
//	3.	To protect a section of code with a spin lock, follow this pattern:
//			KLOCK_QUEUE_HANDLE qh;
//			AcquireSpinLock(&lock, &qh);
//			. . .
//			ReleaseSpinLock(&qh);
//	4.	If you know you're at DISPATCH_LEVEL, you can call these functions instead:
//			KLOCK_QUEUE_HANDLE qh;
//			AcquireSpinLockAtDpcLevel(&lock, &qh);
//			. . .
//			ReleaseSpinLockFromDpcLevel(&qh);

// Note that a Win98/Me driver that uses this package will need WDMSTUB to
// define MmGetSystemRoutineAddress.
//
// For the ultimate, best-possible, performance, don't use this package at
// all. Instead, ship separate driver binaries for XP and for earlier systems.
// Use conditional compilation to choose which set of spin lock functions you
// call. Use the platform-specific suffix mechanism for model sections in
// an INF file to control which driver gets installed.

#pragma once

///////////////////////////////////////////////////////////////////////////////

void InitializeSpinLockFunctionPointers();

void AcquireSpinLock(PKSPIN_LOCK lock, PKLOCK_QUEUE_HANDLE qh);
void ReleaseSpinLock(PKLOCK_QUEUE_HANDLE qh);

void AcquireSpinLockAtDpcLevel(PKSPIN_LOCK lock, PKLOCK_QUEUE_HANDLE qh);
void ReleaseSpinLockAtDpcLevel(PKLOCK_QUEUE_HANDLE qh);
