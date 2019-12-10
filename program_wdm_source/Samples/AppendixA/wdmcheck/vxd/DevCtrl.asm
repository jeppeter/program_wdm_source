;	DevCtrl.asm -- Device control procedure for WDMCHECK.VXD
;	Copyright (C) 2000 by Walter Oney
;	All rights reserved

	.386p
	.nolist
	include vmm.inc
	include debug.inc
	include version.inc

	.list

byp equ <byte ptr>
wp equ <word ptr>
dwp equ <dword ptr>
fwp equ <fword ptr>

VxD_LOCKED_DATA_SEG
didinit  dd    0
exitlist dd		0
VxD_LOCKED_DATA_ENDS

_init segment dword public flat 'icode'
beginit  dd    0
_init ends

_einit  segment dword public flat 'icode'
endinit  dd    0
_einit  ends

_bss	 segment dword public flat 'lcode'
startbss dd    0
_bss	 ends

_ebss	 segment dword public flat 'lcode'
endbss	 dd    0
_ebss ends

Declare_Virtual_Device WDMCHECK, VERMAJOR, VERMINOR, WDMCHECK_control,\
	Undefined_Device_ID, Undefined_Init_Order,\
	, \
	, \

;------------------------------------------------------------------------------
;    Control function:
;------------------------------------------------------------------------------

Begin_Control_Dispatch WDMCHECK

Control_Dispatch Sys_Dynamic_Device_Init, SysDynamicDeviceInit
Control_Dispatch Sys_Dynamic_Device_Exit, SysDynamicDeviceExit
Control_Dispatch W32_DeviceIoControl, _OnW32DeviceIoControl, cCall, <esi>


End_Control_Dispatch WDMCHECK

;------------------------------------------------------------------------------
; C++ infrastructure functions:
;------------------------------------------------------------------------------

BeginProc __purecall, public, locked
	 Trace_Out "WDMCHECK: Call to pure virtual function!"
oops:	 int   1
	 jmp   oops
EndProc __purecall

BeginProc atexit, public, pageable, cCall
	 ArgVar	pFunc, dword

	 EnterProc

	 SaveReg esi

	 mov	esi, exitlist		; allocate element from the list
	 VMMCall List_Allocate		;   ..
	 jc		atexit_fail			; skip ahead if error
	 mov	ecx, pFunc			; save exit function pointer
	 mov	[eax], ecx			;  ..
	 VMMCall List_Attach_Tail	; attach this node to end of list

	 RestoreReg esi

	 xor	eax, eax			; indicate success

atexit_goback:	 
	 LeaveProc
	 Return

atexit_fail:
	 xor	eax, eax			; return -1 to indicate failure
	 dec	eax
	 jmp	atexit_goback
EndProc atexit

BeginProc DestructObjects, pageable
	 mov	esi, exitlist		; get pointer to list handle

destruct_next:
	 VMMCall List_Remove_First	; get & remove 1st element from list
	 jz		destruct_done		; skip ahead when all done
	 push	eax					; save list element ptr
	 call	near ptr [eax]		; call exit fcn
	 pop	eax					; restore element ptr
	 VMMCall List_Deallocate	; return element to free list
	 jmp	destruct_next		; continue loop

destruct_done:
	 VMMCall List_Destroy		; destroy the list now
	 ret						; return to caller
EndProc	DestructObjects

;------------------------------------------------------------------------------
; Initialization function for the driver:
;------------------------------------------------------------------------------

BeginProc	Initialize, init

	 bts	didinit, 0				; did we already initialize?
	 jnc	init_1						; if yes, don't do so again
	 clc							; indicate success
	 ret							; return to message handler
init_1:

	 pushad							; save all registers

;	 Create the list object used to record exit processors

	 mov	eax, LF_Swap		; list can be swappable
	 mov	ecx, 4				; each list entry is big enough for a pointer
	 VMMCall List_Create
	 jnc	init_2				; skip if no error
	 popad						; restore registers
	 ret						; return with CF set to indicate eror
init_2:
	 mov	exitlist, esi		; save list handle

;    The linker won't necessarily generate zero data for the BSS section,
;    which is where the compiler places uninitialized static data. Zero
;    this area by hand, therefore.

	 cld							; force forward direction
	 mov   edi, offset32 startbss	; point to start of BSS area
	 mov   ecx, offset32 endbss		; compute length
	 sub   ecx, edi					; ..
	 shr   ecx, 2					; convert to dwords
	 xor   eax, eax					; get const zero
	 rep   stosd					; zero-fill BSS area

;    Execute static initialization code

	 mov   esi, offset32 beginit+4	; point to 1st initializer ptr
init_3:
	 cmp   esi, offset32 endinit	; reached end of initializers?
	 jae   init_4					; if yes, leave the loop
	 call  dword ptr [esi]			; call static initializer
	 add   esi, 4					; process all of them
	 jmp   init_3						;   ..
init_4:											

	 popad							; restore registers
	 clc							; indicate no error
	 ret							; return to local caller

EndProc	Initialize

;------------------------------------------------------------------------------
; Initialization and shutdown message handlers
;------------------------------------------------------------------------------

BeginProc SysDynamicDeviceInit, init

		call	Initialize			; initialize driver on first call
		jc		dyninit_1			; fail message if can't initialize

		extern	_OnSysDynamicDeviceInit:near
		call	_OnSysDynamicDeviceInit	; call C/C++ message handler
		cmp		eax, 1				; did function succeed?
		jnc		dyninit_1			; if yes, good
		call	DestructObjects		; no. cleanup constructed objects
		stc							; set carry to indicate an error occurred

dyninit_1:
		ret							; return to VMM with CF set if error, clear if none
EndProc SysDynamicDeviceinit

BeginProc SysDynamicDeviceExit, pageable
		extern	_OnSysDynamicDeviceExit:near	; call C/C++ message handler
		call	_OnSysDynamicDeviceExit
		cmp		eax, 1				; did function succeed?
		jc		exitfail			; if not, skip shutdown because we won't unload
		btr		didinit, 0			; have we already terminated?
		jc		dynexit_1			; if yes, don't do it twice
		call	DestructObjects		; destruct static objects
dynexit_1:
		clc							; indicate no error

exitfail:
		ret							; return with CF set if error, clear if not
EndProc SysDynamicDeviceExit


	end
