//	check_goal_states_file.c

#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

#define FILENAME_BUFLEN 120

static char *goal_states_fname;
static FILE *goal_states_fp = NULL;

static void dbg_err (int err, const char *fmt, ...)
{
		char errbuf[100];

    va_list arg_ptr;
    va_start(arg_ptr, fmt);
    vprintf(fmt, arg_ptr);
    va_end(arg_ptr);
		printf ("%s\n", strerror_r (err, errbuf, 100));
}

// compare line to arg, skipping spaces in line
bool compare_line (const char *line, const char *arg)
{
	int li = 0;
	int ai = 0;
	char ac, lc;

	while (true)
	{
		ac = arg[ai];
		lc = line[li];
		if (lc==0)
			return (bool) (ac==0);
		li++;
		if ((lc == ' ') || (lc == '\n'))
			continue;
		if (ac != lc)
			return false;
		ai++;
	}
	return true;
}

bool compare_file (int argc, char *argv[])
{
	char line[128];
	int i;
	char *arg;

	for (i=3; i<argc; i++) 
	{
		arg = argv[i];
		if (NULL == fgets (line, 128, goal_states_fp))
			return false;
		if (!compare_line (line, arg))
			return false;
	}
	if (NULL == fgets (line, 128, goal_states_fp))
		return true;	// file matches
	return false;
}

int main(int argc, char *argv[])
{
	char *tf_arg;
	bool expected, result;

	if (argc < 4) {
		printf ("Expecting at least 3 arguments:\n");
		printf (" 1) goal states file name\n");
		printf (" 2) expected match (true/false)\n");
		printf (" 3..) test arguments\n");
		return EINVAL;
	}
	goal_states_fname = argv[1];
	tf_arg = argv[2];
	if (strcmp (tf_arg, "true") == 0)
		expected = true;
	else if (strcmp (tf_arg, "false") == 0)
		expected = false;
	else {
		printf ("Invalid true/false arg(2)\n");
		return -1;
	}

	goal_states_fp = fopen (goal_states_fname, "r");
	if (NULL == goal_states_fp) {
		dbg_err (errno, "Error opening file %s\n", goal_states_fname);
		return -1;
	}

	result = compare_file (argc, argv);
	fclose (goal_states_fp);
	if (result == expected)
		return 0;
	return 4;
}
