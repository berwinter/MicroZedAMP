/* Copyright (C) 2012 Xilinx Inc. */

/*
 * This Header File is common for both the FreeRTOS demo application and the 
 * latencystat user space demo application.
 */

#ifndef LATENCYDEMO_H
#define LATENCYDEMO_H

typedef enum {
	CLEAR = 0,
	START,
	STOP,
	CLONE,
	GET,
	QUIT,
	STATE_MASK = 0xF,
} latency_demo_msg_type;

/* Number of data pieces stored in the histogram */
#define HISTOGRAM_SIZE 1000

/* Histogram Structure */
struct histogram
{
	/* Total number of samples taken, including out of bounds */
	unsigned volatile long long sample_count;
	/* The number of sample that where taken that were not within the bounds of
	 * the histogram */
	unsigned volatile long long out_count;
	/* The total sum of all samples */
	unsigned volatile long long total_sum;
	/* Minimium sample value */
	unsigned volatile int min;
	/* Maximum sample value */
	unsigned volatile int max;
	/* The histogram values */
	unsigned volatile int data[HISTOGRAM_SIZE];
};

#endif /* LATENCYDEMO_H */
