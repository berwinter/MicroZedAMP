/* $Id: $*/
/******************************************************************************
*
* (c) Copyright 2011 Xilinx, Inc. All rights reserved.
*
* This file contains confidential and proprietary information of Xilinx, Inc.
* and is protected under U.S. and international copyright and other
* intellectual property laws.
*
* DISCLAIMER
* This disclaimer is not a license and does not grant any rights to the
* materials distributed herewith. Except as otherwise provided in a valid
* license issued to you by Xilinx, and to the maximum extent permitted by
* applicable law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL
* FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS,
* IMPLIED, OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF
* MERCHANTABILITY, NON-INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE;
* and (2) Xilinx shall not be liable (whether in contract or tort, including
* negligence, or under any other theory of liability) for any loss or damage
* of any kind or nature related to, arising under or in connection with these
* materials, including for any direct, or any indirect, special, incidental,
* or consequential loss or damage (including loss of data, profits, goodwill,
* or any type of loss or damage suffered as a result of any action brought by
* a third party) even if such damage or loss was reasonably foreseeable or
* Xilinx had been advised of the possibility of the same.
*
* CRITICAL APPLICATIONS
* Xilinx products are not designed or intended to be fail-safe, or for use in
* any application requiring fail-safe performance, such as life-support or
* safety devices or systems, Class III medical devices, nuclear facilities,
* applications related to the deployment of airbags, or any other applications
* that could lead to death, personal injury, or severe property or
* environmental damage (individually and collectively, "Critical
* Applications"). Customer assumes the sole risk and liability of any use of
* Xilinx products in Critical Applications, subject only to applicable laws
* and regulations governing limitations on product liability.
*
* THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE
* AT ALL TIMES.
*
******************************************************************************/
/*****************************************************************************/
/**
* @file port_asm_vectors.s
*
* This file contains the initial vector table for the Cortex A9 processor
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who     Date     Changes
* ----- ------- -------- ---------------------------------------------------
* 1.00a ecm/sdm 10/20/09 Initial version
* </pre>
*
* @note
*
* None.
*
******************************************************************************/
.org 0
.text

.globl _boot

.globl _vector_table
.globl FIQInterrupt
.globl vFreeRTOS_IRQInterrupt
.globl vPortYieldProcessor
.globl DataAbortInterrupt
.globl PrefetchAbortInterrupt
.globl IRQHandler

.section .vectors,"ax"
	.code 32
_vector_table:
	B	  _boot
	B	  Undefined
	ldr   pc, _swi
	B	  PrefetchAbortHandler
	B	  DataAbortHandler
	NOP	  /* Placeholder for address exception vector*/
	ldr   pc, _irq
	B	  FIQHandler

_irq:   .word vFreeRTOS_IRQInterrupt
_swi:   .word vPortYieldProcessor

FIQHandler:					/* FIQ vector handler */
	stmdb	sp!,{r0-r3,r12,lr}		/* state save from compiled code */

FIQLoop:
	bl	FIQInterrupt			/* FIQ vector */

	ldmia	sp!,{r0-r3,r12,lr}		/* state restore from compiled code */
	subs	pc, lr, #4			/* adjust return */


Undefined:					/* Undefined handler */
	stmdb	sp!,{r0-r3,r12,lr}		/* state save from compiled code */

	ldmia	sp!,{r0-r3,r12,lr}		/* state restore from compiled code */

	b	_prestart			

	movs	pc, lr


DataAbortHandler:				/* Data Abort handler */
	stmdb	sp!,{r0-r3,r12,lr}		/* state save from compiled code */

	movs	r11, sp

	bl	DataAbortInterrupt		/*DataAbortInterrupt :call C function here */

	ldmia	sp!,{r0-r3,r12,lr}		/* state restore from compiled code */

	subs	pc, lr, #4			/* adjust return */

PrefetchAbortHandler:				/* Prefetch Abort handler */
	stmdb	sp!,{r0-r3,r12,lr}		/* state save from compiled code */

	bl	PrefetchAbortInterrupt		/* PrefetchAbortInterrupt: call C function here */

	ldmia	sp!,{r0-r3,r12,lr}		/* state restore from compiled code */

	subs	pc, lr, #4			/* adjust return */


.end
