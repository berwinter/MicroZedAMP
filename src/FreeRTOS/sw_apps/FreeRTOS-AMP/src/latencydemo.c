/* Copyright (C) 2012 Xilinx Inc. */

/*
 * The 'latencydemo' FreeRTOS application is comprised of three parts:
 * Remoteproc, IRQ Latency Measurement and a Demonstration Task.
 *
 * Remoteproc:
 * -----------
 * The remoteproc part of this application allows communication between the
 * Linux system and the FreeRTOS application which is running alongside in a AMP
 * mode. The remoteproc part of this application in implemented in the
 * 'remoteproc.c', 'remoteproc.h' and 'remoteproc_kernel.h' source files.
 *
 * The message passing interface (rpmsg) is used to communicate with the 
 * latency demo FreeRTOS application through the use of vring buffers. In this
 * specific application the demo only responds to requests made by Linux. These
 * requests are detailed in the 'message_handler' function of this file.
 *
 * The remoteproc section of this application is also responsible for setting up
 * the AMP state of the system to allow for access to shared memory as well as
 * configuring the system to allow for access to specific hardware peripherals
 * (e.g. the CPU private timer (scutimer) or the Triple Timer Counter (TTC)).
 *
 * IRQ Latency Measurement:
 * ------------------------
 * The IRQ latency measurement part of this application samples IRQ latency by
 * the use of the Triple Timer Counter (TTC).
 *
 * IRQ samples are measured using the timer, it is setup to trigger on overflow.
 * Once overflow is hit the timer remains counting, once the IRQ is triggered
 * and the IRQ service function is executed the timers current value is sampled
 * immediately. The value of the timer will be the number of ticks since the
 * actual IRQ was triggered in hardware.
 *
 * The IRQ samples are populated into a histogram table, including min, max and
 * total sum. This data structure is available for access via the remoteproc
 * messaging interface. The messaging interface also allows for the start, stop
 * and clearing of the sampling process/data.
 *
 * The sampling is setup to run as a FreeRTOS task.
 *
 * Demonstration Task:
 * -------------------
 * The demonstration task is a simple task that has a specific execution delay.
 * Once the task is started, it will begin by sampling the current time (in
 * ticks) and issue a FreeRTOS scheduling delay, upon which the task will resume
 * and re-sample the current time. If the time in which the task was sleeping is
 * different then the request sleep time, a message is printed informing of
 * this, otherwise a message is printed to inform that the task was resumed as
 * expected.
 * 
 * This task is to demonstrate that FreeRTOS is able to schedule the remoteproc,
 * IRQ latency measurement all while maintaining the expected scheduling jitter.
 */

#include <stdlib.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "xil_printf.h"
#include "xil_cache.h"
#include "xil_cache_l.h"
#include "semphr.h"

#include "remoteproc.h"
#include "latencydemo.h"

/* trace() prints to trace buffer */
#define trace(x)		xputs(x)
/* console() prints to serial device */
#define console(x)		xil_printf(x)
#define log(x)			trace(x)

/* -------------------------------------------------------------------------- */

/* Semaphores for synchronisation on memory objects */
xSemaphoreHandle wait_for_irq; /* used to synchronise on irq */
xSemaphoreHandle wait_for_clone; /* used to protect the histogram data */

/* Sample Data, and clone register for sampled data */
struct histogram* hist = 0;
struct histogram* hist_clone = 0;
/* Flag to enable/disable sampling of data */
unsigned volatile int histogram_enable = 0;

/* Clear the Data */
void clear_histogram()
{
	memset(hist, 0, sizeof(struct histogram));
	hist->min = 0xffffffff; /* invalid minimum */
	Xil_L1DCacheFlush();
}

struct ttc_timer
{
	unsigned volatile int clock_control[3];
	unsigned volatile int counter_control[3];
	unsigned volatile int counter_value[3];
	unsigned volatile int interval_counter[3];
	unsigned volatile int match_counter[3][3];
	unsigned volatile int interrupt_register[3];
	unsigned volatile int interrupt_enable[3];
	unsigned volatile int event_control_timer[3];
	unsigned volatile int event_register[3];
};

#define TTC_CHANNEL0			0
#define TTC_CHANNEL1			1
#define TTC_CHANNEL2			2
#define TTC_SAMPLE_CHANNEL		TTC_CHANNEL1

struct ttc_timer* ttc = (struct ttc_timer*)TTC_BASEADDR;

/* -------------------------------------------------------------------------- */

/* interrupt handler function */
void irq70_ttc(void* data)
{
	/* retrieve the current value of the counter */
	unsigned int cnt_value = ttc->counter_value[TTC_SAMPLE_CHANNEL];
	cnt_value &= 0xffff; /* mask the 16-bits */

	/* Disable timer */
	ttc->counter_control[TTC_SAMPLE_CHANNEL] = 0x1; /* disable counter */
	ttc->interrupt_enable[TTC_SAMPLE_CHANNEL] = 0x0; /* disable irq */
	ttc->interrupt_register[TTC_SAMPLE_CHANNEL] =
				ttc->interrupt_register[TTC_SAMPLE_CHANNEL]; /* clear irq */

	/* test min/max */
	if (cnt_value > hist->max)
		hist->max = cnt_value;
	if (cnt_value < hist->min)
		hist->min = cnt_value;

	hist->total_sum += cnt_value;
	hist->sample_count++;

	if (cnt_value > HISTOGRAM_SIZE) {
		/* value is outside the range of the histogram, count it separately */
		hist->out_count++;
	} else {
		/* increment histogram value */
		hist->data[cnt_value]++;
	}

	/* Flush cache */
	Xil_L1DCacheFlush();

	/* trigger ISR waiting semaphore */
	signed portBASE_TYPE xHigherPriorityTaskWoken;
	xSemaphoreGiveFromISR(wait_for_irq, &xHigherPriorityTaskWoken);
}

/* -------------------------------------------------------------------------- */

/* Wait and lock clone mutex */
static void clone_lock_mutex()
{
	int val;
	do {
		val = xSemaphoreTake(wait_for_clone, portMAX_DELAY);
	} while (val == pdFALSE);
}

/* Release clone mutex */
static void clone_unlock_mutex()
{
	xSemaphoreGive(wait_for_clone);
}

/* Latency Sampler Task */
static void task_latency( void *pvParameters )
{
	/* Clear the histogram data */
	clear_histogram();

	/* Init semaphores for synchronisation. */
	vSemaphoreCreateBinary(wait_for_irq);
	vSemaphoreCreateBinary(wait_for_clone);
	if (wait_for_irq == NULL || wait_for_clone == NULL) {
		log("task_latency: Unable to create message semaphores.\r\n");
		while(1);
	}

	log("task_latency: starting sampling of irq latency\r\n");

	/* Init next - this only needs to be done once. */
	portTickType next;
	next = xTaskGetTickCount();

	long long sample_counter = 0;
	unsigned int sampling_running = 0;
	while (1)
	{
		if (histogram_enable) {
			if (!sampling_running) {
				sampling_running = 1;
				/* Lock the mutex for the entire sampling time, this is required
				 * as the time in which the ISR modifies the data is unknown.
				 */
				clone_lock_mutex();
			}
			/* this semaphore will block until the previous sample is taken,
			 * and will be released by the ISR for the next sample
			 *
			 * This delay is ~1000ms, this is to avoid a situation where the
			 * semaphore is locked forever, which causes the clone mutex to
			 * be always locked.
			 *
			 * The 'wait_for_irq' semaphore is used as a synchronisation method
			 * between the task and the IRQ routine.
			 *
			 * During the start of sampling the semaphore is free, and becomes
			 * locked immediately, then the timer is setup for a sample. This
			 * task does a short scheduling delay (to reduce the time waiting
			 * for the timer overflow), and then locks waiting for the semaphore
			 * to be released by the IRQ, once released the timer is reset and
			 * activated again to complete another sample. This continues until
			 * the sampling task is paused.
			 */
			if (xSemaphoreTake(wait_for_irq, 1000 / portTICK_RATE_MS) == pdTRUE) {
				/* once the semaphore is locked, access the timer and set it up */
				ttc->counter_control[TTC_SAMPLE_CHANNEL] = 0x10; /* reset counter */
				ttc->interrupt_enable[TTC_SAMPLE_CHANNEL] = 0x10; /* enable irq */
			} else {
				log("task_latency: failed to get semaphore\r\n");
			}
			

			/* Print out some info to inform that sampling is occurring */
			sample_counter++;
			if (sample_counter == (1000)) {
				sample_counter = 0;
				log("task_latency: sampled 1 full buffers\r\n");
			}
		} else {
			/* stop counter while sampling is disabled */
			if (sampling_running) {
				sampling_running = 0;
				/* wait for any existing samples to complete, once they have
				 * hold the semaphore whilst disabling the timer.
				 */
				xSemaphoreTake(wait_for_irq, 1000 / portTICK_RATE_MS);
				/* disable counter */
				ttc->counter_control[TTC_SAMPLE_CHANNEL] = 0x1;
				/* disable irq */
				ttc->interrupt_enable[TTC_SAMPLE_CHANNEL] = 0x0;
				/* release the semaphore so that when the sampling begins the
				 * process can resume correctly
				 */
				xSemaphoreGive(wait_for_irq);
				/* release the hold on the histogram data, since the sampling
				 * will not modify it any more.
				 */
				clone_unlock_mutex();
			}
		}
		/* wait between samples, in order to let the system schedule */
		vTaskDelayUntil( &next, 1 / portTICK_RATE_MS );
	}
}

/* -------------------------------------------------------------------------- */

/* Demo Task */
static void task_demo(void* pvParameters)
{
	portTickType next;
	portTickType delay;
	portTickType current_delay_start;
	portTickType current_delay_end;
	/* Init next - this only needs to be done once. */
	next = xTaskGetTickCount();
	delay = 100000 / portTICK_RATE_MS;

	log("task_demo: started\r\n");

	while (1)
	{
		current_delay_start = xTaskGetTickCount();
		vTaskDelayUntil(&next, delay);
		current_delay_end = xTaskGetTickCount();

		/* print out the difference between delay expected and actual */
		if ((current_delay_end - current_delay_start) != delay) {
			log("task_demo: task resumed out of expected boundary\r\n");
		} else {
			log("task_demo: task resumed as expected\r\n");
		}
	}
}

/* -------------------------------------------------------------------------- */

void message_handler(struct remoteproc_request* req, unsigned char* data,
		unsigned int len)
{
	switch (req->state)
	{
		case CLEAR:
			log("rpmsg: CLEAR request\r\n");
			clear_histogram(NULL);
			remoteproc_request_ack(req);
			break;
		case START:
			log("rpmsg: START request\r\n");
			histogram_enable = 1;
			remoteproc_request_ack(req);
			break;
		case STOP:
			log("rpmsg: STOP request\r\n");
			histogram_enable = 0;
			remoteproc_request_ack(req);
			break;
		case CLONE:
			log("rpmsg: CLONE request\r\n");
			clone_lock_mutex();
			memcpy(hist_clone, hist, sizeof(struct histogram));
			clone_unlock_mutex();
			remoteproc_request_ack(req);
			break;
		case GET:
			log("rpmsg: GET request\r\n");
			remoteproc_request_ack(req);
			remoteproc_request_response(req, (unsigned char*)hist_clone,
					sizeof(struct histogram));
			break;
		case QUIT:
			log("rpmsg: QUIT request\r\n");
			histogram_enable = 0;
			remoteproc_request_ack(req);
			break;
		default:
			log("rpmsg: Unimplemented request\r\n");
	}
}

/* -------------------------------------------------------------------------- */

void setup_handler(void)
{
	/* Setup remoteproc IRQs */
	remoteproc_init_irqs();

	/* This is the interrupt from the TTC1 - Channel #2
	 * necessary to stop it because if firmware failed counter can still work */
	ttc->clock_control[TTC_SAMPLE_CHANNEL] = 0x0; /* default clock */
	ttc->counter_control[TTC_SAMPLE_CHANNEL] = 0x10; /* reset counter */
	ttc->counter_control[TTC_SAMPLE_CHANNEL] = 0x1; /* stop counter */
	ttc->interrupt_register[TTC_SAMPLE_CHANNEL] =
			ttc->interrupt_register[TTC_SAMPLE_CHANNEL]; /* ACK pending IRQ */
	ttc->interrupt_enable[TTC_SAMPLE_CHANNEL] = 0;
	setupIRQhandler(70, &irq70_ttc, NULL);
}

extern void Init_Uart(unsigned int BaudRate);

int main(void)
{
	/* Init trace buffer */
	trace_init();

	/* MMU resource setup */
	mmu_resource_table_setup();

	/* Setup UART */
	Init_Uart(115200);

	/* Print Message */
	log("FreeRTOS main demo application " __DATE__ " " __TIME__ "\r\n");

	/* Allocate histogram structure */
	hist = (struct histogram*)malloc(sizeof (struct histogram));
	hist_clone = (struct histogram*)malloc(sizeof (struct histogram));
	if (hist == NULL || hist_clone == NULL) {
		log("ERROR: Failed to allocate memory!\r\n");
		return -1;
	}

	/* IRQ handler must be registered before vTaskStartScheduler */
	register_handler(&setup_handler);

	/* Init the remoteproc communication */
	remoteproc_init(&message_handler);

	/* Create sampler task */
	xTaskCreate(task_latency, (signed char*)"TIMER", configMINIMAL_STACK_SIZE,
			NULL, tskIDLE_PRIORITY + 3, NULL);
	/* Create demo task */
	xTaskCreate(task_demo, (signed char*)"TASKDEMO", configMINIMAL_STACK_SIZE,
			NULL, tskIDLE_PRIORITY + 3, NULL);

	/* Start the tasks */
	vTaskStartScheduler();

	/* If all is well, the scheduler will now be running, and the following line
	 * will never be reached. If the following line does execute, then there
	 * was insufficient FreeRTOS heap memory available for the idle and/or timer
	 * tasks to be created. See the memory management section on the FreeRTOS
	 * web site for more details. */
	while (1);

	/* Should never get here */
	free(hist);
	free(hist_clone);
}

/* -------------------------------------------------------------------------- */

void vApplicationMallocFailedHook(void)
{
	/* vApplicationMallocFailedHook() will only be called if
	 * configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h. It is a
	 * hook function that will get called if a call to pvPortMalloc() fails.
	 * pvPortMalloc() is called internally by the kernel whenever a task, queue
	 * or semaphore is created. It is also called by various parts of the demo
	 * application. If heap_1.c or heap_2.c are used, then the size of the heap
	 * available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
	 * FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
	 * to query the size of free heap space that remains (although it does not
	 * provide information on how the remaining heap might be fragmented). */
	taskDISABLE_INTERRUPTS();
	for( ;; );
}

/* -------------------------------------------------------------------------- */

void vApplicationStackOverflowHook(xTaskHandle* pxTask, signed char* pcTaskName)
{
	(void) pcTaskName;
	(void) pxTask;

	/* vApplicationStackOverflowHook() will only be called if
	 * configCHECK_FOR_STACK_OVERFLOW is set to either 1 or 2. The handle and
	 * name of the offending task will be passed into the hook function via its
	 * parameters. However, when a stack has overflowed, it is possible that the
	 * parameters will have been corrupted, in which case the pxCurrentTCB
	 * variable can be inspected directly. */
	taskDISABLE_INTERRUPTS();
	for( ;; );
}
