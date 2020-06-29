/*
 *  Compare servo motor sensor data for failures.
 */

#define _GNU_SOURCE
#define _DEFAULT_SOURCE

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include "filter.h"
#include "util.h"

#if !defined(PACKAGE_NAME) || !defined(PACKAGE_VERSION)
# error PACKAGE_VERSION not defined.
#endif

#if !defined(PACKAGE_BUGREPORT)
# error PACKAGE_BUGREPORT not defined.
#endif

static const char version_string[]
	= PACKAGE " (" PACKAGE_NAME ") " PACKAGE_VERSION;

static void print_version(void)
{
	printf("%s\n", version_string);
}

static void print_bugreport(void)
{
	fprintf(stderr, "Send bug reports to " PACKAGE_BUGREPORT ".\n");
}

enum opt_value {opt_undef = 0, opt_yes, opt_no};

struct opts {
	enum opt_value help;
	enum opt_value verbose;
	enum opt_value version;
	unsigned int e_len;
	unsigned int p_len;
	unsigned int error_limit;
	float phase_lag;
	enum opt_value show_filtered;
	enum opt_value show_stats;
	const char *file_name;
};

static void print_usage(const struct opts *opts)
{
	print_version();

	fprintf(stderr,
		"servo-check - Servo motor sensor check.\n"
		"Usage: Usage: servo-check [flags] data-file\n"
		"Option flags:\n"
		"  -h --help          - Show this help and exit.\n"
		"  -v --verbose       - Verbose execution.\n"
		"  -V --version       - Display the program version number.\n"
		"  --e-len            - Encoder filter length (e-len > 1).  Default: '%u'.\n"
		"  --p-len            - Potentiometer filter length (p-len > 1).  Default: '%u'.\n"
		"  --error-limit      - Error detection limit (error-limit > 1).  Default: '%u'.\n"
		"  --phase-lag        - Phase adjustment in seconds.  Default: '%f'.\n"
		"  -f --show-filtered - Print filtered signal data to stdout.\n"
		"  -s --show-stats    - Print signal stats to stdout at program exit.\n",
		 opts->e_len, opts->p_len, opts->error_limit, opts->phase_lag);

	print_bugreport();
}

static int opts_parse(struct opts *opts, int argc, char *argv[])
{
	static const struct option long_options[] = {
		{"help",          no_argument,       NULL, 'h'},
		{"verbose",       no_argument,       NULL, 'v'},
		{"version",       no_argument,       NULL, 'V'},
		{"e-len",         required_argument, NULL, 'e'},
		{"p-len",         required_argument, NULL, 'p'},
		{"error-limit",   required_argument, NULL, 'l'},
		{"phase-lag",     required_argument, NULL, 'g'},
		{"show-filtered", no_argument,       NULL, 'f'},
		{"show-stats",    no_argument,       NULL, 's'},
		{ NULL,         0,                   NULL, 0},
	};
	static const char short_options[] = "hvVfs";
	int result = 0;

	*opts = (struct opts) {
		.help = opt_no,
		.verbose = opt_no,
		.version = opt_no,
		.e_len = 400,
		.p_len = 400,
		.error_limit = 400,
		.phase_lag = (0.0f - HUGE_VALF),
		.show_stats = opt_no,
		.show_filtered = opt_no,
		.file_name = NULL,
	};

	if (0) {
		int i;

		set_verbose(true);
		debug("argc = %d\n", argc);
		for (i = 0; i < argc; i++) {
			debug("  %d: %p = '%s'\n", i, &argv[i], argv[i]);
		}
	}

	while (1) {
		int c = getopt_long(argc, argv, short_options, long_options,
			NULL);

		if (c == EOF) {
			break;
		}

		switch (c) {
		case 'e':
			opts->e_len = to_unsigned(optarg);
			if (opts->e_len == UINT_MAX) {
				opts->help = opt_yes;
				return -1;
			}
			break;
		case 'p':
			opts->p_len = to_unsigned(optarg);
			if (opts->p_len == UINT_MAX) {
				opts->help = opt_yes;
				return -1;
			}
			break;
		case 'l':
			opts->error_limit = to_unsigned(optarg);
			if (opts->error_limit == UINT_MAX) {
				opts->help = opt_yes;
				return -1;
			}
			break;
		case 'g':
			opts->phase_lag = to_float(optarg);
			if (opts->phase_lag == HUGE_VALF) {
				opts->help = opt_yes;
				return -1;
			}
			break;
		case 'f':
			opts->show_filtered = opt_yes;
			break;
		case 's':
			opts->show_stats = opt_yes;
			break;
		case 'h':
			opts->help = opt_yes;
			break;
		case 'v':
			opts->verbose = opt_yes;
			set_verbose(true);
			break;
		case 'V':
			opts->version = opt_yes;
			break;
		default:
			log("Internal error: %c:   %p = '%s'\n", c, optarg,
				optarg);
			assert(0);
			exit(EXIT_FAILURE);
		}
	}

	if (optind == argc) {
		log("ERROR: Please specify a data-file.\n");
		result = -1;
	}

	opts->file_name = argv[optind++];

	for ( ; optind < argc; optind++) {
		log("ERROR: Found extra agument: '%s'\n", argv[optind]);
		result = -1;
	}

	return result;
}

struct sensor_data {
	float time;
	int value;
};

struct servo_data {
	struct sensor_data encoder;
	struct sensor_data pot;
};

struct stats {
	unsigned int e_len;
	unsigned int p_len;
	int max_diff;
	struct servo_data max_diff_data;
	struct sensor_data e_min;
	struct sensor_data e_max;
	struct sensor_data p_min;
	struct sensor_data p_max;
};

static void stats_reset(struct stats *stats)
{
	struct stats old = *stats;

	memset(stats, 0, sizeof(*stats));
	stats->e_len = old.e_len;
	stats->p_len = old.p_len;
	stats->e_min.value = stats->p_min.value = INT_MAX;
	stats->e_max.value = stats->p_max.value = INT_MIN;
}

static void stats_init(struct stats *stats, unsigned int e_len,
	unsigned int p_len)
{
	stats->e_len = e_len;
	stats->p_len = p_len;
	stats_reset(stats);
}

static void stats_update(struct stats *stats, const struct servo_data *sd)
{
	int diff;

	//debug("%f %d %d\n", sd->time, sd->encoder, sd->pot);

	stats->e_min = (stats->e_min.value < sd->encoder.value)
		? stats->e_min : sd->encoder;

	stats->e_max = (stats->e_max.value > sd->encoder.value)
		? stats->e_max : sd->encoder;

	stats->p_min = (stats->p_min.value < sd->pot.value)
		? stats->p_min : sd->pot;

	stats->p_max = (stats->p_max.value > sd->pot.value)
		? stats->p_max : sd->pot;

	diff = abs(sd->encoder.value - sd->pot.value);
	
	if (diff > stats->max_diff) {
		stats->max_diff = diff;
		stats->max_diff_data = *sd;
	}
}

static void stats_print(const struct stats *stats, FILE *stream)
{
	fprintf(stream, "------------------------------------------------------------------------\n");
	fprintf(stream, "params:      e-len = %d, p-len = %d\n", stats->e_len, stats->p_len);

	fprintf(stream, "enc:         min = {%f, %d}, max = {%f, %d}\n",
		stats->e_min.time, stats->e_min.value,
		stats->e_max.time, stats->e_max.value);

	fprintf(stream, "pot:         min = {%f, %d}, max = {%f, %d}\n",
		stats->p_min.time, stats->p_min.value,
		stats->p_max.time, stats->p_max.value);

	fprintf(stream, "signal diff: min = {%f, %d}, max = {%f, %d} => %d\n",
		stats->p_min.time - stats->e_min.time,
		stats->p_min.value - stats->e_min.value,
		stats->p_max.time - stats->e_max.time,
		stats->p_max.value - stats->e_max.value,
		(stats->p_min.value - stats->e_min.value)
		- (stats->p_max.value - stats->e_max.value));

	fprintf(stream, "sample diff: enc = {%f, %d}, pot = {%f, %d} => %d\n",
		stats->max_diff_data.encoder.time,
			stats->max_diff_data.encoder.value,
		stats->max_diff_data.pot.time,
			stats->max_diff_data.pot.value,
		stats->max_diff);
	fprintf(stream, "------------------------------------------------------------------------\n");
}

struct calibration {
	int error_limit;
	int pot_offset;
	float phase_lag;
};

static void calibration_init(struct calibration *cal, int error_limit,
	float phase_lag)
{
	cal->error_limit = error_limit;
	cal->phase_lag = phase_lag;
}

static void calibration_set(struct calibration *cal, int pot_offset)
{
	debug("pot_offset = %d\n", pot_offset);
	cal->pot_offset = pot_offset;
}

static int servo_data_process(const struct servo_data *sd,
	const struct calibration *cal)
{
	int error = abs(sd->encoder.value - sd->pot.value);

	debug("error = %d, headroom = %d\n", error, cal->error_limit - error);

	if (error > cal->error_limit) {
		fprintf(stdout, "Sensor Error: %f\n", sd->encoder.time
			- cal->phase_lag);
		fflush(stdout);
		return -1;
	}

	return 0;
}

static int pot_scaler(int pot_value)
{
	float f;

	/*
	 * System Parameters:
	 * 2048 enc-ticks / motor-rev
	 * 30 motor-revs / out-rev
	 * 255 pot-ticks / out-rev
	 * 2048 * 30 / 255 enc-ticks / pot-tick
	 */

	f = (float)pot_value * (2048.0f * 30.0f) / 255.0f;
	//debug("%d => %f\n", pot_value, f);

	return (int)f;
}

struct control {
	bool show_filtered;
	bool show_stats;
};

struct filter_pair {
	struct calibration cal;
	struct stats stats;
	struct ave_filter *e_filter;
	struct ave_filter *p_filter;
};

static int process_line(struct filter_pair *filters, char *str,
	const struct control *control)
{
	struct servo_data raw;
	struct servo_data ave;
	char *p1, *p2, *p3;
	unsigned int u;
	int result;

	//debug("@%s@\n", str);

	str = (char *)eat_front_ws(str);
	p1 = str;
	str = (char *)eat_front_chars(str);
	*str = 0;
	//debug("p1 = @%s@\n", p1);

	str = (char *)eat_front_ws(str + 1);
	p2 = str;
	str = (char *)eat_front_chars(str);
	*str = 0;
	//debug("p2 = @%s@\n", p2);

	str = (char *)eat_front_ws(str + 1);
	p3 = str;
	str = (char *)eat_front_chars(str);
	*str = 0;
	//debug("p3 = @%s@\n", p3);

	raw.pot.time = raw.encoder.time = to_float(p1);

	if (raw.encoder.time == HUGE_VALF) {
		assert(0);
		exit(EXIT_FAILURE);
	}

	raw.encoder.value = to_signed(p2);

	if (raw.encoder.value == INT_MAX) {
		assert(0);
		exit(EXIT_FAILURE);
	}

	u = to_unsigned(p3);

	if (u == UINT_MAX) {
		assert(0);
		exit(EXIT_FAILURE);
	}

	if (u >= INT_MAX) {
		log("ERROR: data error.\n");
		assert(0);
		exit(EXIT_FAILURE);
	}

	raw.pot.value = (int)u;

	debug("raw: %f %d %d\n", raw.pot.time, raw.encoder.value,
		raw.pot.value);

	ave.pot.time = ave.encoder.time = raw.encoder.time;
if (0) {
	ave.encoder.value = ave_filter_run(filters->e_filter,
		raw.encoder.value);
}

if (1) {
	ave.pot.value = ave_filter_run(filters->p_filter,
		pot_scaler(raw.pot.value) - filters->cal.pot_offset);
}

	if (raw.pot.time <= 0.5f
		&& (roundf(10.0f * raw.pot.time) == (10.0f * raw.pot.time))) {
		stats_reset(&filters->stats);
		calibration_set(&filters->cal, ave.pot.value);
	}

	if (control->show_filtered) {
		fprintf(stdout, "%f %d %d\n", ave.encoder.time,
			ave.encoder.value, ave.pot.value);
	}

	if (control->show_stats) {
		stats_update(&filters->stats, &ave);
	}

	result = servo_data_process(&ave, &filters->cal);

	return result;
}

static int process_file(struct filter_pair *filters, const char *file,
	const struct control *control)
{
	char *line_data;
	size_t bytes;
	int result;
	int line;
	FILE *fp;

	fp = fopen(file, "r");

	if (!fp) {
		log("ERROR: fopen '%s' failed: %s\n", file, strerror(errno));
		assert(0);
		exit(EXIT_FAILURE);
	}

	for (line = 1, line_data = NULL, bytes = result = 0; ; line++) {

		bytes = getline(&line_data, &bytes, fp);

		//debug("%d: %zd bytes\n", line, bytes);

		if (ferror(fp)) {
			log("ERROR: fread '%s', line %u failed: %s\n", file,
				line, strerror(errno));
			fclose(fp);
			assert(0);
			exit(EXIT_FAILURE);
		}

		if (feof(fp)) {
			break;
		}

		result = process_line(filters, line_data, control) ? 1 : result;

		if (result && !control->show_filtered && !control->show_stats) {
			break;
		}
	}

	free(line_data);
	fclose(fp);

	return !!result;
}

int main(int argc, char *argv[])
{
	struct filter_pair filters;
	struct control control;
	struct opts opts;
	int result = 0;

	if (opts_parse(&opts, argc, argv)) {
		print_usage(&opts);
		return EXIT_FAILURE;
	}

	if (opts.phase_lag == (0.0f - HUGE_VALF)) {
		opts.phase_lag = (float)(opts.e_len + opts.p_len) / 4000.0f;
	}
	
	if (opts.help == opt_yes) {
		print_usage(&opts);
		return EXIT_SUCCESS;
	}

	if (opts.version == opt_yes) {
		print_version();
		return EXIT_SUCCESS;
	}

	if (opts.e_len == 0) {
		log("ERROR: <e-len> out of range: %u.\n", opts.e_len);
		print_usage(&opts);
		return EXIT_FAILURE;
	}

	if (opts.p_len == 0) {
		log("ERROR: <p-len> out of range: %u.\n", opts.p_len);
		print_usage(&opts);
		return EXIT_FAILURE;
	}

	if (opts.error_limit == 0) {
		log("ERROR: <error-limit> out of range: %u.\n",
			opts.error_limit);
		print_usage(&opts);
		return EXIT_FAILURE;
	}

	control.show_filtered = (opts.show_filtered == opt_yes);
	control.show_stats = (opts.show_stats == opt_yes);
	
	filters.e_filter = ave_filter_init(opts.e_len);
	filters.p_filter = ave_filter_init(opts.p_len);

	calibration_init(&filters.cal, (int)opts.error_limit, opts.phase_lag);
	stats_init(&filters.stats, opts.e_len, opts.p_len);

	result = process_file(&filters, opts.file_name, &control);

	if (!result) {
		fprintf(stdout, "System OK\n");
	}

	if (control.show_stats) {
		stats_print(&filters.stats, stdout);
	}

	ave_filter_delete(filters.e_filter);
	ave_filter_delete(filters.p_filter);

	return result ? EXIT_FAILURE : EXIT_SUCCESS;
}
