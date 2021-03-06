#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "matrix_lib.h"
#include "timer.h"

static void die(const char *msg);
static unsigned long int argtoul(const char *arg);
static int argtoi(const char *arg);
static float argtof(const char *arg);

int main(int argc, char *argv[])
{
	float escalar;
	int ret, ve_num_threads, ve_id_number;
	unsigned long int a_height, a_width;
	unsigned long int b_height, b_width;
	const char *bf1, *bf2, *bf3, *bf4;
	struct matrix *matrixA, *matrixB, *matrixC;

	struct timeval start, stop, overall_t1, overall_t2;

	gettimeofday(&overall_t1, NULL);

	if (argc != 12) {
		fprintf(stderr, "USAGE: %s <scalar> <matrix_a_height> <matrix_a_width>"
						" <matrix_b_height> <matrix_b_width> <ve_id_number>"
						" <ve_num_threads> <matrix_a_file> <matrix_b_file>"
						" <result1_file> <result2_file>\n",
						argv[0]);
		die("Insuficient arguments");
	}

	escalar = argtof(argv[1]);
	a_height = argtoul(argv[2]);
	a_width = argtoul(argv[3]);
	b_height = argtoul(argv[4]);
	b_width = argtoul(argv[5]);
	ve_id_number = argtoi(argv[6]);
	ve_num_threads = argtoi(argv[7]);
	bf1 = argv[8];
	bf2 = argv[9];
	bf3 = argv[10];
	bf4 = argv[11];

	gettimeofday(&start, NULL);
	set_ve_execution_node(ve_id_number);
	set_number_threads(ve_num_threads);
	ret = init_proc_ve_node();
	matrixA = read_matrix_binfile(bf1, a_width, a_height);
	ret = load_ve_matrix(matrixA);
	if (!ret)
		die("load_ve_matrix()");

	matrixB = read_matrix_binfile(bf2, b_width, b_height);
	ret = load_ve_matrix(matrixB);
	if (!ret)
		die("load_ve_matrix()");

	matrixC = zero_matrix(a_height, b_width);
	ret = load_ve_matrix(matrixC);
	if (!ret)
		die("load_ve_matrix()");

	gettimeofday(&stop, NULL);

	if (!ret)
		die("init_proc_ve_node()");

	if (!matrixA || !matrixB || !matrixC)
		die("Matrixes creation failure");

	printf("matrix init time: %f ms\n", timedifference_msec(start, stop));

	gettimeofday(&start, NULL);
	ret = sync_vh_ve_matrix(matrixA);
	if (!ret)
		die("sync_vh_ve_matrix()");

	ret = scalar_matrix_mult(escalar, matrixA);
	if (!ret)
		die("scalar_matrix_mult()");

	ret = sync_ve_vh_matrix(matrixA);
	if (!ret)
		die("sync_ve_vh_matrix()");
	gettimeofday(&stop, NULL);

	printf("scalar_matrix_mult time: %f ms\n", timedifference_msec(start, stop));

	dump_matrix_binfile(bf3, matrixA);

	gettimeofday(&start, NULL);
	ret = sync_vh_ve_matrix(matrixA);
	if (!ret)
		die("sync_vh_ve_matrix()");

	ret = sync_vh_ve_matrix(matrixB);
	if (!ret)
		die("sync_vh_ve_matrix()");

	ret = matrix_matrix_mult(matrixA, matrixB, matrixC);
	if (!ret)
		die("matrix_matrix_mult() call failure");

	ret = sync_ve_vh_matrix(matrixA);
	if (!ret)
		die("sync_ve_vh_matrix()");

	ret = sync_ve_vh_matrix(matrixB);
	if (!ret)
		die("sync_ve_vh_matrix()");

	ret = sync_ve_vh_matrix(matrixC);
	if (!ret)
		die("sync_ve_vh_matrix()");
	gettimeofday(&stop, NULL);

	printf("matrix_matrix_mult time: %f ms\n", timedifference_msec(start, stop));

	dump_matrix_binfile(bf4, matrixC);

	ret = unload_ve_matrix(matrixA);
	if (!ret)
		die("unload_ve_matrix()");

	ret = unload_ve_matrix(matrixB);
	if (!ret)
		die("unload_ve_matrix()");

	ret = unload_ve_matrix(matrixC);
	if (!ret)
		die("unload_ve_matrix()");

	delete_matrix(matrixA);
	delete_matrix(matrixB);
	delete_matrix(matrixC);

	ret = close_proc_ve_node();
	if (!ret)
		die("close_proc_ve_node()");

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

