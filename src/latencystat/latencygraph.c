#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "latencygraph.h"

void printf_spaces(int count);
void print_border(int cross, int vertical);
void print_block(int value, int vertical);

/*
 * Print the Graph using Block and Line characters.
 */
void print_graph(struct histogram_values* histogram)
{
	unsigned int left_margin = 1;
	/* each increment is 10 blocks */
	unsigned int haxis_value = histogram->value_increment / 10;
	unsigned int haxis_count = 0;
	unsigned int value = 0;
	unsigned int bucket_label_strlen = strlen(histogram->bucket_label_unit);

	/* draw the graph horizontal axis values */
	printf_spaces(left_margin - 4 + bucket_label_strlen);
	value = histogram->value_min;
	while (1) {
		if (value % histogram->value_increment == 0) {
			printf_spaces((histogram->value_increment / haxis_value) - 4);
			printf("%4d", value);
		}
		if (value >= histogram->value_max) {
			break;
		}
		value += histogram->value_increment;
	}
	printf("\n");

	/* draw the graph horizontal axis line characters */
	printf_spaces(left_margin + 5 + bucket_label_strlen);
	value = histogram->value_min;
	while (1) {
		if (value % histogram->value_increment == 0) {
			if (value >= histogram->value_max) {
				print_border(3, 0);
			} else if (value <= histogram->value_min) {
				print_border(2, 0);
			} else {
				print_border(1, 0);
			}
		} else {
			print_border(0, 0);
		}
		if (value >= histogram->value_max) {
			break;
		}
		value += haxis_value;
		haxis_count++;
	}
	printf("\n");

	/* draw the vertical axis, and the bar graph blocks */
	unsigned int vaxis_value = histogram->bucket_value;
	unsigned int bucket;
	unsigned int bucket_value = histogram->bucket_min;
	for (bucket = 0; bucket < histogram->buckets; bucket++) {
		bucket_value = (vaxis_value * bucket) + histogram->bucket_min;
		if (bucket_value % histogram->bucket_increment == 0) {
			printf_spaces(left_margin);
			if (histogram->bucket_label_unit != NULL) {
				printf("%4d%s ", bucket_value, histogram->bucket_label_unit);
			} else {
				printf("%4d ", bucket_value);
			}
			if (bucket_value == histogram->bucket_max) {
				print_border(3, 1);
			} else {
				print_border(1, 1);
			}
		} else {
			printf_spaces(left_margin + 5 + bucket_label_strlen);
			print_border(0, 1);
		}
		
		/* render data */
		unsigned int value_mod = histogram->data[bucket];
		unsigned int i;
		for (i = 0; i < haxis_count * haxis_value; i += haxis_value) {
			if (value_mod >= i + haxis_value) {
				print_block(100, 0);
			} else if (value_mod >= i) {
				print_block((value_mod - i) * 10, 0);
			} else {
				//print_block(0, 0);
			}
		}

		if (histogram->value_label) {
			if (histogram->data[bucket] != 0) {
				if (histogram->value_label_unit != NULL) {
					printf(" %d%s", histogram->data[bucket], histogram->value_label_unit);
				} else {
					printf(" %d", histogram->data[bucket]);
				}
			}
		}

		printf("\n");
	}
}

void printf_spaces(int count)
{
	int i;
	for (i = 0; i < count; i++)
		printf(" ");
}

void print_border(int cross, int vertical)
{
	if (cross == 1) {
		printf("\u253C");
	} else if (cross == 2) {
		/* front end */
		if (vertical) {
			printf("\u252C");
		} else {
			printf("\u251C");
		}
	} else if (cross == 3) {
		/* back end */
		if (vertical) {
			printf("\u2534");
		} else {
			printf("\u2524");
		}
	} else {
		if (vertical) {
			printf("\u2502");
		} else {
			printf("\u2500");
		}
	}
}

void print_block(int value, int vertical)
{
	if (value >= 80) {
		printf("\u2588"); /* full block character */
	} else if (value >= 70) {
		if (vertical)
			printf("\u2587"); /* vertical 7/8th block */
		else
			printf("\u2589"); /* horizontal 7/8th block */
	} else if (value >= 60) {
		if (vertical)
			printf("\u2586"); /* vertical 6/8th block */
		else
			printf("\u258A"); /* horizontal 6/8th block */
	} else if (value >= 50) {
		if (vertical)
			printf("\u2585"); /* vertical 5/8th block */
		else
			printf("\u258B"); /* horizontal 5/8th block */
	} else if (value >= 40) {
		if (vertical)
			printf("\u2584"); /* vertical 4/8th block */
		else
			printf("\u258C"); /* horizontal 4/8th block */
	} else if (value >= 30) {
		if (vertical)
			printf("\u2583"); /* vertical 3/8th block */
		else
			printf("\u258D"); /* horizontal 3/8th block */
	} else if (value >= 20) {
		if (vertical)
			printf("\u2582");  /* vertical 2/8th block */
		else
			printf("\u258E"); /* horizontal 2/8th block */
	} else if (value >= 10) {
		if (vertical)
			printf("\u2581"); /* vertical 1/8th block */
		else
			printf("\u258F"); /* horizontal 1/8th block */
	} else if (value >= 0) {
		printf(" ");  /* 0/8th block or empty */
	}
}
