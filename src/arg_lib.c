#include <stdio.h>

#include "arg_lib.h"

static void die()
{
	fprintf(stderr, "ERROR: invalid argument.\n");
	exit(EXIT_FAILURE);
}

unsigned long int argtoul(const char *arg)
{
	unsigned long int ret;
	char *endptr;

	if (*arg == 0) die();
	ret = strtoul(arg, &endptr, 0);
	if (*endptr != 0) die();
	return ret;
}

int argtoi(const char *arg)
{
	int ret;
	char *endptr;

	if (*arg == 0) die();
	ret = (int)strtol(arg, &endptr, 10);
	if (*endptr != 0) die();
	return ret;
}


float argtof(const char *arg)
{
	float ret;
	char *endptr;

	if (*arg == 0) die();
	ret = strtof(arg, &endptr);
	if (*endptr != 0) die();
	return ret;
}
