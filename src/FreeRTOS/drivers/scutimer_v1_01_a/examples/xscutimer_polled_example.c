/* $Id: xscutimer_polled_example.c,v 1.1.2.1 2011/01/20 04:01:52 sadanan Exp $ */
/******************************************************************************
*
* (c) Copyright 2010 Xilinx, Inc. All rights reserved.
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
* @file  xscutimer_polled_example.c
*
* This file contains a design example using the Scu Private Timer driver
* (XScuTimer) and hardware timer device. This test assumes Auto Reload mode is
* not enabled.
*
* @note		None.
*
* MODIFICATION HISTORY:
*
*<pre>
* Ver   Who  Date     Changes
* ----- ---- -------- ---------------------------------------------
* 1.00a nm   03/10/10 First release
*</pre>
******************************************************************************/

/***************************** Include Files *********************************/

#include "xparameters.h"
#include "xscutimer.h"
#include "xil_printf.h"

/************************** Constant Definitions *****************************/

/*
 * The following constants map to the XPAR parameters created in the
 * xparameters.h file. They are only defined here such that a user can easily
 * change all the needed parameters in one place.
 */
#define TIMER_DEVICE_ID		XPAR_XSCUTIMER_0_DEVICE_ID
#define TIMER_LOAD_VALUE	0xFF

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

int ScuTimerPolledExample(u16 DeviceId);

/************************** Variable Definitions *****************************/

XScuTimer Timer;		/* Cortex A9 SCU Private Timer Instance */

/*****************************************************************************/
/**
*
* Main function to call the Scu Private Timer polled mode example.
*
* @param	None.
*
* @return	XST_SUCCESS if successful, XST_FAILURE if unsuccessful.
*
* @note		None.
*
******************************************************************************/
#ifndef TESTAPP_GEN
int main(void)
{
	int Status;

	xil_printf("SCU Timer Polled Mode Example Test \r\n");

	/*
	 * Call the polled example, specify the device ID that is generated in
	 * xparameters.h.
	 */
	Status = ScuTimerPolledExample(TIMER_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("SCU Timer Polled Mode Example Test Failed\r\n");
		return XST_FAILURE;
	}

	xil_printf("Successfully ran SCU Timer Polled Mode Example Test\r\n");
	return XST_SUCCESS;
}
#endif

/*****************************************************************************/
/**
*
* This function does a minimal test on the SCU Private timer device and driver.
* The purpose of this function is to illustrate how to use the XScuTimer driver.
*
* @param	DeviceId is the unique device id of the device.
*
* @return	XST_SUCCESS if successful, otherwise XST_FAILURE.
*
* @note		None.
*
****************************************************************************/
int ScuTimerPolledExample(u16 DeviceId)
{
	int Status;
	volatile u32 CntValue1 = 0;
	volatile u32 CntValue2 = 0;
	XScuTimer_Config *ConfigPtr;
	XScuTimer *TimerInstancePtr = &Timer;

	/*
	 * Initialize the Scu Private Timer so that it is ready to use.
	 */
	ConfigPtr = XScuTimer_LookupConfig(DeviceId);

	/*
	 * This is where the virtual address would be used, this example
	 * uses physical address.
	 */
	Status = XScuTimer_CfgInitialize(TimerInstancePtr, ConfigPtr,
				 ConfigPtr->BaseAddr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Load the timer counter register.
	 */
	XScuTimer_LoadTimer(TimerInstancePtr, TIMER_LOAD_VALUE);

	/*
	 * Get a snapshot of the timer counter value before it's started
	 * to compare against later.
	 */
	CntValue1 = XScuTimer_GetCounterValue(TimerInstancePtr);

	/*
	 * Start the Scu Private Timer device.
	 */
	XScuTimer_Start(TimerInstancePtr);

	/*
	 * Read the value of the timer counter and wait for it to change,
	 * since it's decrementing it should change, if the hardware is not
	 * working for some reason, this loop could be infinite such that the
	 * function does not return.
	 */
	while (1) {
		CntValue2 = XScuTimer_GetCounterValue(TimerInstancePtr);
		if (CntValue1 != CntValue2) {
			break;
		}
	}
	return XST_SUCCESS;
}
