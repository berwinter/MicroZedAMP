#ifndef LATENCYGRAPH_H
#define LATENCYGRAPH_H

struct histogram_values {
	unsigned int bucket_min; /* bucket minimum value */
	unsigned int bucket_max; /* bucket maximum value */
	unsigned int bucket_value; /* individual bucket size */
	unsigned int bucket_increment; /* graph tick increment */
	char* bucket_label_unit; /* the graph label's unit */
	unsigned int value_min; /* value/count minimum value */
	unsigned int value_max; /* value/count maximum value */
	unsigned int value_increment; /* graph tick increment */
	unsigned int value_label; /* graph a label value */
	char* value_label_unit; /* the graph label's unit */
	unsigned int buckets; /* count of buckets */
	unsigned int* data; /* bucket data */
};

/*
 * Print the Graph using Block and Line characters.
 */
void print_graph(struct histogram_values* histogram);

#endif /* LATENCYGRAPH_H */