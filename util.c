#define _GNU_SOURCE
#define _DEFAULT_SOURCE

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

static bool opt_verbose = false;

void set_verbose(bool state)
{
	opt_verbose = state;
}

void __attribute__ ((format (printf, 3, 4))) _log(const char *func, int line,
	const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "%s:%d: ", func, line);
	vfprintf(stderr, fmt, ap);
	fflush(stderr);
	va_end(ap);
}

void __attribute__ ((format (printf, 3, 4))) _debug(const char *func, int line,
	const char *fmt, ...)
{
	va_list ap;
	
	if(!opt_verbose) {
		return;
	}

	va_start(ap, fmt);
	fprintf(stderr, "%s:%d: ", func, line);
	vfprintf(stderr, fmt, ap);
	fflush(stderr);
	va_end(ap);
}

const char *eat_front_ws(const char *p)
{
	assert(p);
	//debug("@%s@\n", p);

	while (*p && (*p == ' ' || *p == '\t' || *p == '\r')) {
		//debug("remove='%c'\n", *p);
		p++;
	}

	return p;
}

const char *eat_front_chars(const char *p)
{
	assert(p);
	//debug("@%s@\n", p);

	while (*p && (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')) {
		//debug("remove='%c'\n", *p);
		p++;
	}

	return p;
}

float to_float(const char *str)
{
	const char *p;
	const char *start;
	char *end;
	float f;
	bool found_decimal = false;

	assert(str);
	//debug("'%s'\n", str);

	start = eat_front_ws(str);

	for (p = (*start != '-') ? start : start + 1; *p; p++) {
		if (*p == '-') {
			log("ERROR: bad sign: '%s'\n", str);
			return HUGE_VALF;
		}
		if (*p == '.') {
			if (found_decimal) {
				log("ERROR: multiple decimal: '%s'\n", str);
				return HUGE_VALF;
			}
			found_decimal = true;
			continue;
		}		
		if (!isdigit(*p)) {
			log("ERROR: isdigit failed: '%s'\n", str);
			return HUGE_VALF;
		}
	}

	f = strtof(start, &end);

	if ((f == 0.0 && end == str) || f == HUGE_VALF) {
		log("ERROR: strtof '%s' failed: %s\n", str, strerror(errno));
		return HUGE_VALF;
	}

	//debug("'%s' => %f\n", start, f);
	return f;
}

int to_signed(const char *str)
{
	const char __attribute__ ((unused)) *start;
	const char *p;
	long l;

	assert(str);
	//debug("'%s'\n", str);

	start = eat_front_ws(str);

	for (p = str; *p; p++) {
		if (!isdigit(*p) && *p != '-') {
			log("ERROR: isdigit failed: '%s'\n", str);
			return INT_MAX;
		}
	}

	l = strtoul(str, NULL, 10);

	if (l == LONG_MAX) {
		log("ERROR: strtoul '%s' failed: %s\n", str, strerror(errno));
		return INT_MAX;
	}
	
	if (l > INT_MAX) {
		log("ERROR: too big: %lu\n", l);
		return INT_MAX;
	}

	//debug("'%s' => %ld\n", start, l);
	return (int)l;
}

unsigned int to_unsigned(const char *str)
{
	const char __attribute__ ((unused)) *start;
	const char *p;
	unsigned long u;

	assert(str);
	//debug("'%s'\n", str);

	start = eat_front_ws(str);

	for (p = str; *p; p++) {
		if (!isdigit(*p)) {
			log("ERROR: isdigit failed: '%s'\n", str);
			return UINT_MAX;
		}
	}

	u = strtoul(str, NULL, 10);

	if (u == ULONG_MAX) {
		log("ERROR: strtoul '%s' failed: %s\n", str, strerror(errno));
		return UINT_MAX;
	}
	
	if (u > UINT_MAX) {
		log("ERROR: too big: %lu\n", u);
		return UINT_MAX;
	}

	//debug("'%s' => %lu\n", start, u);
	return (unsigned int)u;
}


void print_array(const int *array, unsigned int array_len)
{
	unsigned int i;

	for (i = 0; i < array_len; i++) {
		fprintf(stderr, "%d ", array[i]);
	}
	fprintf(stderr, "\n");
}
