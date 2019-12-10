// Power request handler mulfunc driver for child device PDOs
// Copyright (C) 1999 by Walter Oney
// All rights reserved

#include "stddcls.h"
#include "driver.h"

static NTSTATUS DefaultPowerHandler(IN PDEVICE_OBJECT pdo, IN PIRP Irp);
static NTSTATUS HandleQueryPower(PDEVICE_OBJECT fdo, PIRP Irp);
static NTSTATUS HandleSetPower(IN PDEVICE_OBJECT pdo, IN PIRP Irp);
static NTSTATUS HandleWaitWake(IN PDEVICE_OBJECT pdo, IN PIRP Irp);
									  
///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

NTSTATUS DispatchPowerPdo(IN PDEVICE_OBJECT pdo, IN PIRP Irp)
	{							// DispatchPowerPdo
	PPDO_EXTENSION pdx = (PPDO_EXTENSION) pdo->DeviceExtension;

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	ASSERT(stack->MajorFunction == IRP_MJ_POWER);
	NTSTATUS status;

	static NTSTATUS (*fcntab[])(IN PDEVICE_OBJECT pdo, IN PIRP Irp) = {
		HandleWaitWake,				// IRP_MN_WAIT_WAKE
		DefaultPowerHandler,		// IRP_MN_POWER_SEQUENCE
		HandleSetPower,				// IRP_MN_SET_POWER
		HandleQueryPower,			// IRP_MN_QUERY_POWER
		};
	
	ULONG fcn = stack->MinorFunction;
	if (fcn >= arraysize(fcntab))
		return DefaultPowerHandler(pdo, Irp);

#if DBG
	static char* fcnname[] = {
		"IRP_MN_WAIT_WAKE",
		"IRP_MN_POWER_SEQUENCE",
		"IRP_MN_SET_POWER",
		"IRP_MN_QUERY_POWER",
		};

	if (fcn == IRP_MN_SET_POWER || fcn == IRP_MN_QUERY_POWER)
		{
		static char* sysstate[] = {
			"PowerSystemUnspecified",
			"PowerSystemWorking",
			"PowerSystemSleeping1",
			"PowerSystemSleeping2",
			"PowerSystemSleeping3",
			"PowerSystemHibernate",
			"PowerSystemShutdown",
			"PowerSystemMaximum",
			};

		static char* devstate[] = {
			"PowerDeviceUnspecified",
			"PowerDeviceD0",
			"PowerDeviceD1",
			"PowerDeviceD2",
			"PowerDeviceD3",
			"PowerDeviceMaximum",
			};

		POWER_STATE_TYPE type = stack->Parameters.Power.Type;

		KdPrint((DRIVERNAME " (PDO) - POWER Request (%s)", fcnname[fcn]));
		if (type == SystemPowerState)
			KdPrint((", SystemPowerState = %s\n", sysstate[stack->Parameters.Power.State.SystemState]));
		else
			KdPrint((", DevicePowerState = %s\n", devstate[stack->Parameters.Power.State.DeviceState]));
		}
	else
		KdPrint((DRIVERNAME " (PDO) - POWER Request (%s)\n", fcnname[fcn]));

#endif // DBG

	status = (*fcntab[fcn])(pdo, Irp);
	return status;
	}							// DispatchPowerPdo

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// The default handler for power IRPs completes the IRP with whatever status
// is already in the IRP. The Driver Verifier will ping a driver with a bogus
// IRP just to make sure you do this...

NTSTATUS DefaultPowerHandler(IN PDEVICE_OBJECT pdo, IN PIRP Irp)
	{							// DefaultPowerHandler
	PoStartNextPowerIrp(Irp);
	NTSTATUS status = Irp->IoStatus.Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
	}							// DefaultPowerHandler

///////////////////////////////////////////////////////////////////////////////
// This driver succeeds all power queries from child devices. In general,
// there would be no reason for a parent driver to fail a query. Some of
// Microsoft's drivers (USBHUB) fail queries that are inconsistent with the
// wake state of the child device, but this leads to problems.

static NTSTATUS HandleQueryPower(PDEVICE_OBJECT fdo, PIRP Irp)
	{							// HandleQueryPower
	PoStartNextPowerIrp(Irp);
	return CompleteRequest(Irp, STATUS_SUCCESS);
	}							// HandleQueryPower

///////////////////////////////////////////////////////////////////////////////
// This driver unconditionally succeeds SET_POWER requests. In a real driver,
// you might want to track the states of all the child devices in order to
// put the parent device in a sleep state consistent with the states of all the
// child devices.

static NTSTATUS HandleSetPower(IN PDEVICE_OBJECT pdo, IN PIRP Irp)
	{							// HandleSetPower
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	PoStartNextPowerIrp(Irp);
	if (stack->Parameters.Power.Type == DevicePowerState)
		PoSetPowerState(pdo, DevicePowerState, stack->Parameters.Power.State);
	return CompleteRequest(Irp, STATUS_SUCCESS);
	}							// HandleSetPower

///////////////////////////////////////////////////////////////////////////////
// This driver simply fails WAIT_WAKE requests. In a real driver, you would need
// to track these IRPs as described in Chapter 11 of the book.

static NTSTATUS HandleWaitWake(IN PDEVICE_OBJECT pdo, IN PIRP Irp)
	{							// HandleWaitWake
	PoStartNextPowerIrp(Irp);	// not needed, but can't hurt
	return CompleteRequest(Irp, STATUS_NOT_SUPPORTED, 0);	// this device lacks wakeup capability
	}							// HandleWaitWake
