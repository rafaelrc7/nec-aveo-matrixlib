#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "matrix_lib_o.h"
#include "timer.h"

static void die(const char *msg);
static unsigned long int argtoul(const char *arg);
static int argtoi(const char *arg);
static float argtof(const char *arg);

int main(int argc, char *argv[])
{
	int ret, num_threads;
	float escalar;
	unsigned long int a_height, a_width;
	unsigned long int b_height, b_width;
	const char *bf1, *bf2, *bf3, *bf4;
	Matrix *matrixA, *matrixB, *matrixC;

	struct timeval start, stop, overall_t1, overall_t2;

	gettimeofday(&overall_t1, NULL);

	if (argc != 11) {
		fprintf(stderr, "USAGE: %s <scalar> <matrix_a_height> <matrix_a_width>"
						" <matrix_b_height> <matrix_b_width> <num_threads>"
						" <matrix_a_file> <matrix_b_file> <result1_file>"
						" <result2_file>\n",
						argv[0]);
		die("Insuficient arguments");
	}

	escalar = argtof(argv[1]);
	a_height = argtoul(argv[2]);
	a_width = argtoul(argv[3]);
	b_height = argtoul(argv[4]);
	b_width = argtoul(argv[5]);
	num_threads = argtoi(argv[6]);
	bf1 = argv[7];
	bf2 = argv[8];
	bf3 = argv[9];
	bf4 = argv[10];

	gettimeofday(&start, NULL);
	set_number_threads(num_threads);
	matrixA = read_matrix_binfile(bf1, a_width, a_height);
	matrixB = read_matrix_binfile(bf2, b_width, b_height);
	matrixC = zero_matrix(a_height, b_width);
	gettimeofday(&stop, NULL);

	if (!matrixA || !matrixB || !matrixC)
		die("Matrixes creation failure");

	printf("matrix init time: %f ms\n", timedifference_msec(start, stop));

	gettimeofday(&start, NULL);
	ret = scalar_matrix_mult(escalar, matrixA);
	gettimeofday(&stop, NULL);
	if (!ret)
		die("scalar_matrix_mult() call failure");

	printf("scalar_matrix_mult time: %f ms\n", timedifference_msec(start, stop));

	dump_matrix_binfile(bf3, matrixA);

	gettimeofday(&start, NULL);
	ret = matrix_matrix_mult(matrixA, matrixB, matrixC);
	gettimeofday(&stop, NULL);
	if (!ret)
		die("matrix_matrix_mult() call failure");

	printf("matrix_matrix_mult time: %f ms\n", timedifference_msec(start, stop));

	dump_matrix_binfile(bf4, matrixC);

	delete_matrix(matrixA);
	delete_matrix(matrixB);
	delete_matrix(matrixC);

	gettimeofday(&overall_t2, NULL);
	printf("overall time: %f ms\n", timedifference_msec(overall_t1, overall_t2));

	return 0;
}

static unsigned long int argtoul(const char *arg)
{
	unsigned long int ret;
	char *endptr;

	if (*arg == 0)
		die("Invalid argument");

	ret = strtoul(arg, &endptr, 0);

	if (*endptr != 0)
		die("Invalid argument");

	return ret;
}

static int argtoi(const char *arg)
{
	int ret;
	char *endptr;

	if (*arg == 0)
		die("Invalid argument");

	ret = (int)strtol(arg, &endptr, 10);

	if (*endptr != 0)
		die("Invalid argument");

	return ret;
}

static float argtof(const char *arg)
{
	float ret;
	char *endptr;

	if (*arg == 0)
		die("Invalid argument");

	ret = strtof(arg, &endptr);

	if (*endptr != 0)
		die("Invalid argument");

	return ret;
}

static void die(const char *msg)
{
	fprintf(stderr, "FATAL ERROR: %s.\nAborting program...\n", msg);
	exit(EXIT_FAILURE);
}
