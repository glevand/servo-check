#if !defined(_UTIL_H)
#define _UTIL_H

#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

void set_verbose(bool state);

void __attribute__ ((format (printf, 3, 4))) _log(const char *func, int line,
	const char *fmt, ...);
#define log(_args...) do {_log(__func__, __LINE__, _args);} while(0)

void __attribute__ ((format (printf, 3, 4))) _debug(const char *func, int line,
	const char *fmt, ...);
#define debug(_args...) do {_debug(__func__, __LINE__, _args);} while(0)

const char *eat_front_ws(const char *p);
const char *eat_front_chars(const char *p);

float to_float(const char *str);
int to_signed(const char *str);
unsigned int to_unsigned(const char *str);

void __attribute__((unused)) print_array(const int *array, unsigned int array_len);

#endif /* _UTIL_H */
