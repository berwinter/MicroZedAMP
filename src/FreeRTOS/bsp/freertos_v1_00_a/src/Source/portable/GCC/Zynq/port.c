/*
    FreeRTOS V7.0.2 - Copyright (C) 2012 PetaLogix Qld Pty Ltd.
                      Copyright (C) 2011 Real Time Engineers Ltd.

    ***************************************************************************
    *                                                                         *
    * If you are:                                                             *
    *                                                                         *
    *    + New to FreeRTOS,                                                   *
    *    + Wanting to learn FreeRTOS or multitasking in general quickly       *
    *    + Looking for basic training,                                        *
    *    + Wanting to improve your FreeRTOS skills and productivity           *
    *                                                                         *
    * then take a look at the FreeRTOS eBook                                  *
    *                                                                         *
    *        "Using the FreeRTOS Real Time Kernel - a Practical Guide"        *
    *                  http://www.FreeRTOS.org/Documentation                  *
    *                                                                         *
    * A pdf reference manual is also available.  Both are usually delivered   *
    * to your inbox within 20 minutes to two hours when purchased between 8am *
    * and 8pm GMT (although please allow up to 24 hours in case of            *
    * exceptional circumstances).  Thank you for your support!                *
    *                                                                         *
    ***************************************************************************

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    ***NOTE*** The exception to the GPL is included to allow you to distribute
    a combined work that includes FreeRTOS without being obliged to provide the
    source code for proprietary components outside of the FreeRTOS kernel.
    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public
    License and the FreeRTOS license exception along with FreeRTOS; if not it
    can be viewed here: http://www.freertos.org/a00114.html and also obtained
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!

    http://www.FreeRTOS.org - Documentation, latest information, license and
    contact details.

    http://www.SafeRTOS.com - A version that is certified for use in safety
    critical systems.

    http://www.OpenRTOS.com - Commercial support, development, porting,
    licensing and training services.
*/


/*-----------------------------------------------------------
 * Implementation of functions defined in portable.h for the ARM7 port.
 *
 * Components that can be compiled to either ARM or THUMB mode are
 * contained in this file.  The ISR routines, which can only be compiled
 * to ARM mode are contained in portISR.c.
 *----------------------------------------------------------*/


/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"

/* xilinx includes */
#include "xparameters.h"
#include "xscutimer.h"
#include "xscugic.h"
#include "semphr.h"
#include "xil_exception.h"

#include "xil_printf.h"

/* Constants required to setup the task context. */
#define portINITIAL_SPSR				( ( portSTACK_TYPE ) 0x1f ) /* System mode, ARM mode, interrupts enabled. */
#define portTHUMB_MODE_BIT				( ( portSTACK_TYPE ) 0x20 )
#define portINSTRUCTION_SIZE			( ( portSTACK_TYPE ) 4 )
#define portNO_CRITICAL_SECTION_NESTING	( ( portSTACK_TYPE ) 0 )

#define XSCUTIMER_CLOCK_HZ 5000000
/*-----------------------------------------------------------*/

/* Setup the timer to generate the tick interrupts. */
static void prvSetupTimerInterrupt( void );

/*
 * The scheduler can only be started from ARM mode, so
 * vPortISRStartFirstSTask() is defined in portISR.c.
 */
extern void vPortISRStartFirstTask( void );

/*-----------------------------------------------------------*/

/*
 * Initialise the stack of a task to look exactly as if a call to
 * portSAVE_CONTEXT had been called.
 *
 * See header file for description.
 */
portSTACK_TYPE *pxPortInitialiseStack( portSTACK_TYPE *pxTopOfStack, pdTASK_CODE pxCode, void *pvParameters )
{
portSTACK_TYPE *pxOriginalTOS;

	pxOriginalTOS = pxTopOfStack;

	/* Setup the initial stack of the task.  The stack is set exactly as
	expected by the portRESTORE_CONTEXT() macro. */

	/* First on the stack is the return address - which in this case is the
	start of the task.  The offset is added to make the return address appear
	as it would within an IRQ ISR. */
	*pxTopOfStack = ( portSTACK_TYPE ) pxCode + portINSTRUCTION_SIZE;
	pxTopOfStack--;

	*pxTopOfStack = ( portSTACK_TYPE ) 0xaaaaaaaa;	/* R14 */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) pxOriginalTOS; /* Stack used when task starts goes in R13. */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) 0x12121212;	/* R12 */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) 0x11111111;	/* R11 */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) 0x10101010;	/* R10 */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) 0x09090909;	/* R9 */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) 0x08080808;	/* R8 */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) 0x07070707;	/* R7 */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) 0x06060606;	/* R6 */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) 0x05050505;	/* R5 */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) 0x04040404;	/* R4 */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) 0x03030303;	/* R3 */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) 0x02020202;	/* R2 */
	pxTopOfStack--;
	*pxTopOfStack = ( portSTACK_TYPE ) 0x01010101;	/* R1 */
	pxTopOfStack--;

	/* When the task starts is will expect to find the function parameter in
	R0. */
	*pxTopOfStack = ( portSTACK_TYPE ) pvParameters; /* R0 */
	pxTopOfStack--;

	/* The last thing onto the stack is the status register, which is set for
	system mode, with interrupts enabled. */
	*pxTopOfStack = ( portSTACK_TYPE ) portINITIAL_SPSR;

	if( ( ( unsigned long ) pxCode & 0x01UL ) != 0x00 )
	{
		/* We want the task to start in thumb mode. */
		*pxTopOfStack |= portTHUMB_MODE_BIT;
	}

	pxTopOfStack--;

	/* Some optimisation levels use the stack differently to others.  This
	means the interrupt flags cannot always be stored on the stack and will
	instead be stored in a variable, which is then saved as part of the
	tasks context. */
	*pxTopOfStack = portNO_CRITICAL_SECTION_NESTING;

	return pxTopOfStack;
}
/*-----------------------------------------------------------*/

void (*handler)(void) = NULL;

void register_handler(void *handler_priv)
{
	handler = handler_priv;
}

portBASE_TYPE xPortStartScheduler( void )
{
	/* Start the timer that generates the tick ISR.  Interrupts are disabled
	here already. */
	prvSetupTimerInterrupt();

	if(handler)
		handler();

	/* Start the first task. */
	vPortISRStartFirstTask();

	/* Should not get here! */
	return 0;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
	/* It is unlikely that the ARM port will require this function as there
	is nothing to return to.  */
}
/*-----------------------------------------------------------*/

static XScuGic   InterruptController;  /* Interrupt controller instance */
XScuTimer Timer;                /* A9 timer counter */

void *
portGetIntc ()
{
    return &InterruptController;
}

/* Does the provided frame pointer appear valid? */
static int ptr_valid(unsigned *fp)
{
    extern unsigned *_end;

    if (fp > _end) {
        return 0;
    }

    if (fp == 0) {
        return 0;
    }

    if ((unsigned)fp & 3) {
        return 0;
    }

    return 1;
}

/* MS: Trace buffer setting */
xSemaphoreHandle xStdioSemaphore;
char *log_buf_base;
unsigned int log_buf_len;
char *current;


static void stdio_lock_mutex()
{
    int val;

    do {
        val = xSemaphoreTake(xStdioSemaphore, portMAX_DELAY);
    } while (val == pdFALSE);
}

static void stdio_unlock_mutex()
{
    xSemaphoreGive(xStdioSemaphore);
}

void xputs(char *str)
{
//	vPortEnterCritical();
	int len = (int)strlen(str);

	if ((current + len) >= (log_buf_base + log_buf_len)) {
		int offset = log_buf_base + log_buf_len - current;
		strncpy(current, str, offset);
		current = log_buf_base;
		strncpy(current, str + offset, len - offset);
	} else {
		strncpy(current, str, len);
		current += len;
	}

	Xil_L1DCacheFlush();
//	vPortExitCritical();
}

void FreeRTOS_ExHandler(void *data);

static void freertos_exception_init(void)
{
	Xil_ExceptionInit();

	/*
	 * Connect the interrupt controller interrupt handler to the hardware
	 * interrupt handling logic in the ARM processor.
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_UNDEFINED_INT,
	                (Xil_ExceptionHandler)FreeRTOS_ExHandler,
	                (void *)4);

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_PREFETCH_ABORT_INT,
	                (Xil_ExceptionHandler)FreeRTOS_ExHandler,
	                (void *)4);

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_DATA_ABORT_INT,
	                (Xil_ExceptionHandler)FreeRTOS_ExHandler,
	                (void *)8);
}

void stdio_lock_init(unsigned int base, unsigned int len)
{
    freertos_exception_init();
    xStdioSemaphore = xSemaphoreCreateMutex();
    log_buf_base = (char *) base;
    log_buf_len = len;
    current = (char *) base;
}

/*
 * safe_printf:
 * use a semaphore to ensure the entire string prints without
 * interruption from other threads. FreeRTOS has no concept of a
 * controlling TTY per thread.
 */
void safe_printf(const char *format, ...)
{
    char string[100];
    va_list args;

    va_start(args, format );
    string[0] = 0;
    /* vprintf() not operational in Pele standalone libc.
     * Use an intermediate string instead.
     */
    vsnprintf(string, sizeof(string), format, args );
    xputs(string); /* MS just print to buffer */
    /* xil_printf("%s", string); */
    va_end( args );
}


#define mfsp(rn)       ({unsigned int rval; \
                          __asm__ __volatile__(\
                            "mov %0," stringify(rn) "\n"\
                            : "=r" (rval)\
                          );\
                          rval;\
                         })


/* Exception handler (fatal).
 * Attempt to print out a backtrace.
 */
void FreeRTOS_ExHandler(void *data)
{
    unsigned *fp, lr;
    static int exception_count = 0;
    int offset = (int)data;

    safe_printf("\n\rEXCEPTION, HALTED!\n\r");

    fp = (unsigned*)mfgpr(11); /* get current frame pointer */
/*    if (! ptr_valid(fp)) {
        goto spin;
    } */

    /* Fetch Data Fault Address from CP15 */
    lr = mfcp(XREG_CP15_DATA_FAULT_ADDRESS);
    safe_printf("Data Fault Address: 0x%08x\n\r", lr);

    /* The exception frame is built by DataAbortHandler (for example) in
     * FreeRTOS/Source/portable/GCC/Zynq/port_asm_vectors.s:
     * stmdb   sp!,{r0-r3,r12,lr}
     * and the initial handler function (i.e. DataAbortInterrupt() ) in
     * standalone_bsp/src/arm/vectors.c, which is the standard compiler EABI :
     * push    {fp, lr}
     *
     * The relative position of the frame build in port_asm_vectors.s is assumed,
     * as there is no longer any direct reference to it.  If this file (or vectors.c)
     * are modified this location will need to be updated.
     *
     * r0+r1+r2+r3+r12+lr = 5 registers to get to the initial link register where
     * the exception occurred.
     */
    safe_printf("FP: 0x%08x LR: 0x%08x\n\r", (unsigned)fp, *(fp + 5) - offset);
    safe_printf("R0: 0x%08x R1: 0x%08x\n\r", *(fp + 0), *(fp + 1));
    safe_printf("R2: 0x%08x R3: 0x%08x\n\r", *(fp + 2), *(fp + 3));
    safe_printf("R12: 0x%08x\n\r", *(fp + 4));
spin:
    exception_count++;
    if (exception_count > 1) {
        /* Nested exceptions */
        while (1) {;}
    }

    while (1) {;}
}

void clearIRQhandler(int int_no)
{
	XScuGic_Disable(&InterruptController, int_no);
	XScuGic_Disconnect(&InterruptController, int_no);
}

#define XSCUGIC_PENDING_CLR_OFFSET	0x00000280 /**< Pending Clear
							Register */


#define XScuGic_ClearPending(DistBaseAddress, Int_Id) \
	XScuGic_WriteReg((DistBaseAddress), \
			 XSCUGIC_PENDING_CLR_OFFSET + ((Int_Id / 32) * 4), \
			 (1 << (Int_Id % 32)))


void setupIRQhandler(int int_no, void *fce, void *param)
{
	int Status;

	clearIRQhandler(int_no);

	Status = XScuGic_Connect(&InterruptController, int_no,
	    (Xil_ExceptionHandler)fce, param);
	if (Status != XST_SUCCESS) {
	    return;
	}
	XScuGic_ClearPending(&InterruptController, int_no);
	XScuGic_Enable(&InterruptController, int_no);
}

void swirq_to_linux(int irq, int cpu)
{
	XScuGic_SoftwareIntr(&InterruptController, irq, cpu);
}

/*
 * Setup the A9 internal timer to generate the tick interrupts at the
 * required frequency.
 */
static void prvSetupTimerInterrupt( void )
{
	extern void vTickISR (void);
	int Status;
	XScuGic_Config *IntcConfig; /* The configuration parameters of the interrupt controller */
	XScuTimer_Config *ScuConfig;

	/*
	 * Initialize the interrupt controller driver
	 */
	IntcConfig = XScuGic_LookupConfig(XPAR_SCUGIC_SINGLE_DEVICE_ID);
	if (IntcConfig == NULL) {
	    return;
	}

	Status = XScuGic_CfgInitialize(&InterruptController, IntcConfig,
	                IntcConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
	    return;
	}


	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT,
	                (Xil_ExceptionHandler)XScuGic_InterruptHandler,
	                &InterruptController);

	/*
	 * Connect to the interrupt controller
	 */
	Status = XScuGic_Connect(&InterruptController, XPAR_SCUTIMER_INTR,
	    (Xil_ExceptionHandler)vTickISR, (void *)&Timer);
	if (Status != XST_SUCCESS) {
	    return;
	}

	/* Timer Setup */

	/*
	 * Initialize the A9Timer driver.
	 */
	ScuConfig = XScuTimer_LookupConfig(XPAR_SCUTIMER_DEVICE_ID);

	/*
	 * This is where the virtual address would be used, this example
	 * uses physical address.
	 */
	Status = XScuTimer_CfgInitialize(&Timer, ScuConfig,
	                ScuConfig->BaseAddr);
	if (Status != XST_SUCCESS) {
	    return;
	}

	/*
	 * Enable Auto reload mode.
	 */
	XScuTimer_EnableAutoReload(&Timer);

	/*
	 * Load the timer counter register.
	 */
	XScuTimer_LoadTimer(&Timer, XSCUTIMER_CLOCK_HZ / configTICK_RATE_HZ);

	/*
	 * Start the timer counter and then wait for it
	 * to timeout a number of times.
	 */
	XScuTimer_Start(&Timer);

	/*
	 * Enable the interrupt for the Timer in the interrupt controller
	 */
	XScuGic_Enable(&InterruptController, XPAR_SCUTIMER_INTR);

	/*
	 * Enable the timer interrupts for timer mode.
	 */
	XScuTimer_EnableInterrupt(&Timer);

	/*
	 * Do NOT enable interrupts in the ARM processor here.
	 * This happens when the scheduler is started.
	 */
}

/*-----------------------------------------------------------*/


