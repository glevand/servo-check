/*
 * Moving average filter.
 * 
 */

#define _GNU_SOURCE
#define _DEFAULT_SOURCE

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "filter.h"
#include "util.h"

struct ave_filter {
	unsigned int len;
	int sum;
	int *head;
	int *end;
	int data[];
};

struct ave_filter *ave_filter_init(unsigned int len)
{
	struct ave_filter *f;
	unsigned int d_bytes;

	assert(len);

	d_bytes = len * sizeof(f->data[0]);

	f = malloc(sizeof(*f) + d_bytes);

	if (!f) {
		log("ERROR: malloc failed.\n");
		assert(0);
		exit(EXIT_FAILURE);
	}

	f->len = len;
	f->sum = 0;
	f->head = f->data;
	f->end = f->data + len - 1;
	memset(f->data, 0, d_bytes);

	return f;
}

void ave_filter_delete(struct ave_filter *f)
{
	assert(f);
	free(f);
}

int ave_filter_run(struct ave_filter *f, int x)
{
	int y;

	f->sum -= *f->head;
	*f->head = x;
	f->sum += *f->head;

	f->head = (f->head < f->end) ? f->head + 1 : f->data; 

	y = f->sum / (int)f->len;

	if (0) {
		debug("data = %p, head = %p, end = %p\n", f->data, f->head,
			f->end);
		print_array(f->data, f->len);
		debug("y = %d, sum = %d\n  ", y, f->sum);
		debug("{%d, %d}\n", x, (int)y);
	}

	return y;
}
