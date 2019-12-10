// Illustrative power management routines for WAKEUP sample driver
// Copyright (C) 2002 by Walter Oney
// All rights reserved

#include "stddcls.h"
#include "driver.h"

VOID CancelSelectiveSuspend(PDEVICE_EXTENSION pdx);
NTSTATUS IdleDevice(PDEVICE_EXTENSION pdx);
NTSTATUS IssueSelectiveSuspendRequest(PDEVICE_EXTENSION pdx);
VOID SelectiveSuspendCallback(PDEVICE_EXTENSION pdx);
NTSTATUS SelectiveSuspendCompletionRoutine(PDEVICE_OBJECT junk, PIRP Irp, PDEVICE_EXTENSION pdx);

///////////////////////////////////////////////////////////////////////////////
// CancelSelectiveSuspend cancels the outstanding selective-suspend IRP, if
// there is one. Refer to the Chapter 5 discussion about how to safely cancel
// an asynchronous IRP for an explanation

#pragma LOCKEDCODE

VOID CancelSelectiveSuspend(PDEVICE_EXTENSION pdx)
	{							// CancelSelectiveSuspend
	PIRP Irp = (PIRP) InterlockedExchangePointer((PVOID*) &pdx->SuspendIrp, NULL);
	if (Irp)
		{						// cancel irp
		KdPrint((DRIVERNAME " - Canceling SUBMIT_IDLE_NOTIFICATION request\n"));
		IoCancelIrp(Irp);
		if (InterlockedExchange(&pdx->SuspendIrpCancelled, 1))
			IoFreeIrp(Irp);
		}						// cancel irp
	}							// CancelSelectiveSuspend

///////////////////////////////////////////////////////////////////////////////
// ChangeAutoPower is called by the WMI IRP handler whenever a change is made
// to the MSPower_DeviceEnable setting. TRUE indicates that we should depower
// the device when it's not in use. FALSE indicates that we should leave the
// device powered at all times.

// Since we're not really driving the device, this sample pretends that the device
// is *never* in use. A real driver would call GenericWakeupFromIdle and
// GenericIdleDevice from, e.g., the IRP_MJ_CREATE and IRP_MJ_CLOSE dispatch
// routines, respectively.

// If your driver doesn't support automatic power-down, you could leave out this
// routine and the support in WMI.CPP for MSPower_DeviceEnable.

#pragma PAGEDCODE

VOID ChangeAutoPower(PDEVICE_EXTENSION pdx, BOOLEAN enable)
	{							// ChangeAutoPower
	PAGED_CODE();
	if (enable == pdx->autopower)
		return;					// no change, so nothing to do
	pdx->autopower = enable;

	// If disabling auto-power behavior, put the device into D0.

	if (!enable)
		{
		CancelSelectiveSuspend(pdx);
		GenericWakeupFromIdle(pdx->pgx, TRUE);
		}

	// If enabling auto-power behavior, we want to depower the device.

	else
		IdleDevice(pdx);

	// Update registry parameter

	HANDLE hkey;
	NTSTATUS status = IoOpenDeviceRegistryKey(pdx->Pdo, PLUGPLAY_REGKEY_DEVICE, KEY_SET_VALUE, &hkey);
	if (!NT_SUCCESS(status))
		{
		KdPrint((DRIVERNAME " - IoOpenDeviceRegistryKey failed - %X\n", status));
		return;
		}

	UNICODE_STRING valname;
	RtlInitUnicodeString(&valname, L"DeviceEnable");

	ULONG value = enable;
	status = ZwSetValueKey(hkey, &valname, 0, REG_DWORD, &value, sizeof(value));
	if (!NT_SUCCESS(status))
		KdPrint((DRIVERNAME " - ZwSetValueKey failed - %X\n", status));
	ZwClose(hkey);
	}							// ChangeAutoPower

///////////////////////////////////////////////////////////////////////////////
// ChangeWakeupEnable is called by the WMI IRP handler whenever a change is made
// to the MSPower_DeviceWakeEnable setting. TRUE indicates we should program the
// device to wake the sytem. FALSE means we should not.

// If your device doesn't support system wakeup, you could leave out this function
// and the support in WMI.CPP for MSPower_DeviceWakeEnable.

#pragma PAGEDCODE

VOID ChangeWakeupEnable(PDEVICE_EXTENSION pdx, BOOLEAN enable)
	{							// ChangeWakeupEnable
	PAGED_CODE();
	if (enable == pdx->wakeup)
		return;					// no change, so nothing to do
	pdx->wakeup = enable;

	// If enabling wakeup, make sure we're in the D0 state before proceeding
	// because USBHUB requires us to be in the D0 state when we issue IRP_MN_WAIT_WAKE.
	// If your driver doesn't support automatic power-down, you could leave
	// out this step.

	if (enable)
		GenericWakeupFromIdle(pdx->pgx, TRUE);	// i.e., wait for power up

	// Based on the enable/disable flag, either issue or cancel a WAIT_WAKE request.

	GenericWakeupControl(pdx->pgx, enable ? EnableWakeup : DisableWakeup);

	// If we've been told to automatically power down the device, turn the device off. 
	// If your driver doesn't support automatic power-down, you could leave out this step.

	if (pdx->autopower)
		IdleDevice(pdx);

	// Update registry parameter

	HANDLE hkey;
	NTSTATUS status = IoOpenDeviceRegistryKey(pdx->Pdo, PLUGPLAY_REGKEY_DEVICE, KEY_SET_VALUE, &hkey);
	if (!NT_SUCCESS(status))
		{
		KdPrint((DRIVERNAME " - IoOpenDeviceRegistryKey failed - %X\n", status));
		return;
		}

	UNICODE_STRING valname;
	RtlInitUnicodeString(&valname, L"DeviceWakeEnable");

	ULONG value = enable;
	status = ZwSetValueKey(hkey, &valname, 0, REG_DWORD, &value, sizeof(value));
	if (!NT_SUCCESS(status))
		KdPrint((DRIVERNAME " - ZwSetValueKey failed - %X\n", status));
	ZwClose(hkey);
	}							// ChangeWakeupEnable

///////////////////////////////////////////////////////////////////////////////
// GetDevicePowerState returns the device power state we wish to enter for a
// given system power state. The USB bus driver seems to fail our wait_wake
// requests unless we're in the D0 state (I don't know why it should), so this
// function is here to force D0 in that case.

#pragma LOCKEDCODE

DEVICE_POWER_STATE GetDevicePowerState(PDEVICE_OBJECT fdo, SYSTEM_POWER_STATE sstate, DEVICE_POWER_STATE dstate)
	{							// GetDevicePowerState
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	if (sstate > PowerSystemWorking || !pdx->wakeup)
		return dstate;
	return PowerDeviceD0;
	}							// GetDevicePowerState

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS IdleDevice(PDEVICE_EXTENSION pdx)
	{							// IdleDevice
	if (IoIsWdmVersionAvailable(1, 0x20))
		return IssueSelectiveSuspendRequest(pdx);
	else
		return GenericIdleDevice(pdx->pgx, PowerDeviceD3, TRUE);
	}							// IdleDevice

///////////////////////////////////////////////////////////////////////////////
// IssueSelectiveSuspendRequest issues the selective suspend request defined in
// XP and later operating systems.

#pragma PAGEDCODE

NTSTATUS IssueSelectiveSuspendRequest(PDEVICE_EXTENSION pdx)
	{							// IssueSelectiveSuspendRequest
	PAGED_CODE();

	// Out of an excess of caution, use a mutex to ensure that only one invocation
	// of this function can be active at one time. In this driver, there is only
	// one code path leading here, and that's through the WMI dispatch routine, which
	// is being serialized by WMI. But you might just lift this function into some
	// other environment where simultaneous calls would be possible...

	ExAcquireFastMutex(&pdx->SuspendMutex);
	if (pdx->SuspendIrp)
		{						// IRP already outstanding
		ExReleaseFastMutex(&pdx->SuspendMutex);
		return STATUS_SUCCESS;
		}						// IRP already outstanding

	PIRP Irp = IoAllocateIrp(pdx->LowerDeviceObject->StackSize, FALSE);
	if (!Irp)
		{
		ExReleaseFastMutex(&pdx->SuspendMutex);
		return STATUS_INSUFFICIENT_RESOURCES;
		}

	// Initialize the selective suspend IRP

	pdx->cbinfo.IdleCallback = (USB_IDLE_CALLBACK) SelectiveSuspendCallback;
	pdx->cbinfo.IdleContext = (PVOID) pdx;

	PIO_STACK_LOCATION stack = IoGetNextIrpStackLocation(Irp);
	stack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	stack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION;
	stack->Parameters.DeviceIoControl.Type3InputBuffer = &pdx->cbinfo;

	pdx->SuspendIrp = Irp;
	pdx->SuspendIrpCancelled = 0;

	ExReleaseFastMutex(&pdx->SuspendMutex);

	// Install a completion routine for this asynchronous IRP

	IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE) SelectiveSuspendCompletionRoutine,
		(PVOID) pdx, TRUE, TRUE, TRUE);

	// Send the IRP to the USB bus driver

	KdPrint((DRIVERNAME " - Issuing SUBMIT_IDLE_NOTIFICATION request\n"));
	IoCallDriver(pdx->LowerDeviceObject, Irp);

	// Return STATUS_SUCCESS to indicate that the IRP was issued. It may
	// have been completed with an error already, but that's not relevant
	// to our return code here.

	return STATUS_SUCCESS;
	}							// IssueSelectiveSuspendRequest

///////////////////////////////////////////////////////////////////////////////
// GENERIC's power management code calls RestoreDeviceContext whenever power is
// restored. For purposes of this sample driver, we need to pretend to the system
// that the end user is present. With a real HID device, the regular class driver makes
// this call whenever an input event occurs (not just when the device powers on).
// If we didn't do this highly unusual thing here, the display wouldn't come on
// when you cause the system to wakeup using this driver.

#pragma LOCKEDCODE

VOID RestoreDeviceContext(PDEVICE_OBJECT fdo, DEVICE_POWER_STATE oldstate, DEVICE_POWER_STATE newstate, PVOID context)
	{							// RestoreDeviceContext
	PoSetSystemState(ES_USER_PRESENT);
	GenericSaveRestoreComplete(context);
	}							// RestoreDeviceContext

///////////////////////////////////////////////////////////////////////////////
// The USB bus driver calls this callback routine to depower our device

#pragma PAGEDCODE

VOID SelectiveSuspendCallback(PDEVICE_EXTENSION pdx)
	{							// SelectiveSuspendCallback
	KdPrint((DRIVERNAME " - Selective suspend callback\n"));
	
	// Make sure any necessary wait-wake gets issued.

	GenericWakeupControl(pdx->pgx, ManageWaitWake);

	// Depower the device. In this driver, we're picking D3 unless wakeup is
	// enabled, in which case we pick D2. Note: these will *always* be the right values
	// for a USB device, which is the only kind of device for which this
	// selective suspend protocol is ever used.

	GenericIdleDevice(pdx->pgx, pdx->wakeup ? PowerDeviceD2 : PowerDeviceD3, TRUE);	// wait for it to finish
	}							// SelectiveSuspendCallback

///////////////////////////////////////////////////////////////////////////////
// This is the I/O completion routine for the selective-suspend IRP. Its job is
// to free the IRP, which we created as an asynchronous IRP. See the discussion in 
// Chapter 5 about how to safely cancel an async IRP for an explanation...

#pragma LOCKEDCODE

NTSTATUS SelectiveSuspendCompletionRoutine(PDEVICE_OBJECT junk, PIRP Irp, PDEVICE_EXTENSION pdx)
	{							// SelectiveSuspendCompletionRoutine
	NTSTATUS status = Irp->IoStatus.Status;
	KdPrint((DRIVERNAME " - SUBMIT_IDLE_NOTIFICATION request completing - %X\n", status));

	// Free the IRP safely.

	if (InterlockedExchangePointer((PVOID*) &pdx->SuspendIrp, NULL)
		|| InterlockedExchange(&pdx->SuspendIrpCancelled, 1))
		IoFreeIrp(Irp);

	// If the IRP finished with an error status other than STATUS_POWER_STATE_INVALID,
	// repower the device because the hub driver may have left it depowered.

	if (!NT_SUCCESS(status) && status != STATUS_POWER_STATE_INVALID)
		GenericWakeupFromIdle(pdx->pgx, FALSE);	// mustn't block if at DISPATCH_LEVEL

	return STATUS_MORE_PROCESSING_REQUIRED;
	}							// SelectiveSuspendCompletionRoutine

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS StartDevice(PDEVICE_OBJECT fdo, PCM_PARTIAL_RESOURCE_LIST raw, PCM_PARTIAL_RESOURCE_LIST translated)
	{							// StartDevice
	PAGED_CODE();
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	// Examine device capabilities.

	PDEVICE_CAPABILITIES pdc = GenericGetDeviceCapabilities(pdx->pgx);
	if (pdc)
		{
		if (pdc->SystemWake <= PowerSystemWorking)
			{
			KdPrint((DRIVERNAME " - System wakeup not supported on this platform\n"));
			if (pdc->DeviceWake > PowerDeviceD0)
				KdPrint((DRIVERNAME " - However, this device can wake itself while system is working\n"));
			}
		if (pdc->DeviceWake == PowerDeviceUnspecified)
			KdPrint((DRIVERNAME " - Device wakeup not supported\n"));
		}

	// Use GENERIC to issue (or not) a WAIT_WAKE.

	GenericWakeupControl(pdx->pgx, pdx->wakeup ? EnableWakeup : DisableWakeup);

	// Set the initial power state of the device

	if (pdx->autopower && !pdx->wakeup)
		IdleDevice(pdx);

	return STATUS_SUCCESS;
	}							// StartDevice

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID StopDevice(IN PDEVICE_OBJECT fdo, BOOLEAN oktouch /* = FALSE */)
	{							// StopDevice
	PAGED_CODE();
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	// Disable wakeup while stopped. This is a requirement for the USB stack,
	// and possibly others too.

	GenericWakeupControl(pdx->pgx, DisableWakeup);

	// Cancel any selective-suspend IRP that might be outstanding

	CancelSelectiveSuspend(pdx);
	}							// StopDevice
