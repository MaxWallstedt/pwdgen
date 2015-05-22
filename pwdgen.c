#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_LIM_FUNCS	32
#define MAX_LIM_CHARS	256

/* Globals */

size_t length = 8;

int (*accept_funcs[MAX_LIM_FUNCS])(int c);
size_t n_accept_funcs = 0;
char accept_chars[MAX_LIM_CHARS];
size_t n_accept_chars = 0;

int (*exclude_funcs[MAX_LIM_FUNCS])(int c);
size_t n_exclude_funcs = 0;
char exclude_chars[MAX_LIM_CHARS];
size_t n_exclude_chars = 0;

int rndfd;

/* Strings */

const char *arg_length_no_param_short = "-l";
const char *arg_length_no_param_long = "--length";
const char *arg_length_param = "--length=";

const char *arg_accept_no_param_short = "-a";
const char *arg_accept_no_param_long = "--accept";
const char *arg_accept_param = "--accept=";

const char *arg_exclude_no_param_short = "-e";
const char *arg_exclude_no_param_long = "--exclude";
const char *arg_exclude_param = "--exclude=";

/* Logic */

void parse_length(const char *param);
void parse_accept(const char *param);
void parse_exclude(const char *param);

void pwdgen_main();

/* Conditions */

int arg_is_length_no_param(const char *arg);
int arg_is_length_param(const char *arg);

int arg_is_accept_no_param(const char *arg);
int arg_is_accept_param(const char *arg);

int arg_is_exclude_no_param(const char *arg);
int arg_is_exclude_param(const char *arg);

/* Errors */

void error_invalid_arg(const char *arg);

void error_length_no_param(const char *length_str);
void error_accept_no_param(const char *accept_str);
void error_exclude_no_param(const char *exclude_str);

void error_length_bad_param(const char *param);
void error_accept_bad_param(const char *param);
void error_exclude_bad_param(const char *param);

void error_invalid_func(const char *func);

/*
 * -l <length> / --length <length> / --length=<length>
 * -a string/@func / --accept string/@func / --accept=string/@func
 * -e string/@func / --exclude string/@func / --exclude=string/@func
 * -h / --help
 * -v / --version
 *
 * Examples:
 *   ./pwdgen -l 8 --accept @isgraph --exclude=xyz
 */
int main(int argc, char *argv[])
{
	int i;
	const char *param;

	for (i = 1; i < argc; ++i) {
		if (arg_is_length_no_param(argv[i])) {
			++i;

			if (i == argc) {
				error_length_no_param(argv[i - 1]);
			}

			param = argv[i];
			parse_length(param);
		} else if (arg_is_length_param(argv[i])) {
			if (argv[i][strlen(arg_length_param)] == '\0') {
				error_length_no_param(argv[i]);
			}

			param = &argv[i][strlen(arg_length_param)];
			parse_length(param);
		} else if (arg_is_accept_no_param(argv[i])) {
			++i;

			if (i == argc) {
				error_accept_no_param(argv[i - 1]);
			}

			param = argv[i];
			parse_accept(param);
		} else if (arg_is_accept_param(argv[i])) {
			if (argv[i][strlen(arg_accept_param)] == '\0') {
				error_accept_no_param(argv[i]);
			}

			param = &argv[i][strlen(arg_accept_param)];
			parse_accept(param);
		} else if (arg_is_exclude_no_param(argv[i])) {
			++i;

			if (i == argc) {
				error_exclude_no_param(argv[i - 1]);
			}

			param = argv[i];
			parse_exclude(param);
		} else if (arg_is_exclude_param(argv[i])) {
			if (argv[i][strlen(arg_exclude_param)] == '\0') {
				error_exclude_no_param(argv[i]);
			}

			param = &argv[i][strlen(arg_exclude_param)];
			parse_exclude(param);
		} else {
			error_invalid_arg(argv[i]);
		}
	}

	if (n_accept_funcs == 0 && n_exclude_funcs == 0) {
		parse_accept("@isgraph");
	}

	rndfd = open("/dev/urandom", O_RDONLY);

	if (rndfd == -1) {
		perror("/dev/urandom");
		exit(EXIT_FAILURE);
	}

	pwdgen_main();
	close(rndfd);

	return 0;
}

void parse_length(const char *param)
{
	int i;
	size_t tmp_length = 0;

	for (i = 0; param[i] != '\0'; ++i) {
		if (param[i] < '0' || param[i] > '9') {
			error_length_bad_param(param);
		}

		tmp_length = (tmp_length << 3) + (tmp_length << 1)
		             + param[i] - '0';
	}

	length = tmp_length;
}

void add_accept_func(int (*f)(int c))
{
	accept_funcs[n_accept_funcs++] = f;
}

void add_accept_str(const char *param)
{
	strcpy(&accept_chars[n_accept_chars], param);
	n_accept_chars += strlen(param);
}

void add_exclude_func(int (*f)(int c))
{
	exclude_funcs[n_exclude_funcs++] = f;
}

void add_exclude_str(const char *param)
{
	strcpy(&exclude_chars[n_exclude_chars], param);
	n_exclude_chars += strlen(param);
}

int (*get_func(const char *func))(int c)
{
	if (strcmp(func, "isalnum") == 0) {
		return isalnum;
	} else if (strcmp(func, "isalpha") == 0) {
		return isalpha;
	} else if (strcmp(func, "isascii") == 0) {
		return isascii;
	} else if (strcmp(func, "isblank") == 0) {
		return isblank;
	} else if (strcmp(func, "iscntrl") == 0) {
		return iscntrl;
	} else if (strcmp(func, "isdigit") == 0) {
		return isdigit;
	} else if (strcmp(func, "isgraph") == 0) {
		return isgraph;
	} else if (strcmp(func, "islower") == 0) {
		return islower;
	} else if (strcmp(func, "isprint") == 0) {
		return isprint;
	} else if (strcmp(func, "ispunct") == 0) {
		return ispunct;
	} else if (strcmp(func, "isspace") == 0) {
		return isspace;
	} else if (strcmp(func, "isupper") == 0) {
		return isupper;
	} else if (strcmp(func, "isxdigit") == 0) {
		return isxdigit;
	} else {
		error_invalid_func(func);
	}

	return NULL;
}

void parse_accept(const char *param)
{
	if (param[0] == '@') {
		add_accept_func(get_func(&param[1]));
	} else {
		add_accept_str(param);
	}
}

void parse_exclude(const char *param)
{
	if (param[0] == '@') {
		add_exclude_func(get_func(&param[1]));
	} else {
		add_exclude_str(param);
	}
}

int rndascii()
{
	unsigned char rnd;
	ssize_t nread;

	nread = read(rndfd, &rnd, 1);

	if (nread != 1) {
		return -1;
	}

	return rnd & 0x7F;
}

int passes_one_func(int c, int (*funcs[])(int), size_t n_funcs)
{
	size_t i;

	for (i = 0; i < n_funcs; ++i) {
		if (funcs[i](c)) {
			return 1;
		}
	}

	return 0;
}

int is_member_of(int c, char a[], size_t l)
{
	size_t i;

	for (i = 0; i < l; ++i) {
		if (c == a[i]) {
			return 1;
		}
	}

	return 0;
}

void pwdgen_main()
{
	size_t i = 0;
	int c;

	while (i < length) {
		c = rndascii();

		if (!passes_one_func(c, accept_funcs, n_accept_funcs)
		    && !is_member_of(c, accept_chars, n_accept_chars)) {
			continue;
		}

		if (passes_one_func(c, exclude_funcs, n_exclude_funcs)
		    || is_member_of(c, exclude_chars, n_exclude_chars)) {
			continue;
		}

		putchar(c);
		++i;
	}

	putchar('\n');
}

int arg_is_length_no_param(const char *arg)
{
	if (strcmp(arg, arg_length_no_param_short) == 0) {
		return 1;
	}

	if (strcmp(arg, arg_length_no_param_long) == 0) {
		return 1;
	}

	return 0;
}

int arg_is_length_param(const char *arg)
{
	if (strncmp(arg,
	            arg_length_param, strlen(arg_length_param)) == 0) {
		return 1;
	}

	return 0;
}

int arg_is_accept_no_param(const char *arg)
{
	if (strcmp(arg, arg_accept_no_param_short) == 0) {
		return 1;
	}

	if (strcmp(arg, arg_accept_no_param_long) == 0) {
		return 1;
	}

	return 0;
}

int arg_is_accept_param(const char *arg)
{
	if (strncmp(arg,
	            arg_accept_param, strlen(arg_accept_param)) == 0) {
		return 1;
	}

	return 0;
}

int arg_is_exclude_no_param(const char *arg)
{
	if (strcmp(arg, arg_exclude_no_param_short) == 0) {
		return 1;
	}

	if (strcmp(arg, arg_exclude_no_param_long) == 0) {
		return 1;
	}

	return 0;
}

int arg_is_exclude_param(const char *arg)
{
	if (strncmp(arg,
	            arg_exclude_param, strlen(arg_exclude_param)) == 0) {
		return 1;
	}

	return 0;
}

void error_invalid_arg(const char *arg)
{
	fprintf(stderr, "Error: Invalid argument \"%s\"\n", arg);
	exit(EXIT_FAILURE);
}

void error_length_no_param(const char *length_str)
{
	fprintf(stderr,
	        "Error: Must specify length after %s\n", length_str);
	exit(EXIT_FAILURE);
}

void error_accept_no_param(const char *accept_str)
{
	fprintf(stderr,
	        "Error: Must specify string or function after %s\n",
	        accept_str);
	exit(EXIT_FAILURE);
}

void error_exclude_no_param(const char *exclude_str)
{
	fprintf(stderr,
	        "Error: Must specify string or function after %s\n",
	        exclude_str);
	exit(EXIT_FAILURE);
}

void error_length_bad_param(const char *param)
{
	fprintf(stderr, "Error: Invalid length \"%s\"\n", param);
	exit(EXIT_FAILURE);
}

void error_accept_bad_param(const char *param)
{
	fprintf(stderr,
	        "Error: Invalid string or function \"%s\"\n", param);
	exit(EXIT_FAILURE);
}

void error_exclude_bad_param(const char *param)
{
	fprintf(stderr,
	        "Error: Invalid string or function \"%s\"\n", param);
	exit(EXIT_FAILURE);
}

void error_invalid_func(const char *func)
{
	fprintf(stderr, "Error: Invalid function \"%s\"\n", func);
	exit(EXIT_FAILURE);
}
