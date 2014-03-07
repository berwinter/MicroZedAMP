#include <stdio.h>
#include <string.h>

#include "latencydemo.h"
#include "latencygraph.h"
#include "latencyrpmsg.h"

void print_graph_formatted(struct histogram* hist);

/* Clock frequency and time macros */
#define CLK_FREQ			111111115UL
#define CLK_SCALAR			1UL
#define CLK_FREQ_CURRENT	((unsigned long long)CLK_FREQ / CLK_SCALAR)
#define CLK_TIME_NSEC(x)	(((unsigned long long)x)*1000*1000*1000 / (CLK_FREQ_CURRENT))
#define CLK_TIME_USEC(x)	(((unsigned long long)x)*1000*1000 / (CLK_FREQ_CURRENT))

/* Dump the binary data in word groupings, display in hex */
static void dump_buffer(char *buf, int size)
{
	int i;
	int *array = (int *)buf;
	int array_size = size / 4;

	printf("\t");
	for (i = 0; i < array_size; i++)
	{
		if (i > 0 && !(i % 8))
			printf("\n\t");
		printf("%08x ", array[i]);
	}
	printf("\n");
}

void print_help(void)
{
	printf("latencystat - Zynq FreeRTOS AMP Latency Demo\n");
	printf("\n");
	printf("\t -g     Displays a terminal graph of the data\n");
	printf("\t        (requires a UTF8 terminal)\n");
	printf("\t -b     Displays a listing of buckets and values\n");
	printf("\t -d     Displays a binary data dump\n");
	printf("\t -h     Displays this help message\n");
}

int main(int argc, char** argv)
{
	struct histogram hist;
	struct rpmsg_target rpmsg0;

	unsigned int display_graph = 0;
	unsigned int display_buckets = 0;
	unsigned int display_binary = 0;
	int i;

	/* argument parsing */
	for (i = 1; i < argc; i++)
	{
		/* if '-g' arg is supplied draw the graph */
		if (strcmp(argv[i], "-g") == 0) {
			display_graph = 1;
		} else if (strcmp(argv[i], "-b") == 0) {
			display_buckets = 1;
		} else if (strcmp(argv[i], "-d") == 0) {
			display_binary = 1;
		} else if (strcmp(argv[i], "-h") == 0) {
			print_help();
			return 0;
		}
	}

	/* Check if anything to display */
	if (display_binary == 0 && display_buckets == 0 && display_graph == 0) {
		print_help();
		return 0;
	}

	/* Open the rpmsg device for sending messages to FreeRTOS */
	if (rpmsg_open_device(&rpmsg0, "/dev/rpmsg0") < 0) {
		return -1;
	}

	printf("Linux FreeRTOS AMP Demo.\n");

	/* Clear the FreeRTOS state */
	rpmsg_send_message(&rpmsg0, CLEAR); /* Clear statistic buffer */
	rpmsg_send_message(&rpmsg0, START); /* Start statistic task */

	printf("Waiting for samples...\n");
	sleep(10); /* wait a bit */

	/* No more samples, stop the FreeRTOS task */
	rpmsg_send_message(&rpmsg0, STOP);

	/* Copy the data across */
	rpmsg_send_message(&rpmsg0, CLONE); /* Create snapshot */
	rpmsg_send_message(&rpmsg0, GET); /* Ask for getting statistic */
	rpmsg_read_response(&rpmsg0, (char *)&hist, sizeof(struct histogram));

	/* Display the graph */
	if (display_graph) {
		printf("-----------------------------------------------------------\n");
		printf("Histogram Graph:\n");
		print_graph_formatted(&hist);
	}
	/* Display the binary data */
	if (display_binary) {
		printf("-----------------------------------------------------------\n");
		printf("Histogram Binary Data:\n");
		dump_buffer((char *)&hist, sizeof(struct histogram));
	}
	/* Display the bucket values */
	if (display_buckets) {
		printf("-----------------------------------------------------------\n");
		printf("Histogram Bucket Values:\n");
		int i;
		for (i = 0; i < HISTOGRAM_SIZE; i++) {
			if (hist.data[i] != 0) {
				printf("\tBucket %llu ns (%d ticks) had %u frequency\n",
					CLK_TIME_NSEC(i), i, hist.data[i]);
			}
		}
	}
	/* Display general data */
	printf("-----------------------------------------------------------\n");
	printf("Histogram Data:\n");
	printf("\tmin: %llu ns (%u ticks)\n", CLK_TIME_NSEC(hist.min), hist.min);
	printf("\tavg: %llu ns (%llu ticks)\n",
			CLK_TIME_NSEC(hist.total_sum / hist.sample_count),
			hist.total_sum / hist.sample_count);
	printf("\tmax: %llu ns (%u ticks)\n", CLK_TIME_NSEC(hist.max), hist.max);
	printf("\tout of range: %llu\n", hist.out_count);
	printf("\ttotal samples: %llu\n", hist.sample_count);
	printf("-----------------------------------------------------------\n");

	/* All done, close and clean up */
	rpmsg_close_device(&rpmsg0);
	return 0;
}

/*
 * Reformat the histogram data and print using the latencygraph.c implementation
 */
void print_graph_formatted(struct histogram* hist)
{
	struct histogram_values data;
	unsigned int databuf[51];

	memset(databuf, 0, sizeof(databuf));
	data.data = databuf;

	/* down convert and reduce buckets by divider.
	 *
	 * Convert the ticks based histogram into a latency in nanoseconds, and
	 * seperate the bukets at the ns ranges.
	 */
	data.bucket_min = 0;
	data.bucket_max = 1000;
	data.bucket_value = 20;
	data.bucket_increment = 100;
	data.buckets = 51;
	data.bucket_label_unit = " ns";

	data.value_min = 0;
	data.value_max = 100;
	data.value_increment = 20;
	data.value_label = 1;
	data.value_label_unit = NULL;

	/* populate data, into rounded buckets */
	int i;
	for (i = 0; i < HISTOGRAM_SIZE; i++) {
		if (hist->data[i] != 0) {
			unsigned long long value = CLK_TIME_NSEC(i);
			unsigned int value_index = (unsigned int)(value / data.bucket_value);
			if (value_index < data.buckets) {
				databuf[value_index] += hist->data[i];
			}
		}
	}

	print_graph(&data);
}