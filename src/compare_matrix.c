#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "matrix_lib_o.h"
#include "arg_lib.h"

float fabsf(float x);

int main(int argc, char *argv[])
{
	float tolerance;
	Matrix *matrix_a, *matrix_b;
	unsigned int diff_found = 0;
	float *arr_a_rows, *arr_b_rows;
	unsigned long int m_height, m_width, i, j;
	const char *matrix_a_bfname, *matrix_b_bfname;

	if (argc < 4) {
		fprintf(stderr, "USAGE: %s <matrix A bin file> <matrix B bin file> <matrixes height> <matrixes width> <tolerance>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	matrix_a_bfname = argv[1];
	matrix_b_bfname = argv[2];
	m_height = argtoul(argv[3]);
	m_width = argtoul(argv[4]);
	tolerance = argtof(argv[5]);

	matrix_a = read_matrix_binfile(matrix_a_bfname, m_width, m_height);
	if (!matrix_a) {
		fprintf(stderr, "ERROR: Could not open file \"%s\"\n", matrix_a_bfname);
		goto fail1;
	}

	matrix_b = read_matrix_binfile(matrix_b_bfname, m_width, m_height);
	if (!matrix_b) {
		fprintf(stderr, "ERROR: Could not open file \"%s\"\n", matrix_b_bfname);
		goto fail2;
	}

	arr_a_rows = matrix_a->rows;
	arr_b_rows = matrix_b->rows;
	for (i = 0; i < m_height; ++i) {
		for (j = 0; j < m_width; ++j, ++arr_a_rows, ++arr_b_rows) {
			if (fabsf(*arr_a_rows - *arr_b_rows) > tolerance) {
				diff_found = 1;
				fprintf(stderr, "FOUND DIFF AT [%ld, %ld] : %f -- %f\n",
						i, j, *arr_a_rows, *arr_b_rows);
			}
		}
	}

	delete_matrix(matrix_a);
	delete_matrix(matrix_b);

	if (diff_found) {
		printf("Matrizes não são iguais.\n");
	} else {
		printf("Matrizes são iguais.\n");
	}

	return diff_found;

fail2:
	delete_matrix(matrix_a);
fail1:
	return -1;
}

float fabsf(float x)
{
	int *x_i = (int *)&x;
	*x_i &= 0x7fffffff;
	return x;
}

