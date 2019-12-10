// spinlock.cpp -- platform-portable spinlock functions
// Copyright (C) 2002 by Walter Oney
// All rights reserved

#include "stddcls.h"
#include "driver.h"				// used only for def'n of DRIVERNAME
#include "spinlock.h"

typedef VOID (FASTCALL *KEACQUIREINSTACKQUEUEDSPINLOCK)(PKSPIN_LOCK, PKLOCK_QUEUE_HANDLE);
typedef VOID (FASTCALL *KEACQUIREINSTACKQUEUEDSPINLOCKATDPCLEVEL)(PKSPIN_LOCK, PKLOCK_QUEUE_HANDLE);
typedef VOID (FASTCALL *KERELEASEINSTACKQUEUEDSPINLOCK)(PKLOCK_QUEUE_HANDLE);
typedef VOID (FASTCALL *KERELEASEINSTACKQUEUEDSPINLOCKFROMDPCLEVEL)(PKLOCK_QUEUE_HANDLE);

KEACQUIREINSTACKQUEUEDSPINLOCK pKeAcquireInStackQueuedSpinLock;
KEACQUIREINSTACKQUEUEDSPINLOCKATDPCLEVEL pKeAcquireInStackQueuedSpinLockAtDpcLevel;
KERELEASEINSTACKQUEUEDSPINLOCK pKeReleaseInStackQueuedSpinLock;
KERELEASEINSTACKQUEUEDSPINLOCKFROMDPCLEVEL pKeReleaseInStackQueuedSpinLockFromDpcLevel;

PVOID GetSystemRoutineAddress(PCWSTR name);

BOOLEAN UseQueuedLocks = FALSE;

///////////////////////////////////////////////////////////////////////////////
// InitializeSpinLockFunctionPointers is called from DriverEntry to decide
// whether to use in-stack queued spin locks or regular spin locks. This function
// calls MmGetSystemRoutineAddress, which does not exist in Win98/Me. Consequently,
// you'd need to use WDMSTUB with your driver in those systems.

#pragma PAGEDCODE

void InitializeSpinLockFunctionPointers()
	{							// InitializeSpinLockFunctionPointers
	pKeAcquireInStackQueuedSpinLock = (KEACQUIREINSTACKQUEUEDSPINLOCK) GetSystemRoutineAddress(L"KeAcquireInStackQueuedSpinLock");
	pKeAcquireInStackQueuedSpinLockAtDpcLevel = (KEACQUIREINSTACKQUEUEDSPINLOCKATDPCLEVEL) GetSystemRoutineAddress(L"KeAcquireInStackQueuedSpinLockAtDpcLevel");
	pKeReleaseInStackQueuedSpinLock = (KERELEASEINSTACKQUEUEDSPINLOCK) GetSystemRoutineAddress(L"KeReleaseInStackQueuedSpinLock");
	pKeReleaseInStackQueuedSpinLockFromDpcLevel = (KERELEASEINSTACKQUEUEDSPINLOCKFROMDPCLEVEL) GetSystemRoutineAddress(L"KeReleaseInStackQueuedSpinLockFromDpcLevel");

	if (pKeAcquireInStackQueuedSpinLock
		&& pKeAcquireInStackQueuedSpinLockAtDpcLevel
		&& pKeReleaseInStackQueuedSpinLock
		&& pKeReleaseInStackQueuedSpinLockFromDpcLevel)

		{
		KdPrint((DRIVERNAME " - Using in-stack queued spin locks\n"));
		UseQueuedLocks = TRUE;
		}

	else
		{
		KdPrint((DRIVERNAME " - Using regular spin locks\n"));
		UseQueuedLocks = FALSE;
		}
	}							// InitializeSpinLockFunctionPointers

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

void AcquireSpinLock(PKSPIN_LOCK lock, PKLOCK_QUEUE_HANDLE qh)
	{							// AcquireSpinLock
	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

	if (UseQueuedLocks)											
		(*pKeAcquireInStackQueuedSpinLock)(lock, qh);
	else
		{
		qh->LockQueue.Lock = lock;
		KeAcquireSpinLock(lock, &qh->OldIrql);
		}
	}							// AcquireSpinLock

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

void ReleaseSpinLock(PKLOCK_QUEUE_HANDLE qh)
	{							// ReleaseSpinLock
	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
	if (UseQueuedLocks)
		(*pKeReleaseInStackQueuedSpinLock)(qh);
	else
		KeReleaseSpinLock(qh->LockQueue.Lock, qh->OldIrql);
	}							// ReleaseSpinLock

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

void AcquireSpinLockAtDpcLevel(PKSPIN_LOCK lock, PKLOCK_QUEUE_HANDLE qh)
	{							// AcquireSpinLockAtDpcLevel
	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
	if (UseQueuedLocks)
		(*pKeAcquireInStackQueuedSpinLockAtDpcLevel)(lock, qh);
	else
		{
		qh->LockQueue.Lock= lock;
		qh->OldIrql = DISPATCH_LEVEL;
		KeAcquireSpinLockAtDpcLevel(lock);
		}
	}							// AcquireSpinLockAtDpcLevel

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

void ReleaseSpinLockAtDpcLevel(PKLOCK_QUEUE_HANDLE qh)
	{							// ReleaseSpinLockAtDpcLevel
	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
	if (UseQueuedLocks)
		(*pKeReleaseInStackQueuedSpinLockFromDpcLevel)(qh);
	else
		KeReleaseSpinLockFromDpcLevel(qh->LockQueue.Lock);
	}							// ReleaseSpinLockAtDpcLevel

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

PVOID GetSystemRoutineAddress(PCWSTR name)
	{							// GetSystemRoutineAddress
	UNICODE_STRING us;
	RtlInitUnicodeString(&us, name);
	return MmGetSystemRoutineAddress(&us);
	}							// GetSystemRoutineAddress